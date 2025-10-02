# Rhythm Language Introduction

Rhythm is a dynamic typed language, meaning that variables do not have a fixed type (values do). Its syntax can be described as a cleaned up JavaScript, with notable differences in a few keywords (`fun` instead of `function`, `and` instead of `&&`, and `or` of `||`). Typically a JavaScript syntax highlighter works well enough for Rhythm code (as shown here).  Rhythm has first class function and lexical closure; no structure or class. It also has builtin dynamic array `Array` and hashtable `Map`.

The `beat` CLI tool is a bytecode compiler and interpreter for the Rhythm language. The `transpose` CLI transpiles Rhythm programs into JavaScript and runs them with Node.js.

Here's an example code in Rhythm

```javascript
fun min(array, less) {
    assert(len(array) > 0);
    var result = array[0];
    var n = len(array);
    for (var i=1; i<n; i=i+1) {
		if (less(array[i], result))
			result = array[i];
    }
    return result;
}

// sort things in ascending order
fun qsort(A, lo, hi, less) {
    if (hi-lo <= 1) return; // do nothing
    var p = partition(A, lo, hi, less);
    qsort(A, lo, p, less);
    qsort(A, p+1, hi, less);
}

// a, b are
fun swap(A, i, j){
    var t = A[i];
    A[i] = A[j];
    A[j] = t;
}

// return pivot index
fun partition(A, lo, hi, less) {
    assert(lo < hi);
    var pivot = A[lo]; // use first elem as pivot
    var i = lo+1;
    var j = hi-1;
    while (true) {
        while (i < hi and less(A[i], pivot)) i = i + 1;
        while (j > lo and less(pivot, A[j])) j = j - 1;
        if (i >= j) {
            swap(A, lo, j);
            return j;
        }
        swap(A, i, j);
    }
}


var A = [3, 4, 2, 6, 5];
print partition(A, 0, 5, fun(a,b){return a<b;}); // expects 1
print A; // expects [2, 3, 4, 6, 5,];

var A = [3, 4, 2, 6, 5];
qsort(A, 0, len(A), fun(a,b) {return a<b;} );
print A; // expects [2,3,4,5,6];

var intervals = [ [1,2], [3, 4], [0, 9] ];
print min(intervals, fun(a,b) { return a[0]<b[0]; } ); // expect [0, 9]

qsort(intervals, 0, len(intervals), fun(a,b){return a[0]<b[0];});
print intervals; // expects [[0, 9, ], [1, 2, ], [3, 4, ], ]
```

## Variables
Unlike JavaScript, every variable must be declared before use by using `var` keyword like:

```javascript
var i = 1;
var a = [1,2,3];
var m = {"ab": 1, "c": true};
var uninitialized; // equivalent to var = uninitialized = nil;
````

You'll notice that there is no types for variables and therefore no type checking at compile time.

## Data Types

Each value in Rhythm falls into one of the following types:
1. Number (both integer and floating point numbers, represented by float64); literals like `3` or `1.8` or `1.1e18`;
2. String (immutable), literals like `"abc"`
3. Nil (null value), literal like `nil`
4. Bool (true/false), literals like `true`, `false`
5. A closure (a function defined in rhythm), literal (lambda) like `fun(a,b){return a+b;}`
6. Array (dynamic array/c++ std::vector), literal like `[1,2,3]`
7. Map (hashtable/c++ std::unordered_map), literal like `{"a": 2, "b": 3}`

Note that there is no equivalent to C struct or C++ class in Rhythm; instead, use `Map` with field name as keys to simulate struct. For example you can use `Map` to make a tree node. Of course no static checking for the shape of struct is available.

### Builtin functions/operators
Between numbers you can use the binary operators `+, -, *, /, %` just like in C or JavaScript.

`-` can also be Unary operator meaning negation of the number.

`!` can negate boolean values.

`and` and `or` are logical binary operations between booleans.

`==` and `!=` means equal or unequal between values, returns boolean `true` or `false`. Note that `==` means value equivalence between concrete data types (Number, String, Bool), not the reference types (Function/Closure, Array, Map).  So `"abc"=="abc"` will be `true`, but `[1,2]==[1,2]` will be false. If you want to compare contents of Array/Map you must write a function that iterates through the content and compare (alternatively, you can approximate this by serializing the array/map and compare the serialized string; see the `to_json` native function).

For `Array`s  we can create literal, get size of it, push to the end, or pop out from the end:

```javascript
var a = [1,2,3,4,5];
print len(a); // expectes 5
push(a, 6);
print a; // expects [1,2,3,4,5,6]
print pop(a); // expects 6
print a; // expects [1,2,3,4,5]
```


For `Map`s we can create literal, assign val to key, test existence of key, or delete the key:

```javascript
var s = "hello";
var i = 42;
var m = { i: s, "hey": "jude" };

print m[42]; // expects "hello"
print m["hey"]; // expects "jude"
print m[m]; // expects nil
print len(m);
```

Value is `nil` is equivalent of saying Key is not present in the map.


`print` is a keyword that prints a variable. To get more precise control over the format of the output, use `printf` function like C.
```javascript
var i = 3;
var f = 1.8e3;
var s = "hello";
var a = [2,3];
var m = {"hey": "jude", 2: true};

printf("i=%d, \nf=%f, \ns=%s, \na=%s, \nm=%s\n", i, f, s, a, m);
//expects
// i=3,
// f=1800.000000,
// s=hello,
// a=[2.000000, 3.000000, ],
// m={hey: jude, 2.000000: true, }
```

### IO functions

Rhythm program does not do any file/network IO except:
1. Reading from standard input (stdin) as text: `readline()`
2. Output to stdout using `print`, `printf`.

`readline()`: reads stdin for newline and return the line just read; it returns empty string when stdin ended. A common use of `readline()` is to read multiple lines from stdin:

```javascript
var cnt = 0;
while(true) {
	var line = readline();
	if (line == false) break; //readline() returns false upon stdin EOF
	cnt = cnt + 1;
	printf("line%d: %s\n", cnt, line);
}
```

Given input file
```
hello
world

fourth line
```

It should print out
```
line1: hello
line2: world
line3:
line4: fourth line
```


## Native functions

| Function                      | Signature                                      | Description                                                                |
|-------------------------------|------------------------------------------------|----------------------------------------------------------------------------|
| `clock()`                     | `clock() -> Number`                            | Returns current time in seconds since epoch                                |
| `printf(format, ...)`         | `printf(String, ...) -> Nil`                   | Prints formatted output to stdout using C-style format specifiers          |
| `sprintf(format, ...)`        | `sprintf(String, ...) -> String`               | Returns formatted string using C-style format specifiers                   |
| `len(container)`              | `len(Array\|Map\|String) -> Number`            | Returns length of array, map, or string                                    |
| `push(array, value)`          | `push(Array, Any) -> Any`                      | Appends value to end of array (modifies array in-place), returns the value |
| `pop(array)`                  | `pop(Array) -> Any`                            | Removes and returns last element from array                                |
| `readline()`                  | `readline() -> String\|Bool`                   | Reads a line from stdin; returns `false` on EOF                            |
| `split(string, delimiter)`    | `split(String, String) -> Array`               | Splits string by delimiter, returns array of substrings                    |
| `assert(condition, message?)` | `assert(Bool, Any?) -> Nil`                    | Throws runtime error if condition is false; optional message               |
| `keys(map)`                   | `keys(Map) -> Array`                           | Returns an array of all keys in the map                                    |
| `tonumber(value)`             | `tonumber(String\|Number\|Bool) -> Number`     | Converts string, number, or boolean to number                              |
| `slurp()`                     | `slurp() -> String`                            | Reads entire stdin content as a single string                              |
| `from_json(string)`           | `from_json(String) -> Any`                     | Parses JSON string into Rhythm values                                      |
| `to_json(value)`              | `to_json(Any) -> String`                       | Serializes Rhythm value to JSON string                                     |
| `floor(x)`                    | `floor(Number) -> Number`                      | Returns largest integer ≤ x                                                |
| `ceil(x)`                     | `ceil(Number) -> Number`                       | Returns smallest integer ≥ x                                               |
| `sin(x)`                      | `sin(Number) -> Number`                        | Returns sine of x (radians)                                                |
| `cos(x)`                      | `cos(Number) -> Number`                        | Returns cosine of x (radians)                                              |
| `tan(x)`                      | `tan(Number) -> Number`                        | Returns tangent of x (radians)                                             |
| `asin(x)`                     | `asin(Number) -> Number`                       | Returns arcsine of x (radians)                                             |
| `acos(x)`                     | `acos(Number) -> Number`                       | Returns arccosine of x (radians)                                           |
| `atan(x)`                     | `atan(Number) -> Number`                       | Returns arctangent of x (radians)                                          |
| `log(x)`                      | `log(Number) -> Number`                        | Returns natural logarithm of x                                             |
| `log10(x)`                    | `log10(Number) -> Number`                      | Returns base-10 logarithm of x                                             |
| `sqrt(x)`                     | `sqrt(Number) -> Number`                       | Returns square root of x                                                   |
| `exp(x)`                      | `exp(Number) -> Number`                        | Returns e raised to power x                                                |
| `fabs(x)`                     | `fabs(Number) -> Number`                       | Returns absolute value of x                                                |
| `pow(x, y)`                   | `pow(Number, Number) -> Number`                | Returns x raised to power y                                                |
| `atan2(y, x)`                 | `atan2(Number, Number) -> Number`              | Returns arctangent of y/x (radians)                                        |
| `fmod(x, y)`                  | `fmod(Number, Number) -> Number`               | Returns floating-point remainder of x/y                                    |
| `random_real(a, b, n)`        | `random_real(Number, Number, Number) -> Array` | Returns array of n random floats between a and b                           |
| `random_int(a, b, n)`         | `random_int(Number, Number, Number) -> Array`  | Returns array of n random integers between a and b (inclusive)             |


## Comments and basic syntax

- Single-line comments start with // and continue to end of line.
- Statements typically end with a semicolon.

```javascript
// This is a comment
var x = 1 + 2; // another comment
```

## Comparisons, concatenation, and truthiness

- Relational operators: <, <=, >, >= work on numbers (and lexicographically on strings).
- String concatenation uses +.

```javascript
print "Hello, " + "world!"; // Hello, world!
print 3 < 5;                // true
```

- Truthiness: only false and nil are falsy. Everything else is truthy (including 0, "", [], {}).
- and and or are short-circuiting and return the last evaluated operand (not coerced to bool).

```javascript
print "hi" or 2;    // "hi"
print nil or "yes"; // "yes"
print nil and 2+3;  // nil
```

## Blocks, scope, and variables

- Blocks are delimited by { ... } and can appear anywhere, not only after control flow keywords.
- Variables are block-scoped and must be declared before use with var.
- Shadowing is allowed in inner blocks; redeclaring the same name in the same block is a compile-time error.
- Using a variable in its own initializer is an error by design.

```javascript
var a = "global";
{
  var a = "outer";
  {
    var a = "inner";
    print a; // inner
  }
  print a; // outer
}
print a; // global
```

Illegal patterns:

```javascript
fun bad() {
  var a = "first";
  var a = "second"; // error: duplicate declaration in the same scope
}

var a = 1;
{
  var a = a + 2; // error: cannot read a in its own initializer
}
```

- All variables (including functions) must be declared before use. Using an undeclared name is a compile-time error.

```javascript
var a = 3;
f(a, 4); // error: f is not declared
```

- return is only allowed inside functions; using it at top level is a parse error.

```javascript
return "at top level"; // parse error
```

## Control flow

- if, while, and for look like C. The for header is (init; condition; update).

```javascript
for (var i = 0; i < 3; i = i + 1) {
  print i;
}
```

- break exits the innermost loop; continue skips to the next iteration. They also work inside nested blocks within a loop.


```javascript
var i = 0;
while (i < 5) {
  i = i + 1;
  if (i == 3) continue;
  if (i == 4) break;
  print i;
}
// prints: 1 2
```

## Functions, arity, and return

- Define with fun name(params) { ... }.
- Functions are first-class: pass as values, return from functions, store in arrays/maps.
- Anonymous functions use the same fun(...) { ... } form.

```javascript
fun add(a, b) { return a + b; }
var mul = fun(a, b) { return a * b; };
print add(3, 4);
print mul(3, 4);
print fun(x) { return x * 2; }(10); // immediately invoked
```

- If a function does not hit a return, it returns nil.

```javascript
var noReturn = fun(x) { print "Processing: " + x; };
print noReturn("test"); // nil
```

- Arity check: calling with the wrong number of arguments is a runtime error.

```javascript
fun f(a, b) { print a + b; }
f(1); // runtime error: wrong number of arguments
```

## Closures and lexical scope

- Rhythm has lexical closures; inner functions capture variables from their enclosing scopes.

```javascript
var x = "outer";
fun makeGreeter() {
  var greeting = "Hello";
  return fun(name) { print greeting + " " + name + " from " + x; };
}
var greet = makeGreeter();
greet("Alice"); // Hello Alice from outer
```

- Captured variables are live and shared by closures (captured by reference).

```javascript
fun makeCounter() {
  var count = 0;
  return fun() { count = count + 1; return count; };
}
var c1 = makeCounter();
var c2 = makeCounter();
print c1(); // 1
print c1(); // 2
print c2(); // 1
```

- Closures in loops capture the same loop variable. If you allocate functions in a loop, they will all see the final value unless you close over a fresh parameter.

```javascript
var fs = [];
for (var i = 0; i < 3; i = i + 1) {
  push(fs, fun() { return i; });
}
print fs[0](); // 3
print fs[1](); // 3
print fs[2](); // 3

fun capture(n) { return fun() { return n; }; }
var fs2 = [];
for (var i = 0; i < 3; i = i + 1) push(fs2, capture(i));
print fs2[0](); // 0
print fs2[1](); // 1
print fs2[2](); // 2
```

- Recursion and mutual recursion are supported.

```javascript
fun fib(n) { if (n < 2) return n; return fib(n-1) + fib(n-2); }
print fib(6); // 8
```

## Arrays

- Arrays are dynamic, mutable, and passed by reference.

```javascript
var a = [];              // empty array
push(a, 1);
push(a, "hello");
print len(a);            // 2
print a[1];              // hello

a = [1,2,3,4,5];
fun sum(xs) {
  var s = 0;
  for (var i = 0; i < len(xs); i = i + 1) s = s + xs[i];
  return s;
}
printf("sum=%d\n", sum(a)); // sum=15
```

- Indexing
  - Indices are 0-based integers.
  - Non-integer index is a runtime error.
  - Out-of-bounds access is a runtime error.
  - Nested indexing works for arrays of arrays.

```javascript
var m = [[1,2], [3,4]];
print m[1][0]; // 3
```

## Maps

- Map literals use { key: value, ... }. Keys can be numbers or strings (and commonly identifiers as keys without quotes).
- Get/set with m[key]. A missing key returns nil.
- Delete by assigning nil to the key.
- Iterate with keys of `m` by using the native function `keys(m)` which returns an array of keys in m. 

```javascript
var i = 42;
var m = { i: "hello", "hey": "jude" };
print m[42];       // hello
print m["hey"];    // jude

m[i] = 7;
m[false] = [1,2,3];      // example of mixed value types
print m;

m[i] = nil;              // delete key 42
printf("size %d\n", len(m));

for (var i=0; i<len(ks); i=i+1) {
  printf("k=%s v=%s\n", ks[i], m[ks[i]]);
}
```

## Property access sugar

For string keys that are valid identifiers, you can use dot access as sugar for m["key"]:

```javascript
var node = { "value": 5, "left": nil, "right": nil };
print node.value;         // 5
node.left = { "value": 3 };
print node.left.value;    // 3
```



## Common errors to expect

- Using a name before it’s declared: compile error.
- Duplicating a var name in the same scope: compile error.
- return outside of a function: parse error.
- Wrong number of function arguments: runtime error.
- Array index not an integer or out of bounds: runtime error.

These rules are shown in examples under rhythm/examples.

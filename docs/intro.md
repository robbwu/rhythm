# Rhythm Language Introduction

Rhythm is a dynamic typed language, meaning that variables do not have a fixed type (values do). Its syntax can be described as a cleaned up JavaScript, with notable differences in a few keywords (`fun` instead of `function`, `and` instead of `&&`, and `or` of `||`). Typically a JavaScript syntax highlighter works well enough for Rhythm code (as shown here).  Rhythm has first class function and lexical closure; no structure or class. It also has builtin dynamic array `Array` and hashtable `Map`.

The `beat` CLI tool is a bytecode compiler and interpreter for the Rhythm language.

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


## Program structure, control flow, ...

Each `*.rhy` file is considered a script for execution. When running with
`beat a.rhy`, the script `a.rhy` will be executed line by line, statement by statement.

Common control flows include `if`, `for`, and `while`, which looks exactly the same
as in C. For example:

```javascript
for (var i=0; i<10; i=i+1) printf("iter %d, "i);
```


## Native functions

| Name | Signature | Returns | Brief description |
|---|---|---|---|
| clock | clock() | Number | Current time in seconds (floating point). |
| printf | printf(fmt, ...args) | nil | Print using C-style formatting. Supports %d (integer), %f (floating), %s (stringified value). |
| sprintf | sprintf(fmt, ...args) | String | Like printf, but returns the formatted string instead of printing. |
| len | len(x) | Number | Length of Array or Map size or String length. |
| push | push(arr, value) | nil | Append value to the end of arr (Array). |
| pop | pop(arr) | Any or nil | Remove and return the last element of arr; returns nil if empty. |
| readline | readline() | String or false | Read one line from stdin (without trailing newline). Returns false on EOF. |
| split | split(str, sep) | Array<String> | Split str by the delimiter sep (string), returning an array of substrings. |
| assert | assert(cond[, msg]) | nil | If cond is false, throws a runtime error with optional message. |
| for_each | for_each(arr, fn) | nil | Call fn(elem) for each element of arr from index 0..len(arr)-1. |
| tonumber | tonumber(x) | Number or nil | Convert x to Number. Numbers pass through; strings are parsed; returns nil if not parseable. |
| slurp | slurp(path) | String | Read entire file at path into a string; throws on IO error. |
| from_json | from_json(jsonStr) | Any | Parse JSON string into Rhythm values (Map, Array, Number, Bool, String, nil). Throws on invalid JSON. |
| to_json | to_json(value) | String | Serialize a Rhythm value to a JSON string. Map keys are converted to strings. |
| floor | floor(x) | Number | Largest integer <= x. |
| ceil | ceil(x) | Number | Smallest integer >= x. |
| sin | sin(x) | Number | Sine of x (radians). |
| cos | cos(x) | Number | Cosine of x (radians). |
| tan | tan(x) | Number | Tangent of x (radians). |
| asin | asin(x) | Number | Arc-sine, result in [-pi/2, pi/2]. |
| acos | acos(x) | Number | Arc-cosine, result in [0, pi]. |
| atan | atan(x) | Number | Arc-tangent, result in (-pi/2, pi/2). |
| log | log(x) | Number | Natural logarithm ln(x). |
| log10 | log10(x) | Number | Base-10 logarithm. |
| sqrt | sqrt(x) | Number | Square root of x. |
| exp | exp(x) | Number | e^x. |
| fabs | fabs(x) | Number | Absolute value. |
| pow | pow(x, y) | Number | x raised to the power y. |
| atan2 | atan2(y, x) | Number | Arc-tangent of y/x using signs to determine the quadrant. |
| fmod | fmod(x, y) | Number | Floating-point remainder of x/y with the sign of x. |
| random_real | random_real(); random_real(hi); random_real(lo, hi) | Number | Uniform real in [0,1) with no args; in [0,hi) with one arg; in [lo,hi) with two args. |
| random_int | random_int(lo, hi) | Number | Uniform integer in the inclusive range [lo, hi]. |

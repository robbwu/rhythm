# Rhythm language

This is a simple and dynamic programming language designed for describing
algorithms precisely and concisely, and of course also executable.
Towards this goal, the Rhythm language has a familiar syntax in the C family
(curly braces, control flows, etc), has builtin data structures like
Array (dynamic array, vector), Map (hashtable), no classes or objects,
and a simple type system.

For the purpose as instruction tool for higher college level algorithms,
the language has the following interesting contrasts with other popular choices:

## Example code

```
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
        //printf("lo %d, hi %d ", lo, hi);
        //printf("A: %s", A);
        var p = partition(A, lo, hi, less);
        //printf(" p %d\n", p);
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

fun disjoint(int1, int2) {
        if (int1[1] < int2[0] or int2[1] < int1[0]) return true;
        return false;
}
// returns an array of classes to take
fun earliest_class_first(intervals) {
        qsort(intervals, 0, len(intervals), fun(a,b){return a[0]<b[0];});
        var res = [];
        for (var i=0; i<len(intervals); i=i+1) {
                if (len(res) == 0)
                        push(res, intervals[i]);
                else if (disjoint(res[len(res)-1], intervals[i]))
                        push(res, intervals[i]);
        }
        return res;
}

var intervals = [ [1,2], [3, 4], [0, 9] ];
print "earliest lass first result";
print earliest_class_first(intervals); // expects [[0, 9],]

fun shortest_class_first(intervals) {
        qsort(intervals, 0, len(intervals), fun(a,b){return (a[1]-a[0])<(b[1]-b[0]);});
        var res = [];
        for (var i=0; i<len(intervals); i=i+1) {
                if (len(res) == 0)
                        push(res, intervals[i]);
                else if (disjoint(res[len(res)-1], intervals[i]))
                        push(res, intervals[i]);
        }
        return res;
}

print "shortest lass first result";
print shortest_class_first(intervals); // expects [[1,2],[3,4]]

fun earliest_finish_class_first(intervals) {
        qsort(intervals, 0, len(intervals), fun(a,b){return a[1]<b[1];});
        var res = [];
        for (var i=0; i<len(intervals); i=i+1) {
                if (len(res) == 0)
                        push(res, intervals[i]);
                else if (disjoint(res[len(res)-1], intervals[i]))
                        push(res, intervals[i]);
        }
        return res;
}
print "earliest finish lass first result";
print earliest_finish_class_first(intervals); // expects [[1,2],[3,4]]

```

## C/C++/Java
Biggest difference is that Rhythm is dynamically typed so more suitable for
algorithm description with less visual clutter. The core data structures are
builtin rather than in a library.

## Python & JavaScript
Python is concise and clean and quite suitable for algorithm description, as
well as being executed. The main appeal of Rhythm in comparison is that: 1)
Rhythm syntax is more familarly based on C family syntax (indents are harder to
see in writing--which is the only feasible way for fair assement which is
proctored written exams). 2) Rhythm is much simpler with barely enough feature
and battery included. This reduces distractions.

JavaScript is also not a bad choice for algorithm description, but it is much
more complicated than what we need; the core syntax and data structures (Buffer,
Typed Arrays, objects as maps ) cannot fit in a single page. Also the language
is messy with a lot of quirks and gotchas, which hinders the communication.

## MMIX (The Art of Computer Programming)
Let's just say MMIX is a bit too low level for algorithm description for the
purpose of a regular college upper level algorithms course.

## Nice features suitable for algorithm course
One of the most useful feature of using a real language (executable) as the
main teaching or instruction tool is that students can run their code,
and the code can in principle be automatically graded by running through
a series of test cases to check correctness and also the *asymptotic complexity*
of the algorithm.

With a compiled language like C/C++/Java, the most apparent solution is to
use the runtime of the code as a proxy for the asymptotic complexity, which is
mostly fine but can be misleading in some cases. For example, the most robust
and popular way to accept/reject a solution in competitive programming is to
set a time limit and raise (TLE--time limit exceeded) error if the solution
is not fast enough. However the the execution time is a function of not only
algorithmic compelexity but also the implementation details, such as the micro
optimization, which is not the focus of the algorithm course.

In Rhythm, the micro optimization is much less of a concern because the language
is simple and compiler does not do significant optimization. The main determinant
of exeuction time is therefore the algorithmic complexity, which is what we want.
Besides, as the bytecode of Rhythm is extremely simple and bare, it's possible to
to have more robust acceptance criteria for algorithmic complexity, such as
total OP_code count, specific OP_code count (comparison, arithmetic, memory access, etc).
This way the students can focus on the algorithmic complexity rather than
progrmaming details and particular language specific quirks.

## Build instructions

### MacOS
Requires a fairly recent C++ compiler (for C++23) from the Xcode or Xcode command line tools. Here's one that works for me

```
% clang++ --version
Apple clang version 15.0.0 (clang-1500.3.9.4)
Target: arm64-apple-darwin23.6.0
Thread model: posix
InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
```

### Linux

Requires a recent C++ compiler; this one is tested to be working:
```
$ c++ --version
c++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
Copyright (C) 2023 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

### Windows (WSL)
Untested, but you should have no problem of building it with a recent toolchain just like linux,
in the Windows Subsystem for Linux (WSL).

# Known Bugs & Limitations

## No `continue` in loops
For now you'll have to manage without it...


## Main inspiration

Crafting Interpreters by Robert Nystrom, the Lox language at
https://craftinginterpreters.com

Lua language, https://www.lua.org

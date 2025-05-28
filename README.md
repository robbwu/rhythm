# Rhythm language

This is a simple and dynamic programming language designed for describing
algorithms precisely and concisely, and of course also executable.
Towards this goal, the Rhythm language has a familiar syntax in the C family
(curly braces, control flows, etc), has builtin data structures like
Array (dynamic array, vector), Map (hashtable), no classes or objects,
and a simple type system.

For the purpose as instruction tool for higher college level algorithms,
the language has the following interesting contrasts with other popular choices:

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
Typed Arrays ) cannot fit in a single page.

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

## Main inspiration

Crafting Interpreters by Robert Nystrom, the Lox language at
https://craftinginterpreters.com

Lua language, https://www.lua.org

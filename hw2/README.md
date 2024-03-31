# Homework 2 Debugging, Fixing, and Optimizing - CSE 320 - Spring 2024
#### Professor Eugene Stark

# Introduction

In this assignment you are tasked with updating an old piece of
software, making sure it compiles and works properly in your VM
environment.  You will also attempt to improve its performance.

Maintaining old code is a chore and an often hated part of software
engineering. It is definitely one of the aspects which are seldom
discussed or thought about by aspiring computer science students.
However, it is prevalent throughout industry and a worthwhile skill to
learn.  Of course, this homework will not give you a remotely
realistic experience in maintaining legacy code or code left behind by
previous engineers but it still provides a small taste of what the
experience may be like.  You are to take on the role of an engineer
whose supervisor has asked you to correct all the errors in the
program, plus make it run faster.

By completing this homework you should become more familiar
with the C programming language and develop an understanding of:
- How to use tools such as `gdb` and `valgrind` for debugging C code.
- Modifying existing C code.
- Execution profiling and optimization.

## The Existing Program

Your goal will be to debug and try to optimize the `dtmf` program,
which is code submitted by a student for HW1 in a previous semester
of this course.  The `dtmf` program is a program whose dual functions
are: (1) to generate audio files containing data encoded as DTMF
(dual-tone, multi-frequency) pulses (like what you hear when you
press the keypad on a telephone), and (2) to read audio files
containing DTMF pulses and possibly other noise, and to decode and
output the corresponding data.
The code you will work on is basically what the student submitted,
except for a few edits that I made to make the assignment more interesting
and educational :wink:.  Although this program was written by one of
the better students in the class, there are a number of things that
could be criticized about the coding style and you might want to think
about what those might be as you work on the code.  You are free to
make any changes you like to the code, and it is not necessary for you
to adhere to the restrictions involving array brackets and the like,
which were imposed on the original code.
(You should not, however, put any functions other than `main` in the
`main.c` file, as this will affect the Criterion test code.)
For this assignment, you may also make changes to the `Makefile`
(*e.g.* to change the options given to `gcc`).
If you find things that make the program very awkward to work on,
you are welcome to rewrite them, but it is not really in the sprit of
the assignment to just rewrite the whole thing (and I am guessing that
you probably don't want to do that anyway).

Because you will need to have some understanding of what the program
is supposed to do, I have included
[the original assignment handout](./README_dtmf.md).
You don't necessarily have to read this thoroughly to get started,
but you will need to refer to it at some point in order to
understand how to fix some of the bugs in the program and maybe to get
an idea of what kind of changes you can make to improve the performance
of the program while preserving its correctness.

### Getting Started - Obtain the Base Code

Here is the structure of the base code:

<pre>
.
├── .gitignore
└── hw2
    ├── include
    │   ├── audio.h
    │   ├── const.h
    │   ├── debug.h
    │   ├── dtmf.h
    │   ├── dtmf_static.h
    │   └── goertzel.h
    ├── Makefile
    ├── rsrc
    │   ├── 941Hz_1sec.au
    │   ├── dtmf_0_500ms.au
    │   ├── dtmf_0_500ms.txt
    │   ├── dtmf_all.au
    │   ├── dtmf_all.txt
    │   └── white_noise_10s.au
    ├── src
    │   ├── audio.c
    │   ├── dtmf.c
    │   ├── goertzel.c
    │   └── main.c
    └── tests
        ├── audio_tests.c
        ├── basecode_tests.c
        ├── blackbox_tests.c
        ├── detect_tests.c
        ├── generate_tests.c
        ├── goertzel_tests.c
        ├── ref_audio.o
        ├── ref_goertzel.o
        ├── rsrc
        │   ├── dtmf_0_500ms.au
        │   ├── dtmf_0_500ms.txt
        │   └── truncated_header.au
        ├── test_common.c
        ├── test_common.d
        ├── test_common.h
        └── validargs_tests.c
</pre>

As usual, the `include` directory contains header files, the `src` directory contains
`.c` source files, and the `Makefile` tells the `make` utility how to build the program. The `rsrc`
directory contains a few audio and text input files for the program, which were distributed
with the original basecode.
The `tests` directory contains the test code that was used in grading this assignment,
and the `tests/rsrc` directory contains some inputs used by the test code.
Some of the tests compare the result produced by the program with that produced by a
reference version -- the reference code is given in binary form and is contained in the
files `ref_audio.o` and `ref_goertzel.o`.

# Part 1: Debugging and Fixing

Complete the following steps:

1. Check that the code compiles `out of the box` (it should).
   For this assignment, I have provided code that does not need any further
   tweaking to make it compile on our systems.

2. Fix bugs.  The program as distributed does not function correctly.
   You can use the Criterion test cases provided to show you the bugs.
   To get started, run `bin/dtmf_tests --verbose -j1`.
   Choose one of the test cases that fails and debug it until you understand
   how to fix it.  "Rinse and repeat" until all the test cases pass.
   You would be well advised to make use of `git` to keep track of what
   you have done to the program, in case you find out that you "fixed"
   something incorrectly and you need to backtrack.

3. Use `valgrind` to identify any memory access errors.  Fix any errors you find.

Run `valgrind` using the following command:

	    valgrind bin/dtmf [ARGS TO DTMF]

>**Hint:** You will probably find it very useful to use valgrind to find some
>of the bugs in step 2, so you may want to try it out before getting too involved
>in step 2.

> :scream: You are **NOT** allowed to share or post on PIAZZA
> solutions to the bugs in this program, as this defeats the point of
> the assignment. You may provide small hints in the right direction,
> but nothing more.

# Part 2: Optimizing

For this part, your objective is to improve the execution speed of the program
as much as you can manage.  Some of the points assigned during grading will
depend on how much faster your version of the program works than the original
version.  You should use the following strategy in trying to speed up the
program:

1.  Use `gprof` to create an execution profile of the program.  The use of
	`gprof` has been discussed in class.  A separate target, `prof` has been
    included in the `Makefile` for building the program with profiling enabled.
    Using `make clean prof` will rebuild the entire program for profiling.

2.  Use techniques discussed in class and in the textbook to improve the
	execution speed of the program.  For example, one thing you should do
	is to use the execution profile to identify "hot spots" in the code
    that might benefit from optimization and try to rewrite the code so that
	the program runs faster.

3.  Use `git` to maintain a history of changes that you made and the
	`gprof` profiles of the associated program versions.  It is a good idea
	to do this because not everything you try is going to speed up the
	program and sometimes you will make it worse.
	It is not necessary for this experimental history to be part of what
	you submit.  I would suggest using a separate branch or branches for
	this purpose.  Once you have identified the changes that produce the
	most improvement, you can merge or cherry pick them back into the
	master branch before submitting your work.

>**Note:** You will likely find that the sample audio files provided with the
> test cases are too small to provide reliably repeatable results using a sampling-based
> profiler such as `gprof`.  Once you get the program working, you can use it
> in "generate" mode to produce longer test files.  I suggest generating a
> test file containing audio data of an hour or more.  Such a file will consist
> of roughly 30M samples at 8000KHz sampling rate and will be about 60MB in size.
> On my system, this was long enough for initial testing, but once I started to
> improve the performance of the program I found that an even longer test file
> would be desirable.
> To create the test file, I wrote a short program to generate a long sequence
> of random "events" in the input format expected by `dtmf -g`, and then I ran
> `dtmf` in "generate mode" on this input to produce the test audio file.

> :scream: **Please do not commit any large test files to your repository!!!**

# Unit Testing

For this assignment, we have provided the set of Criterion tests that
were used to grade this program in the semester in which it was originally
assigned.  You will want to use these tests to help you identify and
correct bugs in the handout version of the program.  We encourage you
also to look at how these tests were constructed, so that you can use
similar ideas in later assignments to test your own code.
Many of tests are true "unit tests", which test individual
functions in the program.  Other tests are "black box tests", which
test the behavior of the program as a whole.  Although Criterion is designed
to a tool for unit testing, we have also used it to create black box tests
as well.  In unit testing, each test case directly calls functions of the
program under test.  In black box testing, each test case launches an
instance of the program as a separate process, after arranging to feed
it input and capture its output for analysis.
We use the Linux `system()` function to launch an instance of the program
for testing.

Criterion provides for test cases to be organized into *suites* of
related tests.  In the `tests` directory, the files
`audio_tests.c`, `basecode_tests.c`, `blackbox_tests.c`, `generate_tests.c`,
`goertzel_tests.c`, and `validargs_tests.c` each contain a suite of tests.
The files `test_common.c` and `test_common.h` contain utility functions
that are shared by the various test suites.
Each test has one or more *assertions* that check whether the code is
functioning properly.  The failure of an assertion causes the unsuccessful
termination of a test.  Other failures during execution, such as a seg-fault,
will also cause the unsuccessful termination of the test.
A test succeeds if it runs to the end without any failures.

Tests are invoked by running: `bin/dtmf_tests`.  By default, Criterion
runs multiple tests in parallel (as separate processes) and only reports
on failures.  This is normally the desired behavior -- you run tests
to make sure that changes to the program have not caused tests that
previously passed to now fail, and you don't want to be distracted by
a lot of chit-chat.  On the other hand, when using the tests to try to
debug the program, it is probably useful to get somewhat more verbose
output.  This can be done using `bin/dtmf_tests --verbose`.
In addition, when tests are run in parallel, the results can appear
in different orders in different runs, leading to confusion.
You can cause the tests to be run sequentially by adding the `-j1`
option (run the tests as just one "job").  In some cases, the `-j1`
option may be essential if the tests were not written with suitable
attention paid to the possibility that they would run concurrently.
For example, if two different tests write output to the same output file,
then they will likely not work properly when run concurrently.

You may write more of your own tests if you wish.  Criterion documentation
for writing your own tests can be found
[here](http://criterion.readthedocs.io/en/master/).

Besides running Criterion unit tests, you should also test the final
program that you submit with `valgrind`, to verify that no memory
access errors are found.
Note that the `dtmf` program does not use dynamic storage allocation,
so we won't need to take advantage of the ability of `valgrind` to
identify memory leaks for this assignment.  However, you will likely
want to know about this for future assignments.

# Hand-in Instructions

Ensure that all files you expect to be on your remote
repository are pushed prior to submission.

This homework's tag is: `hw2`

	    $ git submit hw2

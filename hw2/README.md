# Homework 2 Debugging and Fixing - CSE 320 - Fall 2023
#### Professor Eugene Stark

### **Due Date: Friday 10/6/2023 @ 11:59pm**

# Introduction

In this assignment you are tasked with updating an old piece of
software, making sure it compiles, and works properly in your VM
environment.

Maintaining old code is a chore and an often hated part of software
engineering. It is definitely one of the aspects which are seldom
discussed or thought about by aspiring computer science students.
However, it is prevalent throughout industry and a worthwhile skill to
learn.  Of course, this homework will not give you a remotely
realistic experience in maintaining legacy code or code left behind by
previous engineers but it still provides a small taste of what the
experience may be like.  You are to take on the role of an engineer
whose supervisor has asked you to correct all the errors in the
program, plus add additional functionality.

By completing this homework you should become more familiar
with the C programming language and develop an understanding of:
- How to use tools such as `gdb` and `valgrind` for debugging C code.
- Modifying existing C code.
- C memory management and pointers.
- Working with files and the C standard I/O library.

## The Existing Program

Your goal will be to debug and extend the `grades` program.
I (Prof. Stark) wrote this particular program in 1991 for my own use,
and have continued to use it to the present day.  The original version
ran under SunOS and (as far as I recall, some version of MS-DOS or perhaps
Windows 3.1).  I have made only minimal changes to it over the years,
to get it to function on other systems, including FreeBSD and Ubuntu.
As the code was written a long time ago, only a short time after the ANSI C
standard was adopted and before I personally had much experience with it,
the code contains some pre-ANSI features: most notably, old-style function
headers instead of ANSI C function prototypes.
To be completely honest about the matter, the version of the program I am
handing out is not exactly the same as the one that I use, since I have
made a few changes for this assignment :wink:.  First of all, I introduced
a few bugs here and there to make things more interesting and educational
for you.  Aside from the introduced bugs, the only source file that is
substantially different from the original is `orig_main.c`, which I have
renamed from `main.c` (so that we can use Criterion) and rewritten to use
GNU `getopt` for argument processing where it previously did not.

### Getting Started - Obtain the Base Code

Fetch base code for `hw2` as you did for the previous assignments.
You can find it at this link:
[https://gitlab02.cs.stonybrook.edu/cse320/hw2](https://gitlab02.cs.stonybrook.edu/cse320/hw2).

Once again, to avoid a merge conflict with respect to the file `.gitlab-ci.yml`,
use the following command to merge the commits:


	    git merge -m "Merging HW2_CODE" HW2_CODE/master --strategy-option=theirs


Here is the structure of the base code:

<pre>
.
├── .gitlab-ci.yml
└── hw2
    ├── .gitignore
    ├── hw2.sublime-project
    ├── include
    │   ├── allocate.h
    │   ├── debug.h
    │   ├── global.h
    │   ├── gradedb.h
    │   ├── normal.h
    │   ├── read.h
    │   ├── sort.h
    │   ├── stats.h
    │   ├── version.h
    │   └── write.h
    ├── Makefile
    ├── rsrc
    │   ├── cse307a.dat
    │   ├── cse307b.dat
    │   ├── cse307c.dat
    │   ├── cse307.collated
    │   ├── cse307.dat
    │   └── cse307.tabsep
    ├── src
    │   ├── allocate.c
    │   ├── error.c
    │   ├── main.c
    │   ├── normal.c
    │   ├── orig_main.c
    │   ├── read.c
    │   ├── report.c
    │   ├── sort.c
    │   ├── stats.c
    │   └── write.c
    ├── test_output
    │   └── .git-keep
    └── tests
        ├── basecode_tests.c
        ├── rsrc
        │   ├── blackbox_help.err
        │   ├── collate.in -> cse307.dat
        │   ├── collate.out
        │   ├── cse307a.dat
        │   ├── cse307b.dat
        │   ├── cse307c.dat
        │   ├── cse307.dat
        │   ├── tabsep.in -> cse307.dat
        │   └── tabsep.out
        ├── test_common.c
        └── test_common.h
</pre>

Before you begin work on this assignment, you should read the rest of this
document.  In addition, we additionally advise you to read the
[Debugging Document](DebuggingRef.md).

# Part 1: Debugging and Fixing

The command line arguments and expected operation of the program are described
by the following "usage" message, which is printed by the function `usage()`
in `orig_main.c`:

<pre>
Usage: bin/grades [options] <data file>
Valid options are:
	-r, --report                 	Process input data and produce specified reports.
	-c, --collate                	Collate input data and dump to standard output.
	    --freqs                  	Print frequency tables.
	    --quants                 	Print quantile information.
	    --summaries              	Print quantile summaries.
	    --stats                  	Print means and standard deviations.
	    --comps                  	Print students' composite scores.
	    --indivs                 	Print students' individual scores.
	    --histos                 	Print histograms of assignment scores.
	    --tabsep                 	Print tab-separated table of student scores.
	-a, --all                    	Print all reports.
	-k, --sortby     &lt;key&gt;          Sort by {name, id, score}.
	-n, --nonames                	Suppress printing of students' names.
</pre>

The `--reports` and `--collate` options are mutually exclusive positional arguments
that must appear first.  The remaining (non-positional) options may appear in any
order.  Following the options is a single final argument that specifies the name of
a data file to be read as input.

Option processing in `main()` is performed with the help of the GNU `getopt` library.
This library supports a flexible syntax for command-line arguments, including support
for traditional single-character options (prefixed by '-') and "long-form" options
(prefixed by '--'), which need not be single characters.
The library also takes care of some of the "grunt work" in parsing option arguments
and producing error messages.
You will probably need to read the Linux "man page" on the `getopt` package.
This can be accessed via the command `man 3 getopt`.  If you need further information,
search for "GNU getopt documentation" on the Web.

> :scream: You MUST use the `getopt_long()` function to process the command line
> arguments passed to the program, as in the base code. Your program should be able to
> handle cases where the (non-positional) flags are passed IN ANY order.  This does not
> apply to positional arguments.

You can modify anything you want in the assignment, except for the file `main.c`,
which has been introduced for compatibility with the Criterion library.

Complete the following steps:

1. Clean up the code; fixing any compilation issues, so that it compiles
   without error using the compiler options that have been set for you in
   the `Makefile`.

>**Note:** To fix the compilation errors related to the functions defined in `error.c`,
>you will need to make them into "variadic" functions, which take a variable
>number of arguments.  Read about "variable length argument lists" in K&amp;R 7.3
>or elsewhere, and read the man pages for `stdarg` (`man 3 stdarg`) and
>`vfprintf()` (`man 3 vfprintf`).  Using what you have learned, rewrite the
>functions in `error.c` to have variable-length argument lists, and arrange
>for all arguments after the first to be passed as a `va_list` to `vfprintf()`.

2. Fix bugs.  To get started, try running `bin/grades -r rsrc/cse307.dat`.
   The program will crash.  Track down and fix the problem.
   Repeat until the program runs without crashing.
   Check the functionality of various option settings.  You should
   also use the provided Criterion unit tests to help point the way.

3. Use `valgrind` to identify any memory leaks or other memory access
   errors.  Fix any errors you find.

Run `valgrind` using the following command:

	    valgrind --leak-check=full --show-leak-kinds=all [GRADES PROGRAM AND ARGS]

>**Hint:** You will probably find it very useful to use valgrind to find some
>of the bugs in step 2, so you may want to try it out before getting too involved
>in step 2.

> :scream: You are **NOT** allowed to share or post on PIAZZA
> solutions to the bugs in this program, as this defeats the point of
> the assignment. You may provide small hints in the right direction,
> but nothing more.

# Part 2: Adding Features

Add the following additional features to complete the program's functionality:

- Traditional single-character options:  The table of options in `orig_main.c`
  specifies single-character forms for some of the options, but you will find
  that the program does not recognize them.  Correct that omission.

> :nerd: A good program ideally does not contain multiple definitions that have
> to be kept consistent with each other, as this makes maintenance more difficult.
> You should add the single-character option functionality in such a
> way that (as in the base code) all the information about the options is contained
> in the `options_table` array, and there is no information anywhere else that has
> to be kept consistent with it.

- New option: `--output <outfile>`:  Add to the program an additional non-positional
  option `--output` (short form `-o`) which takes a required filename as an argument.
  When this option is given, output that the program would normally print to the
  standard output should now be sent to the specified file.
  Make sure that the "Usage" message reflects the addition of the new option.

# Unit Testing

For this assignment, you have been provided with a basic set of
Criterion unit tests to help you debug the program.  We encourage you
to write your own as well as it can help to quickly test inputs to and
outputs from functions in isolation.

In the `tests/grades_tests.c` file, there are six unit test examples.
You can run these with the `bin/grades_tests` command.  Each test is
(as much as is feasible) for a separate part of the program.
For example, the first test case just tests the `readfile()` function.
Each test has one or more assertions to make sure that the code functions
properly.  If there was a problem before an assertion, such as a SEGFAULT,
the unit test will print the error to the screen and continue to run the
rest of the tests.

To obtain more information about each test run, you can use the
verbose print option: `bin/grades_tests --verbose=1`.

You may write more of your own tests if you wish.  Criterion documentation
for writing your own tests can be found
[here](http://criterion.readthedocs.io/en/master/).

Besides running Criterion unit tests, you should also test the final
program that you submit with `valgrind`, to verify that no leaks or memory
access errors are found.

>**Note:** It is only necessary to fix "definitely lost" and "possibly lost"
memory leaks.  Valgrind will also issue "still reachable" reports about
dynamically allocated memory that has not been freed, although it is still
accessible, at the time of program termination.  This type of unfreed memory,
though perhaps representative of an "untidy" approach to implementation,
does not generally have any negative impact on program execution.

# Hand-in Instructions

Ensure that all files you expect to be on your remote
repository are pushed prior to submission.

This homework's tag is: `hw2`

	    $ git submit hw2


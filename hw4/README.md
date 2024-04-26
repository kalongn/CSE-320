# Homework 4 "Daemon" Manager - CSE 320 - Spring 2024
#### Professor Eugene Stark

## Introduction

The goal of this assignment is to become familiar with low-level Unix/POSIX system
calls related to processes, signal handling, files, and I/O redirection.
You will implement a program, called `legion`, which manages a collection of
"daemon" processes that implement various kinds of services.

### Takeaways

After completing this assignment, you should:

* Understand process execution: forking, executing, and reaping.
* Understand signal handling.
* Understand how to implement I/O redirection.
* Have gained experience with C libraries and system calls.
* Have enhanced your C programming abilities.

## Hints and Tips

* We **strongly recommend** that you check the return codes of **all** system calls
  and library functions.  This will help you catch errors.
* **BEAT UP YOUR OWN CODE!** Exercise your code thoroughly with various numbers of
  processes and timing situations, to make sure that no sequence of events can occur
  that can crash the program.
* Your code should **NEVER** crash, and we will deduct points every time your
  program crashes during grading.  Especially make sure that you have avoided
  race conditions involving process termination and reaping that might result
  in "flaky" behavior.  If you notice odd behavior you don't understand: **INVESTIGATE**.
* You should use the `debug` macro provided to you in the base code.
  That way, when your program is compiled without `-DDEBUG`, all of your debugging
  output will vanish, preventing you from losing points due to superfluous output.

> :nerd: When writing your program, try to comment as much as possible and stay
> consistent with code formatting.  Keep your code organized, and don't be afraid
> to introduce new source files if/when appropriate.

### Reading Man Pages

This assignment will involve the use of many system calls and library functions
that you probably haven't used before.
As such, it is imperative that you become comfortable looking up function
specifications using the `man` command.

The `man` command stands for "manual" and takes the name of a function or command
(programs) as an argument.
For example, if I didn't know how the `fork(2)` system call worked, I would type
`man fork` into my terminal.
This would bring up the manual for the `fork(2)` system call.

> :nerd: Navigating through a man page once it is open can be weird if you're not
> familiar with these types of applications.
> To scroll up and down, you simply use the **up arrow key** and **down arrow key**
> or **j** and **k**, respectively.
> To exit the page, simply type **q**.
> That having been said, long `man` pages may look like a wall of text.
> So it's useful to be able to search through a page.
> This can be done by typing the **/** key, followed by your search phrase,
> and then hitting **enter**.
> Note that man pages are displayed with a program known as `less`.
> For more information about navigating the `man` pages with `less`,
> run `man less` in your terminal.

Now, you may have noticed the `2` in `fork(2)`.
This indicates the section in which the `man` page for `fork(2)` resides.
Here is a list of the `man` page sections and what they are for.

| Section          | Contents                                |
| ----------------:|:--------------------------------------- |
| 1                | User Commands (Programs)                |
| 2                | System Calls                            |
| 3                | C Library Functions                     |
| 4                | Devices and Special Files               |
| 5                | File Formats and Conventions            |
| 6                | Games, et al                            |
| 7                | Miscellanea                             |
| 8                | System Administration Tools and Daemons |

From the table above, we can see that `fork(2)` belongs to the system call section
of the `man` pages.
This is important because there are functions like `printf` which have multiple
entries in different sections of the `man` pages.
If you type `man printf` into your terminal, the `man` program will start looking
for that name starting from section 1.
If it can't find it, it'll go to section 2, then section 3 and so on.
However, there is actually a Bash user command called `printf`, so instead of getting
the `man` page for the `printf(3)` function which is located in `stdio.h`,
we get the `man` page for the Bash user command `printf(1)`.
If you specifically wanted the function from section 3 of the `man` pages,
you would enter `man 3 printf` into your terminal.

> :scream: Remember this: **`man` pages are your bread and butter**.
> Without them, you will have a very difficult time with this assignment.

### Development and Test Strategy

You will probably find it the most efficient approach to this assignment to write and
test your code incrementally, a little bit at a time, to develop your understanding and
verify that things are working as expected.
You will probably it overwhelmingly difficult to debug your code if you try to write
a lot of it first without trying it out little by little.
Putting some effort into creating useful, understandable, debugging trace output will
also be very helpful.

## Getting Started

Fetch and merge the base code for `hw4` as described in `hw1`.
You can find it at this link: https://gitlab02.cs.stonybrook.edu/cse320/hw4

Here is the structure of the base code:
<pre>
.
├── .gitlab-ci.yml
└── hw4
    ├── daemons
    │   ├── files
    │   ├── lazy
    │   ├── polly
    │   ├── systat
    │   └── yoyo
    ├── demo
    │   └── legion
    ├── .gitignore
    ├── hw4.sublime-project
    ├── include
    │   ├── debug.h
    │   └── legion.h
    ├── lib
    │   └── sf_event.o
    ├── logs
    │   └── .git-keep
    ├── Makefile
    ├── src
    │   ├── cli.c
    │   └── main.c
    └── tests
        ├── basecode_tests.c
        ├── driver.c
        ├── driver.h
        ├── __helper.c
        ├── __helper.h
        ├── tracker.c
        └── tracker.h
</pre>
The `lib` directory contains an object file `sf_event.o` which will be linked
with your program.  This object file provides "event functions" which you must
call in order to enable us during grading to track what your program is doing.
This is explained in further detail a bit later.

The `daemons` directory contains executables for some "daemon" programs for
`legion` to manage.  These are also explained later on.

The `demo` directory contains a demonstration executable of a fully implemented
version of `legion` that should hopefully conform to the specifications laid
out in this document and in the `legion.h` header file.  This executable is
intended simply as a guide to help you to understand better what you are to do.
If there is any conflict between the behavior of the demonstration program and
the written specifications, the written specifications take priority.

If you run `make`, the code should compile correctly, resulting (as usual)
in two executables `bin/legion` and `bin/legion_tests`.
If you run `bin/legion`, it will return without doing very much because the
stub provided for the `run_cli()` function only calls the event functions
`sf_init()` and `sf_fini()` and then returns.
You will need to implement this function.
The tests in `bin/legion_tests` will also fail for similar reasons.

The test code provided in `basecode_tests.c` demonstrates a test driver that
can run your program without your having to always enter input manually.
A similar test driver will be used for grading your program.
The faster speed at which the commands are issued by the test drivers will
be better at exercising race conditions that your program might have,
than manual testing would be.
The tests provided are table-driven "send/expect scripts".  At each step in
the test, there is a possibility of sending a command to your program
and/or checking that some event has occurred.  If the test script progresses
all the way to the end without going wrong in some way, then the test passes,
otherwise it fails.  If you are able to understand the test scripts enough,
you might be able to make a few of your own.  Otherwise, just use them as
a guide to get started and use whatever techniques you can devise yourself
to test your program.

## `Legion`: Functional Specification

### Overview

In the `daemons` directory you will find executables for the following "daemons":

* `lazy` - This daemon does nothing except print a message to its log when
  it starts up and another message when it shuts down.  The rest of the time
  it sleeps.

* `yoyo` - This daemon is like `lazy`, except that it will either exit or
  crash after a random interval of time.  It also sometimes fails to synchronize
  properly with `legion` during startup.  (More on what this means a bit later on.)

* `systat` - This daemon, once started, prints a message to its log every ten
  seconds that contains information about the system uptime and load average.
  The information is obtained by running the `uptime` command.

* `polly` - This daemon is a network server which serves connections on TCP port 10000.
  When a client connects, the server reads a single line of input sent
  by the client, "parrots" that input back to the client, and then closes
  the connection.  Information about the connections and input received from
  clients is written to its log.

* `files` - This daemon is a network server which serves connections on TCP port 10001.
  Like `polly`, it reads a single line of input from a client that has connected,
  but in the case of `files` it attempts to interpret the line of input as the
  name of a file and it sends the content of that file back to the client before
  closing the network connection.
  
Each of the daemons can be run from the command line.  They ignore `stdin`
and output log messages to `stdout`.  For example, the output of `systat` looks
like this:

```
$ daemons/systat
1603128551.397547: SYSTAT: Starting up (pid = 26234)
1603128551.397594: SYSTAT: [ARGV] daemons/systat
1603128551.400307: SYSTAT:  13:29:11 up 3 days, 18:37,  1 user,  load average: 4.20, 2.97, 2.91
1603128561.413578: SYSTAT:  13:29:21 up 3 days, 18:37,  1 user,  load average: 4.24, 3.02, 2.93
1603128571.415329: SYSTAT:  13:29:31 up 3 days, 18:37,  1 user,  load average: 4.29, 3.07, 2.94
1603128581.416968: SYSTAT:  13:29:41 up 3 days, 18:37,  1 user,  load average: 3.94, 3.03, 2.93
1603128582.672891: SYSTAT: Shutting down
$ 
```

Connections to the `polly` and `files` daemons can be made using the `telnet` command.
For example, once you have started `daemons/polly`, you can connect to it by running
the following in another terminal window:

```
$ telnet localhost 10000
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
Hello Polly!
Hello Polly! -- AWWK!!!
Connection closed by foreign host.
$
```

The corresponding log output from `polly` looks as follows:

```
$ daemons/polly
1603128617.320617: POLLY: Starting up (pid = 26257)
1603128617.320662: POLLY: [ARGV] daemons/polly
1603128617.320681: POLLY: Listening on port 10000
1603128624.238847: POLLY: Accepted connection (fd = 4)
1603128628.872055: POLLY: <= Hello Polly!
1603128628.872093: POLLY: Closed connection (fd = 4)
^C
$
```

If for some reason you are stuck in the `telnet` program, you can get out by typing
the escape character `^]` (that's `CTRL+]`) and then typing `q`.

In the above, the "daemons" were started directly from the shell.  However this is not
the normal use case.  The idea is that the daemons will be managed by your "daemon manager": `legion`.
When started, `legion` will present a command-line interface that allows you to register,
unregister, start, and stop daemons, get the status of an individual daemon or of all
the currently registered daemons, and to cause the log files for a daemon to be "rotated",
to prevent individual log files from becoming arbitrarily large over time.
The overall idea is similar to what the `systemctl` command does on a Linux system,
though `legion` will not work in quite the same way.

In order to perform its functions, `legion` will need to be able to launch daemon programs
in child processes whose standard output has been redirected to the appropriate log file,
to cause daemons to terminate by sending them signals, and to perform signal handling to keep
track of the status of all the currently running daemons.

### Detailed Specification

As discussed above, you are to implement a program called `legion`, which when started,
presents a command-line interface to the user.  The `legion` program itself takes no
command-line arguments.
When ready to accept a command, `legion` will issue a prompt '`legion> `'
(with a single terminating space and no newline).
It will then read a single line of input from the user and split that input into fields,
where each field consists of non-space characters delimited either by space characters
(`' '`, ASCII 0x20) or the beginning or end of the line.
Single quote characters (`'`) are treated specially: when such a character is encountered,
all the subsequent characters up to the next single quote (or the end of the line, if there
is no other single quote character) are treated as a single field, regardless of whether
any of those characters are spaces.  The quote characters themselves do not become part of the field.
This mechanism provides a way of creating fields that have spaces in them, which would
otherwise not be possible.

Once the line has been split into fields, the first field is treated as the name of a
command executable and the remaining fields are treated as arguments, just as would be done
by a shell.  `Legion` will check that the command is valid and that an appropriate number
of arguments have been provided, and then it will execute that command.  The commands that
`legion` understands are as follows:

* `help` (no arguments) - Prints a help message that lists the known commands and the
  number of arguments each takes, as in the following:

	```
	legion> help
	Available commands:
	help (0 args) Print this help message
	quit (0 args) Quit the program
	register (0 args) Register a daemon
	unregister (1 args) Unregister a daemon
	status (1 args) Show the status of a daemon
	status-all (0 args) Show the status of all daemons
	start (1 args) Start a daemon
	stop (1 args) Stop a daemon
	logrotate (1 args) Rotate log files for a daemon
	```

* `quit` (no arguments) - Exits the program, after first ensuring that any active daemons
  have been terminated (more later on this).

* `register` (two or more arguments) - Registers the name of a daemon and a command
  to be executed when the daemon is started.  The first argument is the name of a daemon,
  which can be arbitrarily chosen by the user.  The second argument is the name of an
  executable to run when the daemon is started.  Any remaining arguments become part of
  the argument vector passed when the executable is run.

* `unregister` (one argument) - Unregisters a daemon previously registered under the specified
  name.  If no such daemon has been registered, or if daemon has been registered, but it is not
  in the `inactive` state, then it is an error.
  
* `status` (one argument) - Prints the current status of the daemon registered under the
  specified name, in a **tab-separated** format as follows (recall that the `TAB` character
  has ASCII code 0x9 and can be represented in a C string literal by `'\t'`):

	```
	polly	2408	active
	```

    The first field is the name under which the daemon has been registered.
    The second field is either the process ID of the currently running daemon process,
    if the daemon is currently active, or zero if the daemon is not currently active.
    The third field is the current status of the daemon.  The possible status values
    are:

       * `unknown`, if the specified name is not the name of a currently registered daemon,
	   * `inactive`, if the name has been registered, but no daemon process has been started,
       * `starting`, if the daemon is being started but has not yet synchronized with
		 its parent to indicate that it is fully up (see below).
	   * `active`, if the daemon process has been started and is currently running.
	   * `stopping`, if we have sent the daemon a `SIGTERM` signal to request its termination,
          but we have not yet been notified that termination has occurred.
	   * `exited`, if the daemon process has terminated by calling `exit()`.
	   * `crashed`, if the daemon process has terminated abruptly as a result of a signal.

* `status-all` (no arguments) - Prints the current status of all registered daemons,
  one per line.

* `start` (one argument) - Start the daemon that has been registered under the specified name.
  If the current status of the daemon is anything other than `inactive`, it is an error.
  A daemon is started by setting the status of the daemon to `starting`, creating a pipe
  (see `pipe(2)`) which will be used to receive a startup synchronization message from the daemon,
  forking a child process to run the daemon command, arranging for the standard output of the
  child process to be redirected (see `dup2(2)` and `freopen(3)`) to the appropriate log file
  (see below), then having the child process use `execvpe(3)` to execute the command registered
  for the daemon.  The environment passed to `execvpe()` should be the environment
  of `legion` (see `environ(7)`), except the value of the `PATH` environment variable should
  be modified (see `getenv(3)`, `putenv(3)` and `setenv(3)`) by prepending the value of the
 `DAEMONS_DIR` preprocessor symbol (note that a colon `:` is used to separate the prepended
  directory name from the existing list).
  This modification will cause the `DAEMONS_DIR` directory to be tried first to find the
  executable for the daemon.  If an executable is not found in that directory, then the normal
  system search path (which was passed to `legion` in the `PATH` environment variable when it
  started) will be searched.
  
    Once the child process has started, before calling `execvpe()` it should use `setpgid(2)`
    to create and join a new process group, so that it will not receive any signals
    (such as `SIGINT`, `SIGQUIT`, or `SIGTSTP`) generated from the terminal.
	It should also redirect (see `dup2(2)`) the output side of the pipe that was created before
    the fork to file descriptor `SYNC_FD`.
	The daemon executables that have been provided for you will, upon startup, write a
	one-byte synchronization message onto the pipe where it can be read in the parent process.

    If forking the child process is successful, then the parent process will attempt
	to read the one-byte synchronization message from child on the pipe that was set up for
    this purpose.  The reason for this is to cause the parent process to wait until it has
    heard from the child before setting its status to `active` and returning back to the
	command prompt.  This will prevent, for example, the parent process from attempting
	to shut down the child process by sending a `SIGTERM` signal to it before the child process
	has had a chance to install a handler for that signal.
	The synchronization message simply consists of a single byte of data (the value is
	unimportant) which can be read from the input side of the pipe using the
	`read(2)` system call.  Once the parent has successfully read from the pipe, it should
	set the status of the daemon to `active`, close the pipe file descriptor, and go back
	to the command prompt.

	In order to avoid having the parent process hang forever waiting for synchronization
	that might not be forthcoming from a child, the parent process should set a timer
	(see `alarm(2)` or `setitimer(2)`) for an interval of `CHILD_TIMEOUT` seconds.
	Should synchronization not be received from the child in this amount of time,
	a `SIGALRM` signal will be received by the parent.  A handler for `SIGALRM` in the
	parent should send a `SIGKILL` to the child to make sure that it is dead and then
	the parent should return to the command prompt.

	While starting the child process to run a daemon, appropriate signal masking
	(see `sigprocmask(2)`) should be used in the parent process to ensure that it is
	impossible to receive notification about the termination of the child process before
	it has been set to the `active` state.
    If forking the child process fails, then the status of the daemon should be reset to
	`inactive` and an error should be returned from the `start` command.

* `stop` (one argument) - Attempt to stop the specified daemon.  If the current status of
  the daemon is `exited` or `crashed`, then it is simply set to `inactive`.
  Otherwise, if the current status of the daemon is anything other than `active`, it is an error.
  Stopping an active daemon is achieved as follows:  First, the daemon is set to the `stopping`
  state and a `SIGTERM` is sent to the daemon process.
  `Legion` then pauses (see `sigsuspend(2)`) waiting for a `SIGCHLD` signal that indicates the
  daemon process has terminated.  At the same time as the `SIGTERM` is sent,
  a timer is set for `CHILD_TIMEOUT` seconds (see the man page for `alarm(2)` or `setitimer(2)`).
  If the timer expires before a notification is received (via `SIGCHLD` and subsequent `waitpid(2)`)
  that the daemon process has terminated, then a `SIGKILL` is sent to the daemon process.
  Once `SIGKILL` has been sent, `legion` does not wait any more, but insteads reports that the `stop`
  command has encountered an error and returns to the command-line prompt.

* `logrotate` (one argument) - "Rotate" the log files for the specified daemon.  Log files
  are stored in the directory whose name is the value of the `LOGFILE_DIR` preprocessor symbol
  `Legion` must create (see `mkdir(2)`) this directory if does not already exist.
  Logfiles have names that follow the format `%s/%s.log.%c`, where the first string is the value
  of the preprocessor symbol `LOGFILE_DIR`, the second string is the name under which the
  daemon is registered, and the final character is a one-character version indicator,
  which can range from '0' to '9'.  The maximum number of versions (ten or less) is given
  by the preprocessor symbol `LOG_VERSIONS`.  For example, if the value of `LOG_VERSIONS` is 7,
  then the valid versions are '0', '1', ..., '6'.)

    When started, a daemon begins writing (in append mode) to the log file with version '0',
    creating it if it does not exist.  Logfile rotation consists of deleting (see `unlink(2)`)
    any log file with the maximum version number, then renaming (see `rename(2)`) each log
    file to have a version one greater than it did before.  Thus, version '0' becomes version '1',
    version '1' becomes version '2', *etc.*.  Since an active daemon will originally have had
    the version '0' log file open, but this has now become the version '1' log file,
    we need to do something to get the daemon to start writing to the new version '0' log file
    instead of continuing to write to the version '1' log file.
    What we do is, once the log files have been rotated, if there was an active daemon of the
    corresponding name, then we stop it and then start it again.  When it is restarted,
    it will begin appending to the version '0' log file, as desired.

### Functions You Must Implement

There is just one function that you are required to implement according to a particular
interface specification; the rest is up to you.  This function is as follows:

* `run_cli(FILE *in, FILE *out)` - This function is called from `main()` to allow the
  user to interact with the program.  User commands are to be read from the stream `in`
  and output for the user (including error messages) is to be written to the stream `out`.
  You should not make any assumptions about what these streams are, as during grading we
  will call this function from a test driver that replaces the human user.
  Be sure to `fflush(out)` each time a prompt has been printed and you are about to read
  user input, otherwise the test driver will not be able to recognize the prompt and
  know when your program is ready to receive a command.

  Any initialization your program requires must be performed out of the `run_cli()` function.
  **Do not make any changes to `main.c`.**

### Event Functions You Must Call

In order to allow us to track the behavior of your program in grading scripts,
you are required at specific times to call certain "event functions" which are provided
for you in the `lib/sf_event.o` library which is linked automatically with your program.
These event functions are as follows:

* `sf_init(void)` - To be called when `legion` begins execution.  A call to this function
  has already been included for you in `main()`.

* `sf_fini(void)` - To be called when `legion` terminates, either as a result of the
  user having entered the `quit` command or as a result of the delivery of a `SIGINT`
  signal.  A call to this function has already been included for you in `main()`,
  but if you terminate the program in some way other than by returning from main,
  you will have to include additional call(s).

* `sf_register(char *daemon_name, char *cmd)` - To be called when a daemon is registered.
  The `cmd` argument is the name of the executable program to be invoked when the daemon
  is started.

* `sf_unregister(char *daemon_name)` - To be called when a daemon is unregistered.

* `sf_start(char *daemon_name)` - To be called when a daemon is about to be started.

* `sf_active(char *daemon_name, pid_t pid)` - To be called when a daemon process
  has been successfully forked, the synchronization message has been received,
  and the daemon has been placed into the `active` state.

* `sf_stop(char *daemon_name, pid_t pid)` - To be called when `legion` sends
  a daemon process a `SIGTERM` signal and sets it to the `stopping` state.

* `sf_kill(char *daemon_name, pid_t pid)` - To be called when `legion` has sent a daemon
  process a `SIGKILL` signal, after a previously sent `SIGTERM` did not cause the daemon
  to terminate in a timely fashion.

* `sf_term(char *daemon_name, pid_t pid, int exit_status)` - To be called when `legion`
  learns (via receipt of `SIGCHLD` and subsequent call to `waitpid(2)`) that a daemon
  process has terminated via `exit()`.

* `sf_crash(char *daemon_name, pid_t pid, int signal)` - To be called when `legion`
  learns (via receipt of `SIGCHLD` and subsequent call to `waitpid(2)`) that a daemon
  process has terminated abnormally as a result of a signal.

* `sf_reset(char *daemon_name)` - To be called when a daemon that has previously exited
  or crashed is reset (as a result of a `stop` command entered by the user) to the `inactive`
  state.

* `sf_logrotate(char *daemon_name)` - To be called when `legion` performs the log rotation
  procedure for a daemon, before the daemon is restarted.

* `sf_error(char *reason)` - To be called when the execution of a user command fails due
  to an error.  A string describing the error may be passed, or `NULL`.

* `sf_prompt()` - To be called just before printing a prompt for the user.  When your
  program is being automatically tested, it is this call that the test driver will use to
  determine whether the user has been prompted, rather than trying to read and parse
  what you write to the output stream.

* `sf_status(char *msg)` - To be called just before printing a status line.  When your
  program is being automatically tested, it is this call that the test driver will use to
  detect that a status line has been printed and to obtain its contents.  The actual line
  that you print to the output stream will not be read by the test driver.

If you run your program "manually" as `bin/legion`, then the behavior of these functions
will be to simply issue a message to `stderr` each time they are called.
This is so that you can verify that you are making the required calls to these functions.
If having the messages print out is not desired, you can disable it by assigning
a non-zero value to the integer variable `sf_suppress_chatter`.
When your program is invoked by the test driver (`bin/legion_tests`) then instead of
printint to `stderr`, these functions will send messages to the test driver over a
"socket".  This will allow the test driver to follow what your program is doing and
check whether or not it makes sense.

### Signal Handling

Your `legion` program should install a `SIGCHLD` handler so that it can be notified
when daemon processes exit or crash.  Your program should also catch `SIGINT`,
which it should treat as an alternative way of quitting the program.
Similarly to the `quit` command, when `SIGINT` is caught, any active daemons must be
stopped before the program exits.
A handler for `SIGALRM` will also be required to implement the timeouts in the situations
describe above.

If you want your programs to work reliably, you must only use async-signal-safe
functions in your signal handlers.
You should make every effort not to do anything "complicated" in a signal handler;
rather the handlers should just set flags or other variables to communicate back to
the main program what has occurred.  The main program should check these flags and
perform any additional actions that might be necessary.
Variables used for communication between the handler and the main program should
be declared `volatile` so that a handler will always see values that
are up-to-date (otherwise, the C compiler might generate code to cache updated
values in a register for an indefinite period, which could make it look to a
handler like the value of a variable has not been changed when in fact it has).
Ideally, such variables should be of type `sig_atomic_t`, which means that they
are just integer flags that are read and written by single instructions.
Note that operations, such as the auto-increment `x++`, and certainly more complex
operations such as structure assignments, will generally not be performed as a single
instruction.  This means that it would be possible for a signal handler to be
invoked "in the middle" of such an operation, which could lead to "flaky"
behavior if it were to occur.

  > :nerd: Note that you will need to use `sigprocmask()` to block signals at
  > appropriate times, to avoid races between the handler and the main program,
  > the occurrence of which can also result in indeterminate behavior.
  > In general, signals must be blocked any time the main program is actively
  > involved in manipulating variables that are shared with a signal handler.

Note that standard I/O functions such as `fprintf()` are not async-signal-safe,
and thus cannnot reliably be used in signal handlers.  For example, suppose the
main program is in the middle of doing an `fprintf()` when a signal handler is invoked
asynchronously and the handler itself tries to do `fprintf()`.  The two invocations
of `fprintf()` share state (not just the `FILE` objects that are being printed to,
but also static variables used by functions that do output formatting).
The `fprintf()` in the handler can either see an inconsistent state left by the
interrupted `fprintf()` of the main program, or it can make changes to this state that
are then visible upon return to the main program.  Although it can be quite useful
to put debugging printout in a signal handler, you should be aware that you can
(and quite likely will) see anomalous behavior resulting from this, especially
as the invocations of the handlers become more frequent.  Definitely be sure to
remove or disable this debugging printout in any "production" version of your
program, or you risk unreliability.

## Provided Components

### The `legion.h` Header File

The `legion.h` header file that we have provided defines various constants and
data structures for `legion`, gives function prototypes for the functions that you
are to use and those that you are to implement, and may contain specifications for
these functions that you should read carefully in conjunction with the information
provided in the present assignment document.

  > :scream: **Do not make any changes to `legion.h`.  It will be replaced
  > during grading, and if you change it, you will get a zero!**

### The `main.c` Source File

The `main.c` file contains the `main()` function for `legion`.
It already contains calls to `sf_init()`, `run_cli()`, and `sf_fini()`.
**It will be replaced during grading -- don't change it.**

### The `cli.c` Source File

This file contains a stub for the `run_cli()` function, which you are to implement.
You should put the implementation of this function in this file, but you may
create additional source and header files as you like.

### The `lib/sf_event.o` Object File

The `lib/sf_event.o` object file contains implementations of the functions that
you are to call upon occurrence of various events (see *Event Functions* above).
This file will be linked with your `legion` program.

### The `demo/legion` Executable

To help answer questions about what you are expected to implement and to make
it easier for you to get something working that you can test, I have included a
demonstration version of `legion` as `demo/legion`, which you can invoke by
typing `demo/legion`.  This program emits output to `stderr` when the `sf_xxx()`
event functions are called.  All other output produced by this program is
sent to `stdout`.

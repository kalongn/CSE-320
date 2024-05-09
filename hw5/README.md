# Homework 5 - CSE 320 - Spring 2024
#### Professor Eugene Stark

## Introduction

The goal of this assignment is to become familiar with low-level POSIX
threads, multi-threading safety, concurrency guarantees, and
networking.  The overall objective is to implement a simple multi-threaded
chat server.  As this is more difficult than it might sound, to grease the
way I have provided you with a design for the server, as well as binary object
files for almost all the modules.  This means that you can build a
functioning server without initially facing too much complexity.
In each step of the assignment, you will replace one of my
binary modules with one built from your own source code.  If you succeed
in replacing all of my modules, you will have completed your own
version of the server.

It is probably best if you work on the modules in roughly the order
indicated below.  Turn in as many modules as you have been able to finish
and have confidence in.  Don't submit incomplete modules or modules
that don't function at some level, as these will negatively impact
the ability of the code to be compiled or to pass tests.

### Takeaways

After completing this homework, you should:

* Have a basic understanding of socket programming
* Understand thread execution, mutexes, and semaphores
* Have an advanced understanding of POSIX threads
* Have some insight into the design of concurrent data structures
* Have enhanced your C programming abilities

## Hints and Tips

* We strongly recommend you check the return codes of all system
  calls. This will help you catch errors.

* **BEAT UP YOUR OWN CODE!** Throw lots of concurrent calls at your
  data structure libraries to ensure safety.

* Your code should **NEVER** crash. We will be deducting points for
  each time your program crashes during grading. Make sure your code
  handles invalid usage gracefully.

* You should make use of the macros in `debug.h`. You would never
  expect a library to print arbitrary statements as it could interfere
  with the program using the library. **FOLLOW THIS CONVENTION!**
  `make debug` is your friend.

> :scream: **DO NOT** modify any of the header files provided to you in the base code.
> These have to remain unmodified so that the modules can interoperate correctly.
> We will replace these header files with the original versions during grading.
> You are of course welcome to create your own header files that contain anything
> you wish.

> :scream: Also, **DO NOT** modify the interfaces between the modules by introducing
any "back door" communication between them; for example, passing information via
global variables.  The information in the header file for a module constitutes the
entirety of the interface to that module.  If you violate this, you will lose
points during grading because your modules will not interact properly with ours.

> :nerd: When writing your program, try to comment as much as possible
> and stay consistent with your formatting.

## Helpful Resources

### Textbook Readings

You should make sure that you understand the material covered in
chapters **11.4** and **12** of **Computer Systems: A Programmer's
Perspective 3rd Edition** before starting this assignment.  These
chapters cover networking and concurrency in great detail and will be
an invaluable resource for this assignment.

### pthread Man Pages

The pthread man pages can be easily accessed through your terminal.
However, [this opengroup.org site](http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html)
provides a list of all the available functions.  The same list is also
available for [semaphores](http://pubs.opengroup.org/onlinepubs/7908799/xsh/semaphore.h.html).

## Getting Started

Fetch and merge the base code for `hw5` as described in `hw0`. You can
find it at this link: https://gitlab02.cs.stonybrook.edu/cse320/hw5.
Remember to use the `--stategy-option theirs` flag for the `git merge`
command to avoid merge conflicts in the Gitlab CI file.

## The Charla Server and Protocol: Overview

The "Charla" server implements a simple chat service that allows
clients to send messages to each other.  A client wishing to make use
of the system first makes a network connection to the server.  Once
connected to the server, a client can make the following **requests**
to the server:

- Log in to the system, specifying a user-friendly **handle** to identify
  the client.

- Obtain a list of handles of all clients currently logged in to the server.

- Send a message to another client, identified by its handle.  The message
  is placed in that client's mailbox, to be delivered as soon as possible.

- Log out from the system.

In response to these requests, the server will send a **notice**,
consisting of either a positive acknowledgement (`ACK`) or a negative
acknowledgement (`NACK`).  A positive acknowledgement indicates that
the server has carried out the request; a negative acknowledgement
indicates that some error occurred.

Besides `ACK` and `NACK`, the server will send the following additional
types of notices, which are issued asynchronously and independently
of any requests that the client has made:

- Delivery of a message sent by another client to this client.

- Notice that a previously sent message has been delivered to its recipient.

- Notice that a previously sent message has bounced, because the recipient
  disconnected from the server before the message could be delivered.

A client may disconnect from the system at any time.  If the client
was logged in, then it will be logged out.  Any pending messages for that
client are then discarded, and bounce notices are sent to the senders of
those messages.  The handle will remain registered with the system.

The Charla server architecture is that of a multi-threaded network
server.  When the server is started, the **main thread** sets up a
socket on which to listen for connections from clients.  When a
connection is accepted, a **client service thread** is started to
handle requests sent by the client over that connection.  The client
service thread executes a service loop in which it repeatedly receives
a **request packet** sent by the client, performs the request, and
sends either an `ACK` or `NACK` packet in response.

One of the requests that the client can make is a `LOGIN` request,
which contains a handle by which the client wishes to be identified.
In the case of a successful login, an `ACK` packet is sent to the
client.  If the requested handle is already in use by another client,
then the server responds with a `NACK` packet.  If the requested
handle is not already in use, a **mailbox** is created and associated
with that client.  In addition, a **mailbox service thread** is started
to handle messages and notices posted to the mailbox. The mailbox
service thread executes a service loop in which it repeatedly receives
a message or notice from the mailbox and sends it over the network
connection to the client.  The service loop continues to execute until
the mailbox is shut down as a result of the client logging out
(note that disconnection of the client causes an automatic logout).
At that point, the mailbox service thread terminates.

A client can also make a `LOGOUT` request, which asks the server
to log out the client.  Upon receipt of such a request, if the client
was logged in, then it is logged out, its mailbox is shut down,
and the mailbox service thread terminates.  The client is sent an
`ACK` if it was previously logged in and is now logged out,
otherwise a `NACK` is sent.

> :nerd:  Once a client has logged out and its mailbox has been shut
> down, no further messages or notices can be sent to that mailbox.
> However, it can happen that at however at the time of removal it
> might be the case that references to the mailbox are still being
> held by threads that are trying to send a message to that mailbox.
> The mailbox object cannot be freed until no such references exist,
> otherwise these other threads will end up with dangling pointers.
> This issue creates a design complication that will be explained
> further below.

> :nerd: One of the basic tenets of network programming is that a
> network connection can be broken at any time and the parties using
> such a connection must be able to handle this situation.  In the
> present context, the client's connection to the Charla server may
> be broken at any time, either as a result of explicit action by the
> client or for other reasons.  When disconnection of the client is
> noticed by the client service thread, the client is immediately
> logged out.  No `ACK` or `NACK` is sent in this case, because
> there is no longer any connection to a client to send them to.

A third kind of request that the client can make is a `USERS` request,
which asks the server for a list of the handles of currently logged-in
clients.  Upon receipt of such a request, the server obtains a list
of the currently registered handles.  It then sends the client an
`ACK` packet that contains this list.

The fourth kind of request that the client can make is a `SEND` request,
which asks the server to send a message to another client,
identified by its handle.  Upon receipt of such a request, the server
checks that the sender is currently logged in, that the specified
recipient is currently logged in, and it obtains the sender and recipient
mailboxes.  A message is then constructed that contains a reference to
the sender's mailbox in a "from" field and the content to be sent in a
"body" field.  (The reason for storing a reference to the sender's mailbox
in the message, rather than just the sender's handle, is explained further
below.)  The newly constructed message is added to the recipient's mailbox,
where it will ultimately be discovered by the recipient's mailbox
service thread and delivered to the recipient.  If the above procedure
succeeds, an `ACK` is sent to the sending client; otherwise `NACK` is
sent.

### The Base Code

Here is the structure of the base code:

```
.
├── .gitlab-ci.yml
└── hw5
    ├── .gitignore
    ├── include
    │   ├── client.h
    │   ├── client_registry.h
    │   ├── debug.h
    │   ├── globals.h
    │   ├── mailbox.h
    │   ├── protocol.h
    │   ├── server.h
    │   ├── user.h
    │   └── user_registry.h
    ├── lib
    │   ├── charla.a
    │   └── charla_debug.a
    ├── Makefile
    ├── sockets
    │   └── .git-keep
    ├── src
    │   ├── globals.c
    │   └── main.c
    ├── test_output
    │   └── .git-keep
    ├── tests
    │   └── charla_tests.c
    └── util
        └── client
```

The base code consists of header files that define module interfaces,
libraries `charla.a` and `charla_debug.a` containing binary object code
for our implementations of the modules, a source file `globals.c` that
contains definitions of a few global variables, and a source file `main.c`
that contains containing a stub for function `main()`.  The `Makefile` is
designed to compile any existing source code files and then link them
against the provided library.  The result is that any modules for
which you provide source code will be included in the final executable,
but modules for which no source code is provided will be pulled in
from the library.

> :nerd: Note that if you provide code for any of the functions in
> one of the modules, you must provide code (at least stubs) for everything
> in that module, otherwise you will get "multiply defined" errors
> from the linker.

The `util` directory contains an executable for a simple test
client.  When you run this program it will print a help message that
summarizes the commands.  The client program understands command-line
options `-h <hostname>`, `-p <port>`, `-d` and `-q`.
The `-p` option is required; the others are options.
The `-h` option is used to specify the name of the host on which
the Charla server is running, and the `-p` option is used to specify
the port on that host to connect to.  If `-h` is not given, then the
host defaults to `localhost` (via the IP "loopback" address
`127.0.0.1`).  The `-p` option is required, and it must be the same
port number as was used when the Charla server was started.
The `-d` flag specifies "debug mode": in this mode the client will
print low-level information about the packets being sent and received.
If `-d` is not given, then the client runs in a mode that is more like
what the user of a "production" version of the system would see.
The `-q` flag tells the client to run without prompting; this is useful
if you want to run the client with the standard input redirected
from a file (say, for automated testing purposes).

## Additional Background Information

### Reference Counting

The modules in the Charla server use the important technique of **reference counting**.
A reference count is a field maintained in an object to keep track of the number
of extant pointers to that object.  Each time a new pointer to the object is created,
the reference count is incremented.  Each time a pointer is released, the reference
count is decremented.  A reference-counted object is freed when, and only when,
the reference count reaches zero.  Using this scheme, once a thread has obtained
a pointer to an object, with the associated incremented reference count,
it can be sure that, until it explicitly releases that object and decrements the
reference count, the object will not be freed.

In the Charla server, the following types of objects are reference counted:
`CLIENT`, `USER`, and `MAILBOX`.
The specifications of the functions provided by the various modules include
information on when reference counts are incremented and whose responsibility
it is to decrement these reference counts.  It is important to pay attention to this
information -- if you do not, your implementation will end up with storage leaks,
or worse, segmentation faults due to "dangling pointers".
Some of the "get" functions do not increment the reference count of the object
they return.  This is to make them more convenient to use in a setting in which
one is already holding a reference to the containing object.
As long as the containing object is not freed, neither will the objects returned
by the "get" function.  However, if you obtain an object by a "get" function,
and then decrement the reference count on the containing object, it is possible
that the containing object will be freed as a result.  This will result in the
contained objects having their reference counts decremented, and those objects
might then be freed.  You could then end up with a "dangling pointer" to a free object.
To avoid this, if you intend to use a pointer returned by a "get" function after
the containing object has had its reference count decreased, then you should first
explicitly increase the reference count of the object returned by "get" so that
the pointer is guaranteed to be valid until you are finished with it.

Finally, note that, in a multi-threaded setting, the reference count in an object
is accessed concurrently by all threads sharing that object and therefore needs
to be protected by a mutex if it is to work reliably.

### Thread Safety

Nearly all of the modules in the Charla server implement data that is shared
by multiple threads, so synchronization has to be used to make these modules
thread-safe.  The basic approach to this is for each object to contain
a mutex which is locked while the object is being manipulated and unlocked
when the manipulation is finished.  The mutexes will be private fields of the
objects, which are not exposed by the interfaces of the modules.
It will be your responsibility to include the necessary mutexes in your
implementation and to determine when the mutexes should be locked or
unlocked in each function.  Some modules might require "recursive mutexes",
which can be locked multiple times by the same thread.  This usually happens
if there are nested calls to functions that need to lock the object.
Refer to pthreads mutex documentation for information on how to initialize
a recursive mutex.  Some modules may need some additional synchronization
features; for example, for the client registry it is suggested that you use
a semaphore in order to implement the `creg_shutdown_all` functionality.

### Debugging Multi-threaded Programs

GDB has support for debugging multi-threaded programs.
At any given time, there is one thread on which the debugger is currently
focused.  The usual commands such as `bt` (backtrace, to get a stack trace)
pertain to the current thread.  If you want to find out about a different
thread, you need to change the focus to that thread.
The `info threads` command will show you a list of the existing threads.
Each thread has a corresponding "Id" which you can use to specify that
thread to `gdb`.  The command `thread N` (replace `N` by the ID of a thread)
will switch the focus to that particular thread.

## Task I: Server Initialization

When the base code is compiled and run, it will print out a message
saying that the server will not function until `main()` is
implemented.  This is your first task.  The `main()` function will
need to do the following things:

- Obtain the port number to be used by the server from the command-line
  arguments.  The port number is to be supplied using the required
  option `-p <port>`.
  
- Initialize the `user_registry` and `client_registry` modules,
  storing references to these registries in the global variables
  `user_registry` and `client_registry`.

- Set up a server socket to listen for incoming connection requests
  on the specified port.

- Install a `SIGHUP` handler so that clean termination of the server can
  be achieved by sending it a `SIGHUP`.  Note that you need to use
  `sigaction()` rather than `signal()`, as the behavior of the latter is
  not well-defined in a multithreaded context.

- Enter a loop to accept connections.  For each connection, a thread
  should be started to run function `chla_client_service()`.

These things should be relatively straightforward to accomplish, given the
information presented in class and in the textbook.  If you do them properly,
the server should function and accept connections on the specified port,
and you should be able to connect to the server using the test client.
Note that if you build the server using `make debug`, then the binaries
we have supplied will produce a fairly extensive debugging trace of what
they are doing.  This, together with the specifications in this document
and in the header files, should help you to understand the behavior of the
various modules.

## Task II: Send and Receive Functions

The header file `include/protocol.h` defines the format of the packets
used in the Charla network protocol.  The concept of a protocol is an
important one to understand.  A protocol creates a standard for
communication so that any program implementing a protocol will be able
to connect and operate with any other program implementing the same
protocol.  Any client should work with any server if they both
implement the same protocol correctly.  In the Charla protocol,
clients and servers exchange **packets** with each other.  Each packet
has two parts: a required **header** that describes the packet, and an
optional **payload** that can carry arbitrary data.  Packet headers
always have the same size and format, which is given by the
`CHLA_PACKET_HEADER` structure; however the payload can be of arbitrary
size.  One of the fields in the header tells how long the payload is.

- The function `proto_send_packet` is used to send a packet over a
network connection.  The `fd` argument is the file descriptor of a
socket over which the packet is to be sent.  The `hdr` argument is a
pointer to the header of the packet.  The `payload` argument is a
pointer to the payload, if there is one, otherwise it is `NULL`.  The
`proto_send_packet` assumes that multi-byte fields in the packet
passed to it are in **network byte order**, which is the standard
byte order used to communicate over the Internet.
However, as byte ordering (i.e. "endianness") differs between computers,
when filling in the fields of the header it is necessary to convert
any multi-byte quantities from **host byte order** to network byte
order.  This can be done using, e.g., the `htons()`, `htonl()`,
and related functions described in the Linux man pages.
The `write()` system call is used to write the header to the "wire"
(i.e. the network connection).  If the length field of the header
specifies a nonzero payload length, then an additional `write()`
call is used to write the payload data to the wire.

- The function `proto_recv_packet()` reverses the procedure in order to
receive a packet.  It first uses the `read()` system call to read a
packet header from the wire.  If the length field of the header is
nonzero then an additional `read()` is used to read the payload from
the wire (note that the length has to first be converted from network
byte order to host byte order before passing it in a call to `read()`).
The header and payload are stored using pointers supplied by the caller.
The fields of header are returned in network byte order; it is the
responsiblity of the caller to make the conversion to host byte order
when reading the data from these fields.

**NOTE:** Remember that it is always possible for `read()` and `write()`
to read or write fewer bytes than requested.  You must check for and
handle these "short count" situations.

Implement these functions in a file `protocol.c`.  If you do it
correctly, the server should function as before.

## Task III: Users

The next task is to implement USER objects, which represent users of the
system.  A USER has a handle, which is fixed at the time the user is created
and does not change thereafter, and possibly other information
(well, I was intending that there would be other information, but as of
right now there isn't any).
USER objects are managed by the user registry, where they persist across
login sessions.
USER objects are reference counted, so that they can be passed around
externally to the user registry without danger of dangling references.
The functions to be implemented are specified in the file
`include/user.h`, and the implementation should be in a file `src/user.c`.
To avoid redundant (and possibly inconsistent) copies of the specifications
for this (and subsequent) modules they are not repeated here.
Refer to the header file for details.

## Task IV: User Registry

The next task is to implement the user registry.  This should be a
fairly simple warm-up in concurrent programming.
A user registry maintains a mapping from handles to USER objects.
Entries in the user registry persist across login sessions,
for as long as the server continues to run.
The functions to be implemented are specified in the file
`include/user_registry.h`, and the implementation should be in a file
`src/user_registry.c`.
You are free to choose the data structure used to store the
entries, as well as the specific content of those entries.
Note once again that these functions will be called concurrently,
so they have to be implemented in a thead-safe way.

## TASK V: Clients

A CLIENT object represents the state of a network client connected
to the system.  It contains the file descriptor of the connection to
the client and it provides functions for sending packets to the client.
If the client is logged in as a particular user, it also contains a
reference to a USER object and to a MAILBOX object.
Note that whereas the file descriptor is set when a CLIENT is created
and does not change, the USER and MAILBOX objects can change
asynchronously due to the client logging in and out.  Because of this,
some care needs to be taken to avoid races when accessing these fields.

An important function of the client object is to serialize access
to the client's network connection.  All messages for a client should
be sent using the functions provided by the CLIENT object, so that
concurrent calls do not interfere with each other.

CLIENT objects are managed by the client registry, and are reference
counted so that references can be safely passed around externally
to the client_registry.

## Task VI: Client Registry

The client registry keeps track of the clients that are currently
connected to the server.  This is similar to the function of the
user registry, except that there are a couple more features.
First, the client registry provides a way to obtain a list of all
currently connected clients.
Second, the client registry also provides a way for the main
thread to shut down all client connections and wait for the clean
termination of all server threads before finally terminating the
server process.  This function is called from the `terminate()`
function in `main.c`.

The shutdown mechanism perhaps deserves some additional discussion.
At the time the server is to be shut down, there will in general be
clients connected to the server, service threads running,
messages in transit, and so on.
We want to shut down the server in an organized fashion so that
all service threads are terminated, all connections are closed,
all objects such as messages are freed, and only then does the
server process exit.
To accomplish this, the shutdown procedure is initiated by the main
thread making a call to `creg_shutdown_all()`.
The `creg_shutdown_all()` function will go through the set of currently
connected clients and call the system function `shutdown(2)` on each
of the registered file descriptors.
As a result, the network connections will be terminated and the
server threads handling these connections will detect EOF on them.
These client service threads will respond by using `client_logout()`
to log out the client.  The `client_logout()` function in turn
calls `mb_shutdown()` on the associated mailbox, which marks
that mailbox as "defunct".  The mailbox service thread handling that
mailbox will then return from `mb_next_entry()`, notice that the
mailbox is defunct, and will terminate.  The client service thread will
be waiting (using `pthread_join()`) for the mailbox service thread
to terminate.  Once this occurs, the client service thread will
unregister the client from the client registry and will itself
terminate.  When the number of registered clients drops to zero,
the main thread blocked in the call to `creg_shutdown_all()`
will return from that call and complete the termination procedure
by finalizing the client registry and the user registry and then
calling `exit()`.

> :nerd: You might want to study how this happens by running the
> debug version of the server, connecting several clients, then
> using the `kill` shell command to send a `SIGHUP` to the server.
> The debugging messages will show the sequence of events that
> occurs during server shutdown.

The functions provided by the client registry are specified in the
`client_registry.h` header file.  Provide implementations for these
functions in a file `client_registry.c`.  Note that these functions
will need to be thread-safe, so synchronization will be required.
Use a mutex to protect access to the client registry data.
It is up to you to devise a scheme by which the caller of
`creg_shutdown_all()` will block waiting for the number of registered
clients to reach zero and then return (hint: use a semaphore).
If you implement the client registry correctly, the Charla server
should still shut down cleanly in response to SIGHUP using your version.

**Note:** You should certainly unit-test your client registry
separately from the rest of the server.  Create test threads that
rapidly call `creg_register()` and `creg_unregister()` methods
concurrently and then check that a call to `creg_shutdown_all()`
function blocks until the registered client count reaches zero
before returning.

## Task VII: Mailboxes

Next, implement mailboxes.  A MAILBOX object holds messages and notices
to be sent to the associated client.   A *message* is a user-generated
transmission from one client to another.  A *notice* is a server-generated
transmission that informs a client about the disposition of a previously
sent message.

The mailbox functions are specified in `include/mailbox.h` and they should
be implemented in a file `src/mailbox.c`.  Once again, keep in mind that
all these functions have to be thread-safe.

The core mailbox functions are `mb_add_message()`, `mb_add_notice()`,
and `mb_next_entry()`, which enqueue and remove mailbox entries.  The
behavior of these functions follows the producer/consumer paradigm
(see section **12.5** in **Computer Systems: A Programmer's
Perspective 3rd Edition**) in the sense that callers of
`mb_add_message()` and `mb_add_notice()` **produce** entries to be
added to a mailbox and callers of `mb_next_entry()` **consume**
entries from the mailbox.  If a consumer tries to consume an entry
from an empty mailbox, then it should block until an entry becomes
available.  Similarly, if there were a maximum capacity specified for
a mailbox, then a producer trying to insert an entry into the mailbox
should block until there is space.  For this assignment, we have not
set a maximum capacity for a mailbox (though it would be essential do
so before deploying this server in a real networking environment) so
you don't have to worry about producers blocking.

`MAILBOX` objects are reference counted.  This makes it possible to
temporarily export them from the `CLIENT` object in which they live
and pass them around without danger of dangling references.
This happens during the operations of sending a message and
constructing a list of handles of currently logged-in users when
processing a `USERS` request from a client.

When a `MAILBOX` is created during client login, a new thread,
the "mailbox service thread" is also created (note that the creation
of this thread and the thread routine that it runs is actually part
of the server module, not the mailbox module).  The function of the
mailbox service thread is to wait for messages and notices to appear
in the `MAILBOX`, remove them one at a time, and send them over the
network connection to the client.
The mailbox service thread executes a loop in which it repeatedly
calls the `mb_next_entry()` function of the mailbox module to wait
for and remove an entry from a mailbox and then it sends the
message or notice contained in that entry to the client.
However, there has to be a way to shut down the mailbox and cause
the termination of the mailbox service thread when the client logs
out.  This is accomplished by the `mb_shutdown()` function.
That function sets a flag to mark the mailbox as in the "defunct" state.
It then has to wake up the mailbox service thread that is blocked in
`mb_next_entry()`, so it can notice that the mailbox is now defunct
and terminate.

To arrange for the mailbox service thread to block waiting for the
arrival of a message in `mb_next_entry()` you can call `sem_wait()`
on a semaphore.  When the mailbox is shut down, `sem_post()` on that
semaphore can be used to release the consumer thread so that it can
return from `mb_next_entry()`.  It is then the responsibility of the
caller of `mb_next_entry()` to check whether the mailbox is defunct and
if so, to take action to free resources and terminate.

Yet another issue that mailboxes have to deal with is what to do with
mailbox entries that remain when a mailbox is shut down.
The specification of the Charla server requires that a bounce notice
be sent to the sender of each message that is discarded because
its recipient logged out or disconnected before the message could
be delivered.  So, as part of the process of shutting down a mailbox,
these bounce notices have to be generated.
In order to avoid having the mailbox module depend on the details
of its client modules, a mailbox provides the ability for its client
to provide a **discard hook** to be called when a mailbox entry is
discarded.  This is done by the `mb_set_discard_hook()` function.
During the process of mailbox finalization, if a discard hook has
been set, then it is invoked on each mailbox entry that remains
in the mailbox.  Once the discard hook has been called on an entry,
the entry itself can be freed.

In the Charla server, the discard hook for a mailbox will be set
to a function that generates a bounce notice if the entry being
discarded is an undelivered message.  To do this, the discard hook
needs access to the mailbox of the original sender of a message.
To avoid excessive copying of the sender's handle and repeated
lookups of this handle in the directory, we have chosen for message
entries in a mailbox to record the sender of a message not by its
handle, but by a pointer to the sender's mailbox.  This makes it
easy to send a bounce notice, but note that a reference count must
be associated with the mailbox pointer stored in a message.
This means that when a message is discarded during mailbox finalization,
the `mb_unref()` function must be called on the `from` field of each
message being discarded.

### Task VIII: Service Thread Functions

If you've made it this far, there is only one more module to implement
to complete your version of the server.
As described in the overview, each time the server accepts a connection
from a client a new thread is created to run the `chla_client_service()`
function, and when a client logs in a new thread is created to run
the `chla_mailbox_service()` function.  Your final task is to implement
these thread functions, which are specified in the header file
`include/server.h`.  You should implement these functions in the
file `src/server.c`.  Note that the number of lines of code required
here is probably the largest of any of the modules you have to
implement, so you should certainly define helper functions as appropriate.

> :nerd: We strongly suggest that you do not attempt this part of the
> assignment until you have successfully completed the other parts.
> You will need a good understanding of all the other parts of the
> server in order to code the service thread functions.

The `chla_client_service()` function should contain a service loop
that repeatedly reads a request packet sent by the client, carries out
the request, and arranges for the required `ACK` or `NACK` to be sent.
At this point, you should probably have a reasonably good understanding
of what has to be done in order to carry out each request.
The `chla_mailbox_service()` function should contain a service loop
that repeatedly calls `mb_next_entry()` on the client's mailbox.
Each entry will contain a notice or message to be sent over the wire
to the client.  In addition, when a message is sent, a delivery
notification needs to be generated and enqueued in the sender's
mailbox.
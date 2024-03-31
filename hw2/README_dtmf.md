# Homework 1 - CSE 320
#### Professor Eugene Stark

**Read the entire doc before you start**

## Introduction

In this assignment, you will write a command line utility to synthesize audio
data containing Dual-Tone Multi-Frequency (DTMF) signals and to detect DTMF
signals in audio data.
The goal of this homework is to familiarize yourself with C programming,
with a focus on input/output, bitwise manipulations, and the use of pointers.

For all assignments in this course, you **MUST NOT** put any of the functions
that you write into the `main.c` file.  The file `main.c` **MUST ONLY** contain
`#include`s, local `#define`s and the `main` function (you may of course modify
the `main` function body).  The reason for this restriction has to do with our
use of the Criterion library to test your code.
Beyond this, you may have as many or as few additional `.c` files in the `src`
directory as you wish.  Also, you may declare as many or as few headers as you wish.
Note, however, that header and `.c` files distributed with the assignment base code
often contain a comment at the beginning which states that they are not to be
modified.  **PLEASE** take note of these comments and do not modify any such files,
as they will be replaced by the original versions during grading.

> :scream: Array indexing (**'A[]'**) is not allowed in this assignment. You
> **MUST USE** pointer arithmetic instead. All necessary arrays are declared in
> the `const.h` header file. You **MUST USE** these arrays. **DO NOT** create
> your own arrays. We **WILL** check for this.

> :nerd: Reference for pointers: [https://beej.us/guide/bgc/html/multi/pointers.html](https://beej.us/guide/bgc/html/multi/pointers.html).

# Getting Started

Here is the structure of the base code:

<pre>
.
└── hw1
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
        └── hw1_tests.c
</pre>

- The `Makefile` is a configuration file for the `make` build utility, which is what
you should use to compile your code.  In brief, `make` or `make all` will compile
anything that needs to be, `make debug` does the same except that it compiles the code
with options suitable for debugging, and `make clean` removes files that resulted from
a previous compilation.  These "targets" can be combined; for example, you would use
`make clean debug` to ensure a complete clean and rebuild of everything for debugging.

- The `include` directory contains C header files (with extension `.h`) that are used
by the code.  Note that these files contain `DO NOT MODIFY` instructions at the beginning.
You should observe these notices carefully where they appear.

- The `src` directory contains C source files (with extension `.c`).

- The `tests` directory contains C source code (and sometimes headers and other files)
that are used by the Criterion tests.

- The `rsrc` directory contains some samples of audio files that you can use for
testing purposes.

## A Note about Program Output

What a program does and does not print is VERY important.
In the UNIX world stringing together programs with piping and scripting is
commonplace. Although combining programs in this way is extremely powerful, it
means that each program must not print extraneous output. For example, you would
expect `ls` to output a list of files in a directory and nothing else.
Similarly, your program must follow the specifications for normal operation.
One part of our grading of this assignment will be to check whether your program
produces EXACTLY the specified output.  If your program produces output that deviates
from the specifications, even in a minor way, or if it produces extraneous output
that was not part of the specifications, it will adversely impact your grade
in a significant way, so pay close attention.

**Use the debug macro `debug` (described in the 320 reference document in the
Piazza resources section) for any other program output or messages you many need
while coding (e.g. debugging output).**

# Part 1: Program Operation and Argument Validation

In this part, you will write a function to validate the arguments passed to your
program via the command line. Your program will treat arguments as follows:

- If no flags are provided, you will display the usage and return with an
`EXIT_FAILURE` return code

- If the `-h` flag is provided, you will display the usage for the program and
  exit with an `EXIT_SUCCESS` return code.

- If the `-g` flag is provided, you will generate audio data containing DTMF
  signals; reading "DTMF events" from `stdin` and writing audio data to `stdout`,
  exiting with `EXIT_SUCCESS` on success and `EXIT_FAILURE` on any error.

- If the `-d` flag is provided, you will perform DTMF detection; reading
  audio data from `stdin` and writing DTMF events to `stdout`,
  exiting with `EXIT_SUCCESS` on success and `EXIT_FAILURE` on any error.
  
> :nerd: `EXIT_SUCCESS` and `EXIT_FAILURE` are macros defined in `<stdlib.h>` which
> represent success and failure return codes respectively.

> :nerd: `stdin`, `stdout`, and `stderr` are special I/O "streams", defined
> in `<stdio.h>`, which are automatically opened at the start of execution
> for all programs and do not need to be reopened.

Some of these operations will also need other command line arguments which are
described in each part of the assignment.  The usage scenarios for this program are
described by the following message, which is printed by the program when it is invoked
without any arguments:

<pre>
USAGE: bin/dtmf [-h] -g|-d [-t MSEC] [-n NOISE_FILE] [-l LEVEL] [-b BLOCKSIZE]
   -h       Help: displays this help menu.
   -g       Generate: read DTMF events from standard input, output audio data to standard output.
   -d       Detect: read audio data from standard input, output DTMF events to standard output.

            Optional additional parameters for -g (not permitted with -d):
               -t MSEC         Time duration (in milliseconds, default 1000) of the audio output.
               -n NOISE_FILE   specifies the name of an audio file containing "noise" to be combined
                               with the synthesized DTMF tones.
               -l LEVEL        specifies the loudness ratio (in dB, positive or negative) of the
                               noise to that of the DTMF tones.  A LEVEL of 0 (the default) means the
                               same level, negative values mean that the DTMF tones are louder than
                               the noise, positive values mean that the noise is louder than the
                               DTMF tones.

            Optional additional parameter for -d (not permitted with -g):
               -b BLOCKSIZE    specifies the number of samples (range [10, 1000], default 100)
                                in each block of audio to be analyzed for the presence of DTMF tones.
</pre>

The `-g|-d` means that one or the other of `-g` or `-d` may be specified.
The `[-t MSEC]` means that `-t` may be optionally specified, in which
case it is immediately followed by a parameter `MSEC`.
The specifications `[-n NOISE_FILE]`, `[-l LEVEL]`, and `[-b BLOCKSIZE]` have similar meanings.

A valid invocation of the program implies that the following hold about
the command-line arguments:

- All "positional arguments" (`-h|-g|-d`) come before any optional arguments
(`-t`, `-n`, `-l`, or `-b`).
The optional arguments may come in any order after the positional ones.

- If the `-h` flag is provided, it is the first positional argument after
the program name and any other arguments that follow are ignored.

- If the `-h` flag is *not* specified, then exactly one of `-g` or `-d`
must be specified.

- If an option requires a parameter, the corresponding parameter must be provided
(*e.g.* `-t` must always be followed by an MSEC specification).

    - If `-t` is given, the MSEC argument will be given as a non-negative integer in
    the range `[0, UINT32_MAX]`.  In the absence of `-t`, a default value of 1000
	will be used.

	- If `-n` is given, the NOISE_FILE argument will be the name of an audio file
	that contains "noise" to be combined with the synthesized DTMF tones.

	- If `-l` is given, the LEVEL argument will be an integer argument in the range
	`[-30, 30]`, which specifies the ratio, in decibels (dB), of the strength
	of the noise signal to that of the DTMF tones.  A ratio R corresponds to a dB
    value of 10*log<sub>10</sub> R, so that, for example, 0 dB means a ratio of 1
    and 3 dB means a ratio of approximately 2.  In the absence of `-l`, a default
	value of 0 dB is used.

	- If `-b` is given, the BLOCKSIZE argument will be an integer in the range
	`[10, 1000]` which specifies the number of audio samples to be used in each
	"block" that is analyzed for the presence of DTMF tones.  This is described
	in more detail below.  In the absence of `-b`, a default value of 100 will be used.

- The options `-t`, `-n`, and `-l` may only be used in combination with `-g` and
the option `-b` may only be used in combination with `-d`.

For example, the following are a subset of the possible valid argument
combinations:

- `$ bin/dtmf -h ...`
- `$ bin/dtmf -d -b 120`
- `$ bin/dtmf -g -n noise.au -l -3`

> :scream: The `...` means that all arguments, if any, are to be ignored; e.g.
> the usage `bin/dtmf -h -x -y BLAHBLAHBLAH -z` is equivalent to `bin/dtmf -h`.

Some examples of invalid combinations would be:

- `$ bin/dtmf -g -d -b 120`
- `$ bin/dtmf -b 120 -d`
- `$ bin/dtmf -g -b 120`
- `$ bin/dtmf -d -b 1k`

> :scream: You may use only "raw" `argc` and `argv` for argument parsing and
> validation. Using any libraries that parse command line arguments (e.g.
> `getopt`) is prohibited.

> :scream: Any libraries that help you parse strings are prohibited as well
> (`string.h`, `ctype.h`, etc).  The use of `scanf`, `fscanf`, `sscanf`,
> and similar functions is likewise prohibited.  *This is intentional and
> will help you practice parsing strings and manipulating pointers.*

> :scream: You **MAY NOT** use dynamic memory allocation in this assignment
> (i.e. `malloc`, `realloc`, `calloc`, `mmap`, etc.).

> :nerd: Reference for command line arguments: [https://beej.us/guide/bgc/html/multi/morestuff.html#clargs](https://beej.us/guide/bgc/html/multi/morestuff.html#clargs).

**NOTE:** The `make` command compiles the `dtmf` executable into the `bin` folder.
All commands from here on are assumed to be run from the `hw1` directory.

### **Required** Validate Arguments Function

In `const.h`, you will find the following function prototype (function
declaration) already declared for you. You **MUST** implement this function
as part of the assignment.

```c
int validargs(int argc, char **argv);
```

The file `dtmf.c` contains the following specification of the required behavior
of this function:

```c
/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the operation mode of the program (help, generate,
 * or detect) will be recorded in the global variable `global_options`,
 * where it will be accessible elsewhere in the program.
 * Global variables `audio_samples`, `noise file`, `noise_level`, and `block_size`
 * will also be set, either to values derived from specified `-t`, `-n`, `-l` and `-b`
 * options, or else to their default values.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected program operation mode, and global variables `audio_samples`,
 * `noise file`, `noise_level`, and `block_size` to contain values derived from
 * other option settings.
 */
int validargs(int argc, char **argv);
```

> :scream: This function must be implemented as specified as it will be tested
> and graded independently. **It should always return -- the USAGE macro should
> never be called from validargs.**

The `validargs` function should return -1 if there is any form of failure.
This includes, but is not limited to:

- Invalid number of arguments (too few or too many).

- Invalid ordering of arguments.

- A missing parameter to an option that requires one (e.g. `-b` with no
  `BLOCKSIZE` specification).

- Invalid `BLOCKSIZE` (if one is specified).  A `BLOCKSIZE` is invalid if it
contains characters other than the digits ('0'-'9'), or if it denotes a
value not in the range [10, 1000].

The `global_options` variable of type `int` is used to record the mode
of operation (i.e. help/generate/detect) of the program.
 This is done as follows:

- If the `-h` flag is specified, the least significant bit (bit 0) is 1.

- The second-least-significant bit (bit 1) is 1 if `-g` is passed
(i.e. the user wants generate mode).

- The third-most-significant bit (bit 2) is 1 if `-d` is passed
(i.e. the user wants detect mode).

If `validargs` returns -1 indicating failure, your program must call
`USAGE(program_name, return_code)` and return `EXIT_FAILURE`.
**Once again, `validargs` must always return, and therefore it must not
call the `USAGE(program_name, return_code)` macro itself.
That should be done in `main`.**

If `validargs` sets the least-significant bit of `global_options` to 1
(i.e. the `-h` flag was passed), your program must call `USAGE(program_name, return_code)`
and return `EXIT_SUCCESS`.

> :nerd: The `USAGE(program_name, return_code)` macro is already defined for you
> in `const.h`.

If validargs returns 0, then your program must read input data from `stdin`
and write output data to `stdout`.
Upon successful completion, your program should exit with exit status `EXIT_SUCCESS`;
otherwise, in case of an error it should exit with exit status `EXIT_FAILURE`.

> :nerd: Remember `EXIT_SUCCESS` and `EXIT_FAILURE` are defined in `<stdlib.h>`.
> Also note, `EXIT_SUCCESS` is 0 and `EXIT_FAILURE` is 1.

### Example validargs Executions

The following are examples of the setting of `global_options` and the
other global variables for various command-line inputs.
Each input is a bash command that can be used to invoke the program.

- **Input:** `bin/dtmf -h`.  **Setting:** `global_options=0x1`
(`help` bit is set, other bits clear).

- **Input:** `bin/dtmf -g -t 2000 -n `.  **Setting:** `global_options=0x2`
(`generate` bit is set), `audio_samples=16000` (there will be 8000 audio
samples per second, for an interval of 2000 msec or 2 sec, see below),
other variables set to their default values.

- **Input:** `bin/dtmf -d -b 120`.  **Setting:** `global_options=0x4`
(`detect` bit is set), `blocksize=120`, other variables set to their default values.

- **Input:** `bin/dtmf -b 120 -d`.  **Setting:** `global_options=0x0`.
This is an error case because the specified argument ordering is invalid
(`-b` is before `-d`). In this case `validargs` returns -1, leaving `global_options`
and the other options variables unset.

# Part 2: Digital Audio and the Sun Audio File Format

## Digital Audio

The purpose of this section is to provide you with some basic knowledge
of how audio is represented in digital form.
An analog audio signal is a continuous *waveform*; *i.e.* a continuous function
that maps each point in an interval of real time to an instantaneous
*amplitude* which is a real number.  In order to represent
such a signal digitally, the interval of time is replaced by
a equally spaced sequence of *sampling points*,
and the audio signal is approximated by giving a *sample* of
its amplitude at each of the sampling points.  The original real
amplitude itself is digitized, and gets replaced by a value that can be
represented in a finite number of bits.
This sampling scheme is called *pulse-code modulation*, or PCM.

Typically, digital audio data will have multiple *channels*;
*e.g.* audio intended for stereo system will have two channels:
one for each speaker.  As the channels are independent, each channel
requires its own individual sample at each sampling point.
The collection of all the samples, one for each channel, at a particular
sampling point, is called a *frame*.  The number of frames per unit
time is called the *frame rate*.  For example, CD-quality audio signals
have a frame rate of 44,100 frames per second (44.1KHz).
Each sample is represented by 16 bits, and there are two samples per frame,
representing the amplitude of the left and right channels of a stereo signal.
Thus, each frame in a CD-quality audio signal consists of a total
of four bytes (32 bits) of data (only the amplitudes need be
represented explicitly, because the time interval between the samples
is always the same).  It would thus require (10)(44,100)(4) = 1,764,000 bytes
to represent a 10-second clip of such a signal.

When digital audio signals are played, either on the computer or
on a stereo system, the digital data is read from the storage medium
and fed on demand to the audio output device.  At regularly spaced
intervals corresponding to the frame rate of the signal, the audio
output device takes each frame and presents the samples to
*digital-to-analog converters*, one for each output channel,
which convert the digital amplitude samples into voltage levels
on output wires.  These voltage levels are then amplified and used
to drive speakers.  Since each sample determines the output voltage
on one channel over an entire sampling interval whose duration is
the reciprocal of the sampling rate (1/44,100Hz = 22.68 microseconds
for CD audio), if you were to look closely, say, using an oscilloscope,
at the shape of the final waveform coming out of a digital audio
system, you would see that it is not smooth but has a "step" shape
(see figure below, which shows two cycles of a sine wave that has
been sampled at a rate of 10 samples per cycle).
However, these steps occur rapidly enough that your ear cannot
tell the difference between this signal and a smoothly varying one.

<img src="sampled_sine_wave.png">

In the digital representation of audio signals, there are several
parameters that can be changed according to the intended application
and details of the computer system on which the signals will be
processed.  The most basic parameter is the frame rate.
A higher frame rate yields a higher-quality representation of the
audio signal, but requires a larger number of bytes to represent
the signal.  In general, the highest frequency that can be represented
by a digital audio signal is equal to one-half the frame rate
(this is called the <em>Nyquist frequency</em>).
For example, CD audio with a frame rate of 44,100Hz can represent
audio frequencies up to 22,050Hz.  This is a higher frequency than
your ear can detect, so when played back the original sound is
accurately reproduced as far as your ear is concerned.
On the other hand, for other applications like telephony for which
high-fidelity reproduction is not as important, lower frame rates
are used.  For example, 8000Hz is a common choice for voice signals,
because the Nyquist frequency of 4000Hz is high enough to cover
the voice frequencies that are necessary for speech understanding.

Another important parameter in the representation of digital
audio signals is the number of bits per sample.
Once again, there is a tradeoff between the fidelity of the
representation and the amount of data required.
As mentioned above, CD-quality audio uses 16 bits (or two bytes)
per sample.  These 16 bits are used to represent
(using two's complement encoding) a signed amplitude in the
range [-32768, 32767].  This is enough that
it is generally hard for your ear to detect the fact that the
signal does not have a smoothly varying amplitude but rather
"jumps" from one discrete value to the next.
For applications that are not as demanding, 8-bit (one byte)
samples are sometimes used.
For multi-byte samples, a choice exists as to the order in
which the individual bytes in a sample appear in the data stream.
If the bytes of a sample occur starting with the the least significant
byte and ending with the most significant byte the representation is
said to be *little-endian*.
The opposite ordering is said to be *big-endian*.
These two choices do not affect the fidelity of the represented
signal -- they are more or less arbitrary variations that have to do
with the way the designers of a particular computer system chose
to store multi-byte quantities in memory.

One other variation in the way audio signals are represented digitally
is the way in which samples are encoded in a fixed number of bits.
The particular encoding scheme used in CD audio is called
*linear* encoding, or more precisely, *signed linear* encoding.
In *unsigned linear* encoding, the bits in a sample
are used to represent an *unsigned* amplitude, rather than a
signed amplitude.  In this case, a 16-bit sample would represent an
amplitude in the range [0, 65535] instead of [-32768, 32767].
Although the choice between signed and unsigned encoding affects
the details of signal processing algorithms, it does not affect
the fidelity of the signal that is ultimately played back, because
the "DC level" of a signal is inaudible and is generally filtered
out by the amplifier and speakers.
A disadvantage of linear encoding is the limited dynamic range
(the range of "loudness") that can be represented.
Other encodings exist that map the amplitudes logarithmically,
rather than linearly, to the sample bits.

To summarize the above, the representation scheme used to encode
data on audio CDs is described as
*44.1KHz, 16-bit, two-channel (i.e. stereo), signed linear PCM encoding*.
It may be *little-endian* or *big-endian*, depending on the
computer system.

## The `.au` audio file format

For this assignment, your program will need to read and write audio data.
In order to be able to manipulate this audio data with other tools
(*e.g.* to play the audio over the computer speakers), it is necessary
for the audio data to be presented in a standard format that is compatible
with these other tools.  For this purpose we will use the `.au` format,
which is an audio file format that was originally introduced by Sun Microsystems.
It is a relatively simple format that is still commonly supported on many systems,
especially on Unix and Linux systems.
The overall structure of a `.au` file can be depicted as follows:

<pre>
  +------------------+--------------------------+-------+     +-------+
  + Header (24 bytes)| Annotation (optional) \0 | Frame | ... | Frame |
  +------------------+--------------------------+-------+     +-------+
                                                ^
  &lt;----------- data offset --------------------&gt;(8-byte boundary)
</pre>

The file begins with a 24-byte *header* consisting of six unsigned 32-bit
words.  These are presented in *big-endian* format, as is all other
multibyte data in a `.au` file.

The header format can be defined by the following C code:

> <pre>
> #define AUDIO_MAGIC (0x2e736e64)
> 
> #define PCM8_ENCODING (2)
> #define PCM16_ENCODING (3)
> #define PCM24_ENCODING (4)
> #define PCM32_ENCODING (5)
> 
> typedef struct audio_header {
>     unsigned int magic_number;
>     unsigned int data_offset;
>     unsigned int data_size;
>     unsigned int encoding;
>     unsigned int sample_rate;
>     unsigned int channels;
> } AUDIO_HEADER;
> </pre>

The meanings of the header fields are as follows:

- The first field in the header is the "magic number", which is always
  equal to 0x2e736e64.  This value represents the four ASCII characters
  ".snd".

- The second field in the header is the "data offset", which is the
  number of bytes from the beginning of the file that the audio sample
  data begins.  The specifications require that this value be divisible
  by 8, though as this requirement is violated by some common tools
  (*e.g.* `sox`), we will not enforce it.  The minimum value is 24
  (when there is no annotation field), because the header consumes the
  first 24 bytes of the file.

- The third field in the header is the "data size", which is the number
  of bytes of audio sample data.  The value 0xffffffff indicates that
  the size is unknown (this could occur, for example, if the format was
  used to transmit an audio stream of indefinite duration).

- The fourth field in the header specifies the encoding used for the
  audio samples.  Some of the possible values and their meanings are:

    - 2  (specifies 8-bit linear PCM encoding)
    - 3  (specifies 16-bit linear PCM encoding)
    - 4  (specifies 24-bit linear PCM encoding)
    - 5  (specifies 32-bit linear PCM encoding)

  For this assignment, we will only support 16-bit linear PCM encoding.

- The fifth field in the header specifies the "sample rate", which is the
  number of frames per second.
  For this assignment, we will only support a rate of 8000Hz.

- The sixth field in the header specifies the number of audio channels;
  for example 1 means mono and 2 means stereo.
  We will only support 1-channel (*i.e.* mono) audio for this assignment.

Following the header is an optional "annotation field", which can be used
to store additional information (metadata) in the audio file.
The length of this field is supposed to be a multiple of eight bytes
and it must be terminated with at least one null ('\0') byte, but its format
is otherwise undefined.  For this assignment, we will be skipping over
this field, if present, and ignoring its contents.

Audio data begins immediately following the annotation field (or immediately
following the header, if there is no annotation field).  The audio data occurs
as a sequence of *frames*, where each frame contains data for one sample on each
of the audio channels.
The size of a frame therefore depends both on the sample encoding and on
the number of channels.  For example, if the sample encoding is 16-bit PCM
(i.e. two bytes per sample) and the number of channels is two, then the number
of bytes in a frame will be 2 * 2 = 4.  If the sample encoding is 32-bit PCM
(i.e. four bytes per sample) and the number of channels is two, then the number
of bytes in a frame will be 2 * 4 = 8. 

Within a frame, the sample data for each channel occurs in sequence.
For example, in case of 16-bit PCM encoded stereo, the first two bytes of each
frame represents a single sample for channel 0 and the second two bytes
represents a single sample for channel 1.  Samples are signed values encoded
in two's-complement and are presented in big-endian (most significant byte first)
byte order.

<pre>
 +-----------------------------------+-----------------------------------+
 | Sample 0 MSB | ... | Sample 0 LSB | Sample 1 MSB | ... | Sample 1 LSB |
 +-----------------------------------+-----------------------------------+
</pre>

## Implementation

You will need to provide implementations for the following functions,
for which more detailed specifications are given in the file <code>include/audio.h</code>:
and for which stubs have been given in the file `src/audio.c`:

```c
int audio_read_header(FILE *in, AUDIO_HEADER *hp);
int audio_write_header(FILE *out, AUDIO_HEADER *hp);
int audio_read_sample(FILE *in, int16_t *samplep);
int audio_write_sample(FILE *out, int16_t sample);
```

# Part 3: Dual-Tone, Multi-Frequency Signaling

*Dual-Tone, Multi-Frequency* (DTMF) is a scheme for sending data over an
audio channel, such as a telephone connection, where it might occur in conjunction
with some other audio transmission, typically speech.
It was developed by the Bell System for use in push-button telephones,
under the *Touch-Tone* trademark.
These days it is perhaps becoming less likely that everyone has actually
used a push-button telephone, but even if you haven't the tones that are generated
when the buttons are pressed will probably be familiar to you.
A good article that provides basic information on DTMF signaling can be found
on [Wikipedia](https://en.wikipedia.org/wiki/Dual-tone_multi-frequency_signaling).
You should use this article to fill in any gaps in my presentation here.

The basic idea of DTMF signalling is to use various combinations of eight
pure sine-wave signals to represent symbols from a 16-character alphabet.
The scheme can be summarized as follows:
<pre>
              1209 Hz 1336 Hz 1477 Hz 1633 Hz
       697 Hz   1       2       3       A
       770 Hz   4       5       6       B
       852 Hz   7       8       9       C
       941 Hz   *       0       #       D
</pre>
There are four *row frequencies*: 697 Hz, 770 Hz, 852 Hz, and 941 Hz,
and four *column frequencies*: 1209 Hz, 1336 Hz, 1477 Hz, and 1633 Hz.
Each of the sixteen combinations of one row frequency and one column
frequency corresponds to one of the 16 symbols shown in the table.
The first three columns will probably be familiar to you as the usual
telephone keypad.  The last column is also part of the original design,
though most telephone keypads did not include it.
The Wikipedia article linked above has various audio clips that show
how the DTMF tones sound individually and when placed in rapid succession
to transmit data.
The rationale underlying the scheme is that because it uses audio
frequencies from the normal range found in speech, the signals could
be transmitted over existing telephone circuits.  The frequencies were
carefully chosen to make it as easy as possible to distinguish signaling
tones from speech that might be going on simultaneously.

There are strict specifications for the generation and detection of DTMF
signals.  As it is not our intention here to build an "industrial-strength"
implementation, we will treat these specifications somewhat loosely in order
to avoid over-complicating your programming task.
Broadly speaking, our goal is to be able to detect DTMF symbols occurring
at a maximum rate of 10 symbols per second in audio that has been encoded
at a sample rate of 8000Hz using 16-bit signed-linear PCM encoding.
An industrial-strength implementation would take extra pains to allow for
some degradation of the signals that might be introduced by the transmission
channels, and to limit "false positive" detection of DTMF signals in the presence
of speech; but we won't worry so much about such things.

# Part 4: The Goertzel Algorithm

The theoretical basis for decoding DTMF, as well as for many other signal-processing
problems, is *Fourier analysis*, which basically involves the decomposition of a
signal into a sum (or more generally, an integral) of sinusoidal *frequency components*.
The coefficients that appear in the sum (or integral) are complex numbers that
express the *magnitude* and *phase* of each frequency component.
The collection of these coefficients comprises the *Fourier transform* of the
signal, and it can be shown that, under suitable assumptions, a given signal can be
reconstructed from its Fourier transform.

The original scheme for decoding DTMF used a set of *matched filters*.
Each filter was an analog circuit that was designed to only pass
frequency components of an audio signal in a narrow range about one of the
row or column frequencies.  By playing audio into a bank consisting of one
filter for each of the DTMF frequencies, the presence or absence of a DTMF
symbol at any time could be determined.
Later, the use of analog circuits was supplanted by *digital signal processing*
techniques; however, the basic idea is still the same: filter the audio to
determine the strength of the signal at frequency components close to each
of the DTMF frequencies, then use the resulting information to decide whether
or not a DTMF symbol is present at a given time.

Typically, DTMF signal detection is accomplished digitally using the
*Goertzel Algorithm* or *Goertzel filters*.
The *Goertzel Algorithm* uses a *digital filter* to input audio samples and
produce as output the Fourier coefficient of the audio signal at a chosen frequency.
Technically (you don't have to fully understand this), the output of the algorithm
is the value of the *discrete time Fourier transform* (DTFT) of the input signal
at the chosen frequency.  This value is a complex number (y, say) which encodes
*magnitude* and *phase* information of the specified frequency component.
For the purposes of DTMF detection, only the magnitude (*i.e.* "strength") information
is important.  The algorithm we use is a "generalized" Goertzel algorithm, which in
contrast to the original Goertzel algorithm can be used to obtain the component of the
input signal at an arbitrary real-valued frequency, rather than the just the component
at frequencies rounded to specific integer "frequency bins".

A good description of the original Goertzel algorithm and its generalized version
is given [in this paper](https://link.springer.com/article/10.1186/1687-6180-2012-56).
Although some background in digital signal processing is needed in order to understand
the paper fully, what will primarily be useful for us is the pseudocode given in Figure 4
of the paper.  This pseudocode describes the generalized Goertzel algorithm, which inputs
a signal x consisting of N real-valued samples x[0], x[1], ... x[N-1],
together with a real-valued frequency "index" k, and outputs a complex value y that
represents the value of the DTFT of the input signal for the "k-th" frequency component
&omega;<sub>k</sub>.
Although any real number is valid as the value of the "index" k, for us the useful range
of values will be [0, N/2).  A given value of k represents a frequency &omega;<sub>k</sub>
of k/N cycles per sampling interval.  Thus, k = 0 represents frequency 0
(the constant or "DC" component), k = 1 represents the "fundamental frequency" of the
signal x, which takes N samples to cover one cycle, k = 2 represents the "second harmonic"
of the fundamental frequency, which covers one cycle in N/2 samples, and k = N/2
represents the "Nyquist frequency", which covers one cycle every 2 samples.

As an example of how to calculate the proper value of k, suppose we are considering
a signal x that consists of N=4000 samples taken at a sampling rate of 8000Hz,
so that the duration of x is 0.5 sec.  Furthermore suppose we are interested in using
the Goertzel algorithm to compute the strength of the component of x at the first DTMF
row frequency of 697 Hz.  The frequency of 697 Hz covers 697/8000 = 0.087124 cycles
per sampling interval.  So we would use a value of k satisfying k/N = k/4000 = 0.087124;
that is, k = 348.5.  More generally, if the sampling rate is R Hz, the number of samples
in the signal x is N, and we are interested in a component frequency of F Hz, then
we would take k = N * F / R.

Once having understood where k comes from, the pseudocode in Figure 4 of the cited paper
is not difficult to understand.  The digital filter has three state variables, called
s<sub>0</sub>, s<sub>1</sub>, and s<sub>2</sub>.
The state variables are given initial values of 0.  The filter operates by consuming the samples
of the signal x one at a time (see "Main loop").  The i-th iteration of the main loop updates
the state variable s<sub>0</sub> according to a simple formula that depends on the i-th sample
value x[i], the current values of the state variables s<sub>1</sub>, and s<sub>2</sub>,
and a real-valued coefficient B that depends on k and N and is the same at each iteration.
Then, the state variables are "shifted" in preparation for the next iteration:
the value of s<sub>2</sub> is discarded and replaced by the value of s<sub>1</sub>,
and the value of s<sub>1</sub> is replaced by the newly computed value of s<sub>0</sub>.
In technical terms, this is a *second-order* filter, because the values of two of the state
variables (*i.e.* s<sub>1</sub> and s<sub>2</sub>) are carried from one iteration to the next.
For the generalized Goertzel algorithm we will be using, the main loop executes for
N-1 iterations, consuming samples x[0], x[1], ..., x[N-2].

> :scream:  In our version of the Goertzel algorithm we will assume that the sample
> values of the input signal are real values in the range [-1.0, 1.0], which are given
> in double-precision floating point.  Since the sample values in the audio input will
> be 16-bit signed integer values in the range [-INT16_MAX, INT16_MAX], before supplying
> these values to the Goetzel algorithm they must first be converted to double-precision
> floating point and divided by INT16_MAX to obtain values in the range [-1.0, 1.0].

The last step of the algorithm (see "Finalizing calculations") consumes the final sample x[N-1]
and produces a complex number y.  Note that, in Figure 4, constants C, D, and the output
value y are complex numbers, so since C does not have any complex number data type,
each complex number has to be explicitly represented by its real and imaginary part,
and the indicated arithmetic operations have to be performed appropriately on these parts.
The symbol "j" is the engineer's notation for what the mathematician would
refer to as "i", the imaginary unit.  In case you have forgotten what you probably should
have learned in Calculus class, the complex exponential C = e<sup>-jA</sup> evaluates to
cos A - j sin A in the cartesian representation, and similarly for D.
Addition of complex numbers a + j b and c + j d is given by (a+c) + j (b+d).
Multiplication of complex numbers a + j b and c + j d is given by (ac - bd) + j (ad + bc).
You will need to do all these calculations in double-precision floating point
(C type `double`).

As previously discussed, we will only be interested in the squared magnitude |y|<sup>2</sup>
of the complex value y.  The squared magnitude of a complex number a + j b is given by
(a + j b) (a - j b) or a<sup>2</sup> + b<sup>2</sup>.
Additionally, our version of the generalized Goertzel algorithm will return the value
2|y|<sup>2</sup>, rather than |y|<sup>2</sup>.  This is for the technical reason that
the value |y|<sup>2</sup> represents the "strength" or "energy" of the signal x at the
"positive frequency" &omega;<sub>k</sub>, but there is an equal amount of "energy" at the
"negative frequency" &omega;<sub>-k</sub>, so to have the result of the algorithm correspond
to the total energy at the specified frequency, we double the result.
The calculation of 2|y|<sup>2</sup> is an additional step that we will use,
which is not shown in the pseudocode in Figure 4 of the cited paper.

## Implementation

The following structure type has been defined for you (in `include/goertzel.h`)
for storing the state of an instance of the generalized Goertzel algorithm:

```c
typedef struct goertzel_state {
    uint32_t N;      // Number of samples in the signal to be analyzed.
    double k;        // Real-valued "index" of the frequency component
                     // whose strength is to be computed.
    double A;	     // Intermediate value used to compute B and C.
    double B;        // Multiplicative constant applied at each iteration.
    double s0;       // Goertzel filter state variables.
    double s1;
    double s2;
} GOERTZEL_STATE;
```

You will need to use this structure because your DTMF detection code will need to
run eight instances of the Goertzel algorithm simultaneously (one for each of the
DTMF frequencies) and since you are not permitted to declare any arrays in your own
code, an array of eight instances of this structure has been pre-declared for you
in `include/const.h`:

```c
GOERTZEL_STATE goertzel_state[NUM_DTMF_FREQS];
```

You will need to give implementations for the following functions, for which
more detailed specifications have been given in the file `include/goertzel.h`
and for which stubs have been given in the file `src/goertzel.c`:

```c
void goertzel_init(GOERTZEL_STATE *gp, uint32_t N, double k);
void goertzel_step(GOERTZEL_STATE *gp, double x);
double goertzel_strength(GOERTZEL_STATE *gp, double x);
```

Implement these functions by following the discussion above and the pseudocode
in Figure 4 of the cited paper.

Before attempting to use your implementations of these functions in the detection of
DTMF signals, you should verify that they are producing meaningful results.
To help you, I have provided a very basic unit test, which uses one cycle of a cosine
signal over N=100 samples as input, and checks the output of the Goertzel algorithm
for k=0, k=1, and k=2.
As a more advanced test (I have not provided code for this), if you run instances
of the Goertzel algorithm on the first 1000 samples in the file `rsrc/dtmf_0_500ms.au`
(using N=1000), you should get the following results:

- 697 Hz: 0.000003
- 770 Hz: 0.000007
- 852 Hz: 0.000007
- 941 Hz: 0.031544
- 1209 Hz: 0.000002
- 1336 Hz: 0.031479
- 1477 Hz: 0.000009
- 1633 Hz: 0.000001

# Part 5: "Top-Level" Functions

## DTMF Generation

For DTMF generation, you will need to give an implementation for the following function,
for which the following specification and a stub has been given in the file `src/dtmf.c`:

```c
/**
 * DTMF generation main function.
 * DTMF events are read (in textual tab-separated format) from the specified
 * input stream and audio data of a specified duration is written to the specified
 * output stream.  The DTMF events must be non-overlapping, in increasing order of
 * start index, and must lie completely within the specified duration.
 * The sample produced at a particular index will either be zero, if the index
 * does not lie between the start and end index of one of the DTMF events, or else
 * it will be a synthesized sample of the DTMF tone corresponding to the event in
 * which the index lies.
 *
 *  @param events_in  Stream from which to read DTMF events.
 *  @param audio_out  Stream to which to write audio header and sample data.
 *  @param length  Number of audio samples to be written.
 *  @return 0 if the header and specified number of samples are written successfully,
 *  EOF otherwise.
 */
int dtmf_generate(FILE *events_in, FILE *audio_out, uint32_t length);
```

The data to be written to `audio_out` should be in Sun Audio format (a binary format)
as described in a previous section.
The data to be read from `events_in` will be in a textual format that consists of lines
of input, each of which is terminated by a single newline (`\n`) character.
Each line of input will be a "DTMF event" consisting of three fields separated by
a tab (`\t`) character.  The first two fields are integer fields that give the starting
and ending index of the DTMF event.  The third field is a single-character field that
gives the DTMF symbol for the event.  This symbol is one of those listed in the following
array, defined in `include/dtmf_static.h`:

```c
uint8_t dtmf_symbol_names[NUM_DTMF_ROW_FREQS][NUM_DTMF_COL_FREQS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
```

An example of input acceptable to `dtmf_generate()` is the following:

```
0	1000	0
2000	3000	1
4000	5000	2
6000	7000	3
```

Note that each space in the above is actually represented by a single tab character
and each line is terminated by a single newline character.

The audio produced by the `dtmf_generate()` function should consist of a valid
audio header, follows by `length` 16-bit audio samples.
For each sample index `i` in the range `[0, length)`, if there is a DTMF
event with start index `s` and end index `e` such that `i` is in `[s, e)`,
then the sample at index `i` should be the synthesized sample at index i
of the DTMF tone for the specified symbol.
If the row frequency for such a tone is `Fr` and the column frequency is `Fc`,
then the sample should be computed by computing the following two values
(double-precision floating point) in the range [-1.0, 1.0]:

```c
cos(2.0 * M_PI * Fr * i / AUDIO_FRAME_RATE)
cos(2.0 * M_PI * Fc * i / AUDIO_FRAME_RATE)
```

then adding these two values together, each weighted by `0.5` to again produce a value
in the range `[-1.0, 1.0]`, multiplying by `INT16_MAX` to obtain a value in the
range `[-INT16_MAX, INT16_MAX]` and finally truncating this value to type `int16_t`
to obtain a 16-bit integer value in the same range.
For each sample index `i` that is not in the range covered by a DTMF event,
the sample value at index i should be 0.

If a "noise file" has been specified in the command-line options, then the construction
of the audio samples is to be modified as follows:  at each index i, the sample at that
index should be constructed by combining the DTMF sample computed as described above
with the "noise" sample at index i.  The combination of the two samples should be
performed so as to respect the noise-to-DTMF ratio specified by the "level" parameter,
as well as the total loudness of the resulting combined signal.
That is, the noise sample should be weighted by a factor `w` in the range [0.0, 1.0] and
the DTMF sample should be weighted by the factor `1-w`, where `w` is chosen so that the
quantity `10 * log (w/(1-w))` equals the specified "level" value (which is given in dB).
If end-of-file is reached on the noise file before the required total number of samples
has been generated, then the samples are constructed as if the noise file were extended
by samples with value 0.
If the end of the DTMF event input is reached before the required total number of samples
has been generated, then the samples are constructed using DTMF sample values of 0.

## DTMF Detection

For DTMF detection, you will need to give an implementation for the following function,
for which the following specification and a stub has been given in the file `src/dtmf.c`:

```c
/**
 * DTMF detection main function.
 * This function first reads and validates an audio header from the specified input stream.
 * The value in the data size field of the header is ignored, as is any annotation data that
 * might occur after the header.
 *
 * This function then reads audio sample data from the input stream, partititions the audio samples
 * into successive blocks of block_size samples, and for each block determines whether or not
 * a DTMF tone is present in that block.  When a DTMF tone is detected in a block, the starting index
 * of that block is recorded as the beginning of a "DTMF event".  As long as the same DTMF tone is
 * present in subsequent blocks, the duration of the current DTMF event is extended.  As soon as a
 * block is encountered in which the same DTMF tone is not present, either because no DTMF tone is
 * present in that block or a different tone is present, then the starting index of that block
 * is recorded as the ending index of the current DTMF event.  If the duration of the now-completed
 * DTMF event is greater than or equal to MIN_DTMF_DURATION, then a line of text representing
 * this DTMF event in tab-separated format is emitted to the output stream. If the duration of the
 * DTMF event is less that MIN_DTMF_DURATION, then the event is discarded and nothing is emitted
 * to the output stream.  When the end of audio input is reached, then the total number of samples
 * read is used as the ending index of any current DTMF event and this final event is emitted
 * if its length is at least MIN_DTMF_DURATION.
 *
 *   @param audio_in  Input stream from which to read audio header and sample data.
 *   @param events_out  Output stream to which DTMF events are to be written.
 *   @return 0  If reading of audio and writing of DTMF events is sucessful, EOF otherwise.
 */
int dtmf_detect(FILE *audio_in, FILE *events_out);
```

In order to determine whether a DTMF tone is present in a block, the results of running
eight instances of the generalized Goertzel algorithm (one for each DTMF frequency) are
first computed for the samples in that block.  From this information, the strongest row
frequency component and the strongest column frequency component are identified.
For a DTMF tone to be regarded as present in the block, the following criteria must be
satisfied:

- The sum of the strengths of the strongest row and column frequency components must be
  no more than -20dB lower than the absolute maximum possible signal strength
  (which for us is 1.0, due to our assumption that the sample values lie in the range `[-1.0, 1.0]`).
  Thus, stated another way, the requirement is that the sum of the strengths of the
  strongest row and column frequency components must be at least 0.01.
- The ratio of the strengths of the strongest row frequency component and the strongest column
  frequency component must be between -4dB and 4dB.  This difference in strength between the two
  components is referred to in technical jargon as *twist*.
- The strongest row frequency component must be at least 6dB stronger than the other
  row frequency components, and similarly for the strongest column frequency components.

# Part 6: Running the Completed Program

In any of its operating modes, the `dtmf` program reads from `stdin` and writes
to `stdout`.  Since audio data is in a binary format, it will not be useful to try to
input audio data directly from the terminal or to display audio output directly to
the terminal. Instead, the program can be run using **input and output redirection**
as has already been discussed; *e.g.*:

```
$ bin/dtmf -g -t 100 < dtmf.txt > audio.au
```

This will cause the input to the program to be redirected from the text file
`dtmf.txt` and the output from the program to be redirected to the file `audio.au`.
Since `dtmf.txt` is a text file, its contents can be displayed on the terminal or
edited with a text editor.
For example, this file might have contained the following line of text:

```
100	200	0
```

When supplied as input to `bin/dtmf` as above, the output file produced is an
audio file that contains 100 ms of audio data (i.e. 800 samples at 8000 Hz),
with a DTMF tone for symbol '0' starting at index 100 and continuing to index 199.
For debugging purposes, the contents of `audio.au` can be viewed using the
`od` ("octal dump") command:

```
$ od -t x1 audio.au
0000000 2e 73 6e 64 00 00 00 18 00 00 06 40 00 00 00 03
0000020 00 00 1f 40 00 00 00 01 00 00 00 00 00 00 00 00
0000040 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
*
0000340 f8 a0 2c ce 3f 44 21 a6 ed ce cd dd d7 22 fa 4f
0000360 15 f7 17 24 07 c0 fe e0 05 95 0d c6 04 27 e9 63
0000400 d6 1d e3 65 0e 8e 36 55 36 ff 0c 06 d6 ab c1 b6
0000420 dc 2a 0d 56 2c a6 25 c8 07 45 ef f0 ef d8 fc 04
0000440 ff 3b f4 04 eb 50 f8 83 18 6c 2f 3d 22 3b f4 96
0000460 c9 57 c5 dc ef 77 25 27 3c 28 25 0b f7 3e d9 61
0000500 de 5d f8 2b 0a 9f 08 df ff 52 01 9f 11 b6 1b a3
0000520 0b 94 e6 e0 cc 4c d8 54 07 a0 35 d6 3c 43 14 b4
0000540 df 8d c7 15 da d2 04 a8 20 3f 1c 89 07 55 fa 2f
0000560 fe 82 06 1e fe 99 e9 67 dd 96 ef bc 18 bc 37 26
0000600 2c b9 fc a1 cc 12 c2 bb e7 aa 1b 6b 34 af 24 32
0000620 fe cc e6 43 e9 63 fa 3e 01 cd fa 3c f3 d7 00 25
0000640 1a 87 28 cf 15 60 e8 af 00 00 00 00 00 00 00 00
0000660 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
*
0003120 00 00 00 00 00 00 00 00
0003130
```

This shows a file that is 1624 bytes in length (3130 in octal).
The seven-digit number at the beginning of each line is the offset
(in octal) from the beginning of the file.
The remainder of each line shows the value (in hexadecimal) of each
successive byte of the file.
The audio header occupies the first one and one-half lines.
This is followed by a number of bytes of zeros, which correspond to
audio sample values of zero, up to offset 340 octal (224 decimal),
where nonzero sample data corresponding to the specified DTMF tone
starts.  Zero samples start again at offset 650 octal (424 decimal,
so there is a total of (424 - 224) / 2 = 100 nonzero samples.

If you use the above command with an `audio.au` that is much longer, there would
be so much output that the first few lines would be lost off of the top of the screen.
To avoid this, you can **pipe** the output to a program called `less`:

```
$ od -t x1 audio.au | less
```

This will display only the first screenful of the output and give you the
ability to scan forward and backward to see different parts of the output.
Type `h` at the `less` prompt to get help information on what you can do
with it.  Type `q` at the prompt to exit `less`.

Alternatively, the output of the program can be redirected via a pipe to
the `od` command, without using any output file:

```
$ bin/dtmf -g -t 100 < dtmf.txt | od -t x1 | less
```

## Testing Your Program

Pipes can be used to test your program by generating DTMF audio and
then immediately applying detection to it:

```
$ cat dtmf_in.txt | bin/dtmf -g | bin/dtmf -d > dtmf_out.txt
```

If `dtmf_in.txt` contains the following lines of input:

```
0		2000	0
2000	4000	1
```

then if the program is working properly, the contents of `dtmf_out.txt` should be
identical to those of `dtmf_in.txt`.

> :nerd:  That the output is exactly identical to the input only occurs in this case
> because the block size (100) used for detecting DTMF tones aligns with the actual
> ranges of the DTMF tones specified in the input file.  If a different block size
> is used in detection, the output will be slightly different.

It is useful to be able to compare two files to see if they have the same content.
 The `diff` command (use `man diff` to read the manual page) is useful for comparison
of text files, but not particularly useful for comparing binary files such as
compressed data files.  However the `cmp` command can be used to perform a
byte-by-byte comparison of two files, regardless of their content:

```
$ cmp file1 file2
```

If the files have identical content, `cmp` exits silently.
If one file is shorter than the other, but the content is otherwise identical,
`cmp` will report that it has reached `EOF` on the shorter file.
Finally, if the files disagree at some point, `cmp` will report the
offset of the first byte at which the files disagree.
If the `-l` flag is given, `cmp` will report all disagreements between the
two files.

We can take this a step further and run an entire test without using any files:

```
$ cmp -l <(cat dtmf_in.txt) <(cat dtmf_in.txt | bin/dtmf -g | bin/dtmf -d)
```

This compares the original file `dtmf_in.txt` with the result of taking that file,
using it as the specification for the synthesis of an audio file, and then applying
DTMF detection to the result.
Because both files are identical, `cmp` outputs nothing.

> :nerd: `<(...)` is known as **process substitution**. It is allows the output of the
> program(s) inside the parentheses to appear as a file for the outer program.

> :nerd: `cat` is a command that outputs a file to `stdout`.

## Unit Testing

Unit testing is a part of the development process in which small testable
sections of a program (units) are tested individually to ensure that they are
all functioning properly. This is a very common practice in industry and is
often a requested skill by companies hiring graduates.

> :nerd: Some developers consider testing to be so important that they use a
> work flow called **test driven development**. In TDD, requirements are turned into
> failing unit tests. The goal is then to write code to make these tests pass.

This semester, we will be using a C unit testing framework called
[Criterion](https://github.com/Snaipe/Criterion), which will give you some
exposure to unit testing. We have provided a basic set of test cases for this
assignment.

The provided tests are in the `tests/hw1_tests.c` file. These tests do the
following:

- `validargs_help_test` ensures that `validargs` sets the help bit
correctly when the `-h` flag is passed in.

- `validargs_generate_test` ensures that `validargs` sets the generate-mode bit
correctly when the `-g` flag is passed in and sets the `noise_file` variable
correctly when the `-n` flag is passed.

- `validargs_detect_test` ensures that `validargs` sets the detect-mode bit
correctly when the `-d` flag is passed in and sets the `block_size` variable
correctly when the `-b` flag is passed.

- `validargs_error_test` ensures that `validargs` returns an error when the
`-b` (blocksize) flag is specified together with the `-g` (generate) flag.

- `help_system_test` uses the `system` syscall to execute your program through
Bash and checks to see that your program returns with `EXIT_SUCCESS`.

- `goertzel_basic_test` performs a basic (but by no means exhaustive) test of
the output of the Goertzel algorithm.

### Compiling and Running Tests

When you compile your program with `make`, a `dtmf_tests` executable will be
created in your `bin` directory alongside the `dtmf` executable. Running this
executable from the `hw1` directory with the command `bin/dtmf_tests` will run
the unit tests described above and print the test outputs to `stdout`. To obtain
more information about each test run, you can use the verbose print option:
`bin/dtmf_tests --verbose=0`.

The tests we have provided are very minimal and are meant as a starting point
for you to learn about Criterion, not to fully test your homework. You may write
your own additional tests in `tests/hw1_tests.c`. However, this is not required
for this assignment. Criterion documentation for writing your own tests can be
found [here](http://criterion.readthedocs.io/en/master/).

### Test Files

In the `rsrc` directory I have provided several files that you might find useful
in testing your code.  They are:

- `941Hz_1sec.au`  An audio file consisting of one second (8000 samples) of a
pure sinusoidal tone of frequency 941 Hz.

- `dtmf_0_500ms.au`  An audio file consisting of 500ms (4000 samples) of the
DTMF tone for symbol "0" (941 Hz + 1336 Hz).

- `dtmf_0_500ms.txt`  A text file containing the specification of a single "DTMF event"
of 4000 samples of the DTMF tone for symbol "0".

- `dtmf_all.au`  An audio file of 1.6 seconds in duration, in which all 16 DTMF
tones occur.  Each tone is on for 50ms and off for 50ms.

- `dtmf_all.txt`  A text file used as the input for the generation of the `dtmf_all.au`
audio file.

- `white_noise_10s.au`  An audio file consisting of 10 seconds of "white noise".

### Useful Tools

There are two easily available tools that you might find useful in debugging and testing
your program.  The first is "audacity", which is a very nice GUI application to do all sorts
of things with audio.  You can install it using `sudo apt install audacity`.
Of particular interest is the `Analyze>Spectrum` menu item, which computes the frequency
spectrum of an audio clip and displays it.  If you apply this analysis to a DTMF tone,
you will clearly see the peaks corresponding to the two DTMF frequencies.
The algorithm that we are using to do DTMF detection is essentially a special case of this
analysis, in which the frequencies of interest are specified in advance.
The second is "sox", which is a command-line tool for converting between various audio
file formats and performing various transformations on audio files.
You can install it using `sudo apt install sox`.  This can be used to convert audio files
between the particular `.au` format that we are using and other formats.  There are quite
a number of command-line options, so you have to read the man page carefully.

# Hand-in instructions

**TEST YOUR PROGRAM VIGOROUSLY BEFORE SUBMISSION!**

Make sure that you have implemented all the required functions specifed in `const.h`.

Make sure that you have adhered to the restrictions (no array brackets, no prohibited
header files, no modifications to files that say "DO NOT MODIFY" at the beginning,
no functions other than `main()` in `main.c`) set out in this assignment document.

Make sure your directory tree looks basically like it did when you started
(there could possibly be additional files that you added, but the original organization
should be maintained) and that your homework compiles (you should be sure to try compiling
with both `make clean all` and `make clean debug` because there are certain errors that can
occur one way but not the other).

This homework's tag is: `hw1`

`$ git submit hw1`

> :nerd: When writing your program try to comment as much as possible. Try to
> stay consistent with your formatting. It is much easier for your TA and the
> professor to help you if we can figure out what your code does quickly!

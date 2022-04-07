
<!-- [![xelp](./img/xelp-prompt-med.png)](http://www.deftio.com/xelp)  -->

<a href="www.deftio.com/xelp"><img src="./img/xelp-prompt.png" width="30%"></img></a><br>

[![License](https://img.shields.io/badge/License-BSD%202--Clause-blue.svg)](https://opensource.org/licenses/BSD-2-Clause)


# Xelp - A C command line interpreter and script parser


## About XELP

xelp is a simple combined command line interpreter / script interpreter for embedded projects which run "on the metal" or in may not have a formal OS.  This allows the programmer to have a script interpeter available for debugging with the syntax of a command line.  Xelp is meant to work seamlessly with pure C (also C++) so that the programmer can add their own commands which are then available at run time from the xelp script environment.  Passing data from xelp to C functions and from C to xelp scripts is supported.    The xelp parser can run without dynamic memory support which allows its use in interrupts or memory constrained environments such as machine monitors.

Xelp includes a small set of built-in commands for memory operations, viewing, and pointer operations which can be optionally compiled in.

Written in pure C with function pointers for adding user called fuctions.  
Compiled sizes range from 900 - 4k bytes depending on options chosen, platform and architecture. 


## Features

* Command Line Interface (CLI) with C language function calls 
* Scriptable commands  
	* Anything run at commandline or menus can also be called as a script.  
	* Scripts can be ROM-able strings
	* No script strings are modified during execution by core parser / interpreter (eg no "strtok() style" procecssing.)
* Programmer supplied C-language functions can be called from command line or from script
* Each function can also have an optional help string.
* Single-key mode for immediate menus or actions (w/o having to pressing ENTER) 
	* programmer supplied actions (key presses or cmds) switch btw single key or cmd-line modes.
* Thru-mode allows redirection of key strokes to another peripheral w/o any parsing (useful for debugging modems or other peripherals with their own command sets) 
	* thru-mode is switchable on the fly at runtime and can be redirected or compiled out.
	* example: use thru-mode to on-the-fly connect to a modem and type in AT commands then switch back to CLI mode when done allowing graceful switching between CLI and peripheral command sets
* Tokenizer output available for user supplied functions that need to parse params
* Str2Int tokenizer allows converting numbers, and hex digits to integers.  (123 --> int, or 123h --> int)
* Single line comments via # symbol  (useful for scripts). Tabs also supported for indentation readability in scripts.  
* Can be configured, at compile time to save space.  See tables in docs for compiled sizes.   See xelpcfg.h to control these options:
	* (KEY Mode) key-only mode (no CLI just key-press menus) 
	* (CLI Mode) cli-only mode (command line prompt w destructive backspace handling)
	* (THR Mode) thru support optional (redirect all keys to another peripheral w/o processing) 
	* help function optional (remove to save space, see table)
	* Override/select key mappings (enter, backspace, etc), also escape char mappings 
	* Settable prompt for CLI (e.g. "myPrompt>")
* Supports "quoted strings" in command line (treats as single token), escapes for command line via '`', escapes for quoted strings via '\'.  All escape chars are overridable at compile time.
* No dynamic memory needed for CLI / script interpreter / tokenizer / command dispatch (eg no malloc/free new/delete)  
* No globals or global state -- all state is stored in an instance so several instances can be run at the same time
	* allows separate XELP instances to be attached to different serial ports for example.
* Reentrant provided same instance is not used as a CLI for 2 competing threads.  Scripts are reentrant by default unless user supplied functions are not reentrant.
* Platform independant
	* No library support required (stdio.h, string.h etc not needed).  
	* Entirely in C (no assembly) for portability. C89, C90, C99, ANSI compliant (for dealing w older compilers)
	* Simple platform asbtraction layer ("HAL") for porting uses 5 function pointers. 
* OSI approved open-source - BSD-2 License  

	
## usage:
```C
#include "xelp.h"			/* in the file where xelp calls are to be made */
```

compile and link xelp.c
other platform support architecture
set default declarations (key maps etc)

no other dependancies are required for embedded operations.

## Debugging on Linux / POSIX
Currently on linux the "curses" library (actually ncurses) is used to trap key presses for debugging purposes.  This can be installed as follows on debian.
```bash
sudo apt-get install libncurses5-dev

make

```
Note that the example executable is called xelp-example.out this can be run to see some basic xelp commands.  However the real value comes out when user supplied and callable functions are added e.g. 
```C
int runMotor (...);
// and this is registered via 
```

There is no binary distribution - include the source in your project along with the appropriate types file and xelpcfg.h file to control options.


## Platform Support

xelp has been compiled and run (with no warnings for the following processors / platforms).

### Architecture Support Table

| Processor Arch  | Compiler  | Platform        | Arch           |
|-----------------|-----------|-----------------|----------------|
| 80x86-64        | GCC 4.8   | Linux/Ubuntu    | 64 bit     	 |
| 80x86-64        | Vis C++   | Windows-10      | 64 bit         |
| 80x86-32        | GCC 4.8   | Linux/Ubuntu    | 32 bit  	     |
| 80x86-32        | Vis C++   | Win XP          | 32 bit         |
| 8086 / 186/286  | Turbo C++ | MS DOS          | 16 bit         |
| ARM32           | GCC       | MBED /Rasp Pi   | 32 bit         | 
| ARM32-Thumb	  | GCC       | "               | 32 bit         |
| MSP430          | GCC       |                 | 16 bit         |
| 6502            | cc65      | Super NES / C64 |  8 bit         |   
| PIC18Fxxx       | SDCC      |                 | 16 bit         |
| 8051x           | SDCC      |                 |  8 bit         |
| 68HC11/12       | GCC       |                 |  8 bit         |


## Testing
Testing has been done on linux as the main test driver though xelp has been run as part of private projects on other platforms / processors.

To run the unit tests:
```bash
make xelp-unit-tests.out 

./xelp-unit-tests.out  

```
Note to make sure the text executable is runnable by this command if necessary:

```bash
chmod +x xelp-unit-tests.out
```

## FAQ / Questions

Q: I just want to able to use keypresses without being in "ESC" mode or "CLI" mode.
A: just compile with the "XELP_ENABLE_KEY" #define in xelpcfg.h and comment out "XELP_ENABLE_FULL" in xelpcfg.h

Q: 


## License

(OSI Approved BSD 2-clause)

Copyright (c) 2011-2016, M. A. Chatterjee <deftio at deftio dot com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



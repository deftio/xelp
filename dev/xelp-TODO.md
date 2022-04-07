# xelp  (a CLI library and extensible script interpreter) TODO list
(c)  2012 M. A. Chatterjee 

Next up:

## Feature Implementation
	[0] nextTok() --> get next parseable token.  change params to *start, *len rather than several  char *s
	[x] strComp(str1,len1,str2)  #used in parser to see if something is a command in the dictionary
	[x] help() formmatting
	[x] change mode key-seq --> CMD mode (e.g CTRL-P)  
	[x] change mode key-seq --> passthru mode (e.g CTRL-T)
	[x] change mode key-seq --> single-key mode (e.g Esc) 
	[x] default state machine with Key, CLI modes
	[x] default state machine with Key only mode
	[x] default state machine with CLI only mode
	[ ] make command buffer size user settable.  e.g user make (static) allocation char myBuf[80] and passed it to xelp (TBD)
	[ ] combine mEchoState, mCurMode in to single mFlags variable (save stack space)
	[x] overidable key-map via #defines
		[x] default destructive backspace (e.g. xelp treats as destructive bsp)
		[x] default key-combo to enter CMD mode
		[x] default key-combo to enter parse mode
		[x] default terminal statement char (e.g. ";") //may need to be compiled in due to state machine
		[ ] default intra statement char (e.g. ":")  or use [] like TCL 
		[x] default cmd line esc char (`)  //backtick
		[x] default quoted esc char (\)
	[x] funcPtr to map output chars
	[x] funcPtr to map input chars
	[x] funcPtr to map pass-thru chars  (allows chars typed to be sent to .. somewhere else)
	[ ] funcPtr for errors (e.g. mpErr(int errCode))  // when the parser is exec'ing a bunch o stuff and something fails it calls this
		[ ]	make dissolvable macro to save stack space
	[x] function to print string (optional len or nul-char)  eg (DCIOut(char* buf, maxlen))  if maxlen=pos int print maxlen chars. if maxlen=0, prints null terminated chars
	[x] parser-gen state machine stand alone tool.  e.g. takes #defines, full char state machine, creates compact state string and jump table.
	[x]	for certain symbols, such as quoted ESC, CLI ESC emits #define instead of hard value.  This allows user to just #define in their config what those chars are.
	[0] function set CR/LF emission characteristic -- decide (won't do)
	[ ] catchall function for when command isn't recognzied ==> f(str, len)  ==> allows different parser to be called such as making primitive http response
	[ ] parser grammar formalize
		<command><space><args><terminal>  --> terminal is ';' from the POV of parser
			-->` (backtick)  allows escaping of terminal chars so that if we include math ops ^ isn't "taken" out for paths \ or / aren't taken
			-->`:  colon is clause separator for if statements  
			--> parse not attempted (in parse mode) until <CR>  
	[ ]	developer examples C / CLI
		[ ] how to make Key mode only (menu system, min build).  Add help.   Add scripting several key commands via loop in C.
		[ ] make a CLI mode only (w help) application. show abstraction set up with backspace.
		[ ] make a CLI+Key mode only app (w help).  Show scripting.
		[ ] how to make a function in C (and call it from the intrepreter at command line or from script)
		[ ] how to make script and call the interpreter.  and from inside a C func.  
		[ ] API reference
	[ ] build configurations (to minimize size) -- via #defines
		[x] single key only mode (removes parser, nextTok & psm_tables, possibly some #def code in Parse)
		[x] remove helpfn #seperate from single key only build (extra option), move help KEY, CLI help strings to #define
		[x] remove mpthru via build config
		[x] CLI parser support only (no built-in library commands)
		[ ] base lib functions     (get/peek, set/poke, if, go)  // note: if "if" can support labels might not need go.  e.g. if 1 : trueLabel :  ;
		[ ] extended lib functions (str2int, int2hex, int2dec)
		[ ] var stack functions (user supplied mem, ints, strings in managed dict, int fns: +-*/^&|)  #TBD...
	[ ] C++ wrapper, xelp.hpp is included which provides a simpler C++ class style interface.  Both the C++ and C versions are identical in functionality.
		[ ] C++ includes XelpBuf class
	[ ] test framework migrate to jetview
	[ ] xelp base library funcs (separate file?).  library funcs requre *ths for scratchpad ram / var ram?			
		[0] _peek <addr> <numbytes>  # num bytes optional, if not spec'd returns 2 bytes.  bytes always returned as hex.  grouped in 16 by line.
		[0] _poke <addr> <values>    # must be hex bytes. poke 34h 03h a3h 34h  //h for hex, else dec.  each item must have h to be hex. first item is addr (required)
		[ ] _echo <hex/ascii> str 	 # (send hex or bytes to mpOut)  can also handle var names, func output if applicable e.g. echo $var
		[ ] _xelp <param> <value>  	 # * change parser behavior such as echo on / off
		[ ] _thru <hex/ascii> str 	 # * (send ascii or hex bytes to mpThru). default is ascii quoted string
		[ ] _skc str 				 # * exec single key cmds in quoted string ... or don't use quoted string skc cmds start after first space e.g. skc abdef wieod woie <<-space allowed as skc cmd. 
		[ ] _if bcommand arg arg : command args args args : command args args ; #';' ends if  note that XELP_S_OK equates to TRUE and anything else will equate to FALSE
		[ ] _n2b <char[2]>			 # converts ascii hex to hex byte --NNh  --> hex  NN --> decimal (interal func)
		[ ] _while bcommand arg arg : command args args args: command args args: command args arg arg arg; # ';' ends while
		[ ] _go <label>				 # * attempt to send parser ptr to first instance of label in script.  needs script context
			? - if <label> ends in ":"" treat as label.  if label ends in "h" treat as hex else treat as decimal (tbd)

	# *  <-- requires parser context (e.g instruction parser buffer pointer location)

		--- following require extra memory whether as stack or heap ---
		-->mpMalloc(int bytes)  -->mpFree(void *) -->consider primitive block alloc
		[ ] _int <name>  			 # create a 16 bit int of name <name>.  stored on var stack as 4 bytes.  2 bytes refer to <name> start pos in script. 2 bytes are val
		[ ] _str <name> <quoted str>
		[ ] math::?
		[ ] structure support?
		[ ] TCL style brackets [] or original thinking with _ :  e.g. _ begins a exec context and : terminates it.
		[ ] secure login option (example) won't run any funcs w/o password 
		[ ] register access (platform dependant)  cat regname  etc  --> see AVRSH project
		[ ] CLI prompt string or char--->  e.g. ">"

### Future   
	[ ] recent key history.  e.g. if (up_arrow) show last CLI command (only if no other key has been pressed.)
	[ ] if (down_arrow) show empty line.
	[ ] define function (which is just a script) ... functions take 2 forms scripted or C.  both should be able to call each other.
		- C functions are defined at compile time.  can be called via string/fptr (already works) from script.  C funcs can call each other as per normal convention.
		- Script funcs can be defined at run time or compile time (as a string).  At run time as
			runtime: func <name> "string"    --> just stores a named string
			compile time: "func \"name contents \";  //? store an array of strings of funcs.
			?script returning values.. *****
	[ ] $name "string"  --> declare a string var
	[ ] %num  <val>     --> declare a integer
	[ ] func "string"   --> declare a function. inside a func pull parameters via @0 @1 etc
		could also use LISP style  "defun"


### Other Notes
	seems like the VIC-20 BASIC was 8200 bytes of ROM in 6502 assemlber (includes math, multidim vars, print formatting, but not kernal calls like SYS <num>)
	see if NextTok, Parse -->
		getLine(buf, blen, *tokSt, *tokE, *lineEnd, int doTokOnly)  // used by Parse, nextTok, and _go and _if
		nextTok(buf, blen, *start, *end) { call getLine(buf,blen,start,end,dummy, TRUE);
		parse(ths, buf, blen) 		// has loop calling getLine, does .. whatever neccessary to parse
		_go (buf, label, *start)  	// searches for label at *start of line*  --> nextTok currently can't do this
		_if ...                   	// can do labels b/c it can see 1st token in line.  
		CLI first search CLI funcs.  Then search built in functions in a single function - not as fptrs.  saves space.

## Release Tasks
	[ ] test suite eg unit tests and report (note migrate to jetview)
	[ ] lint coverage
	[ ] makefile update
	[ ] travis or equivalent continuous integration test
	[ ] docs
	[ ] license (change in all files)
	[ ] change log and versioning
	[ ] naming 		xelp | xelpcmd | xelp | dci

## platforms & CPUs
	[x]	80x86-64		linux
	[ ]	80x86-64		Win Visual Stuxelp
	[ ]	80x86-64		Mac
	[ ] 80x86-32		Win Visual Stuxelp
	[ ] 80x86-32		linux
	[x] Atmel AVR 		Arduino GCC
	[x] 8051			SiLabs GCC
	[x] MSP430 			TI stick, TI ..  GCC
	[ ] MSP430          code composer
	[ ] MSP430          IAR
	[x] 68HC11	
	[x] ARM32			MBED
	[x] ARM32-Thumb		MBED?
	[x] PIC18F			GCC

	[ ]	MIPS
	[ ] H8 series
	[ ] 6502			cc65
	[ ] 80x86-16		Turbo C or Turbo C++
	[ ] PowerPC

##notes
Most of this works in ad-hoc form, need to make sure it is robust, documented and generic.

========================================

### Handling a Key Press

The keypress logic handles the "live" command line.  It must support modes such as direct (single-key) entry where
each key press is process w/o necessarily having to hit ENTER.  It also must support backspace.

```
XELPParseKey(ctx, key)
	// first keypress --> mode check
	
	if key==mode-change --> change modes
		return OK // (note leave parse buf unchanged)

	if key!=mod_change :
		if mode == single_key_mode:
			if key in singl-key-map
				return exec_sngle_key_fn(key)

		if mode == parse_mode:
			if key == BKSP && pos > 0
				command_buf_pos--
				handleBKSP() //function ptr to handle back space at the terminal level
				return OK

			else if key == ENTER
				return XELPParse(buf,command_buf_pos, mode) // attempt execute parsed command
				command_buf_pos = 0

			else
				if command_buf_pos < buf_len-1 // some key pressed... enter it in the buffer
					buf[pos++]=key

		if mode == pass_thru:
			return pass_thru(key)
```




=============================
## cross compiling notes

ARM32 (here shown with thumb, optimze for size)
```bash
arm-none-eabi-gcc -c xelp.c -Os -mthumb  
```

MSP430 (optimize for size, comile-only)
```
msp430-gcc -c xelp.c -Os
```

AVR (Arduino)
```
avr-gcc -c xelp.c -Os -s
```

8051 (via Small Device C Compiller SDDC)
```
```

68HC11 / 68HC12
sudo apt-get install gcc-m68hc1x
```
```
6502 
http://anton.maurovic.com/posts/nintendo-nes-gamedev-part-1-setting-up/


MIPS

=============================
## Grammar for statements


example xelp statements
foo1 arg1 arg2 ... ;			#call function foo1 with arg1 arg2 but args are passed as single string

Look at foo's declaration.  Note that foo is called with the buffer consisting of the position after the
carraige return, the <CR> is what activates the parser to attempt a function call.

[deftio](www.deftio.com)

when <CR> is encounted at the commandline the func match is attempted and if successful the function is called.
If no function match is found (either built-in or user supplied) then the command buffer ptr is set back to the beginning
of the buffer (no undo for this just like any other command line).
there is no command line history (no space on small platforms)


## parser segementation:
"chunker"  --> looks for when to send a command to the cmd dispatch parser.
		   --> every time ";" 


### Commands are interpretted a line at a time. e.g with thes caveats.
a semicolon (;) terminates the list of args (can be no args) and activates the intepreter,
a (:) is a by-convention separater used in some xelp.. commands.  It is the same as ; but...
	--> it allows multiple commands to passed in a buffer which is exploited in xelp_if, xelp_while constructs (tbd)
 
```
cmd1 arg1 arg2: cmd2 arg2a arg2b: cmd3 arg4; 

\\note the colon operator.  this will send the following to

cmd1 (char* argBuf, argBufLen) {
	//becaues of colons. argBuf contains this:
	"arg1 arg2: cmd2 arg2a arg2b: cmd3 arg4;"  #in other words cmd2 cmd3 are passed with their arguments
}
also the parser "consumes" the entire string at one shot.  If cmd1 decides to "drop" all the rest of the arguments or commands
"on the floor" then they won't be run as the parser passed them as arguments to cmd1.
```

However cmd1 can also via the supplied parsing mechs.  call cmd2 or cmd3 and pass the intended arguments as correctly.


grammar thoughts:
make as generic as possible e.g.

or prefix names w "xelp."   --> namespaces all built-ins
":" vs <ESC>; --> "^;"		?? use escape sequence for separators or : grammar?


_if				#conditional if <expr> : <true statements> : <false statements>;
				#if command_returns_bool arg arg : command_if_true arg arg ;
				#if command_returns_bool arg arg : command_if_true arg arg : command_if_false arg arg;
				#if command_returns_bool arg arg : { command_if_true : command_if_true } : { command_if_false arg arg;
_wh				#loop while <expr> {statements}
_set			#write bytes to mem
_set 400h 02h 	#write hex byte 0x02 to address 400
_set 400d 2039234h # write a hex set of digits to address.
_get 400h		# return value at address (1 byte) as hex
_h2i 			# hex str to int
_i2h			# int num to str
_d2i			# dec to int
_i2h			# int to dec
_s2n            # string to num.  returns integer.  deals with dec or hex  (hex via suffix)
xelp_var2i       # take var name and returns its value  # replace with $val ?  --> allows if, while to be smarter

#### variables (if allowed)
_int         # name val   #if val ommitted than set to 0
_inc         # name 
_dec         # name
_+           # name name
_- 			 # 
_*			 #

### "if" grammar
if <command-b arg arg > : <command arg arg> :  <command arg arg arg> ; #note command-b must return int result

if foo: true-command arg arg: ;  # no "else".  second ; means no "else"  

if <command-b> : : ;  #optional {} for either clause



### "while" grammar
while <command-b arg arg> <command arg arg arg>  # limit to one command?  afterall you can always "wrap" commands in the command

xelp.var name type value  #puts a var in the storage stack.  xelp doesn't have any built in way to manipulate so this is just avail to other funcs


## func names
any letter or number.  punct, and escaped sequences not permitted in func names, but not expressly forbidden



### conventions for passing binary data (xelp... only not generic cli)
Use hex with prefix?
h29830abd323    terminal is <space> | <:> | <;>

## setup xelp
* load single-key fns
* load parse-mode fns
* set mpThru - if desired
* set mpOut  - if desired
* set mpErr  - if desired
* set mpBksp - if desired (special terminal handling for destructive backspace)
* setup keymap (note defaults for those that are \0)
	* esc mode symbol via #define in parser gen.  
	* quoted esc mode symbol #via #define in parser gen
	* mode: single-key at cmd line
	* mode: parse at cmd line
	* mode: pass thru at cmdline


/*************************************************************************
 * XELP_STACK_MACHINE operations
 * note: XELP_STACK_MACHINE compile option must be defined for these to work.  
 * See xelpcfg.h for enabling XELP_STACK_MACHINE and setting stack depth
 */
#define XELP_STACK_NOP      0x0000
#define XELP_STACK_PUSH     0x0100
#define XELP_STACK_POP      0x0200
#define XELP_STACK_XOR      0x0300
#define XELP_STACK_NOT      0x0400
#define XELP_STACK_ADD      0x0500
#define XELP_STACK_INC      0x0600
#define XELP_STACK_DEC      0x0700
#define XELP_STACK_SUB      0x0800
#define XELP_STACK_MUL      0x0900
#define XELP_STACK_AND      0x0A00
#define XELP_STACK_OR       0x0B00

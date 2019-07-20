# XELP  A cmd line parser with ROMable scripting support

## XELP design notes
Feature selection between small embeded extensble CLI library and true interoperable language support


* Command Line Interface (CLI) with C language function calls 
* Scriptable commands  
	* Anything run at commandline or menus can also be called as a script.  
	* Scripts are ROM-able  
	* No script strings are modified during execution by core parser / interpreter (eg no "strtok() style" procecssing.)
* Programmer supplied C language functions can be called from command line or from script
* Each function can also have an optional help string.
* Single-key mode for immediate menus or actions (w/o having to pressing ENTER)
* Thru-mode allows redirection of key strokes to another peripheral w/o parsing by CLI etc.
* Tokenizer output available for user supplied functions that need to parse lines / params / numbers
* Single line comments via # symbol  (useful for scripts). Tabs also supported for indentation readability in scripts.  
* Can be configured, at compile time to save space.  See tables in docs for tested sizes.   See xelpcfg.h to control these options:
	* (KEY Mode) key-only mode (no CLI just key-press menus) 
	* (CLI Mode) cli-only mode (command line prompt w destructive backspace handling)
	* (THR Mode) thru support optional (redirect all keys to another peripheral w/o processing) 
	* help function optional (remove to save space, see table)
	* Override/select key mappings (enter, backspace, etc), also escape char mappings 
* Supports quoted strings in command line, escapes for command line via '`', escapes for quoted strings via '\'.  All escape chars are overridable.
* No dynamic memory needed for CLI / script interpreter / tokenizer / command dispatch (eg no malloc/free new/delete)  
* No globals or global state -- all state is stored in an instance so several instances can be run at the same time
* Reentrant provided same instance is not used as a CLI for 2 competing threads.  Scripts are reentrant by default unless programmer supplied functions are not reentrant.
* Platform independant
	* No library support required (stdio.h, string.h etc not needed).  
	* Entirely in C (no assembly) for portability. C89, C90, C99, ANSI compliant (for dealing w older compilers) 
		* this means things like // comments are not used in the .h or .c implementation files  :(
	* Simple platform asbtraction layer ("HAL") for porting uses 5 function pointers. 
* OSI approved open-source.  feedback welcome!  



# Goals.. 
* less than 1.5KB of MSP430 code (allows running on MSP430F2012 as low end micro) including parser, and 

## mtcl --> (micro-TCL) language extension of xelp (separate C-file? or separate repo?)
Provides more language support, requires more code, handling of dynamic variables and functions
 * TCL like language
 * var support  (so needs mini mem allocator)
 * store scripted fns at command line
 * provide var access from C  e.g. getVar(ths, char *, len) --> returns struct {int type; <value>} #use same str match as commands
 * scripted or C functions can call each other
 * CLI is treated like an annoymous function with no arguments (? or set a CLI arg string?  this is like calling a shell script with args sort of ..)
 * register C funcs via xelpAddFunc(funcStruct *), xelpDelFunc(char* name)
 												  --> 
 * if in same repo could be xelp is base exensible cmd lin parser.
 * move built-in commands (help,peek, poke) --> to separate C-file -- xelpcmds.c  (XELP_INCL_BASE_CMDS)  // need to deal with name spaces.  could hard-code a ptr search in Parse
 * if in xelpcfg we include #define KWIP_INCL_MTCL  1   // <<-- now language support included. replaces Parse with more capable parse. etc
 * mTCL commands need var storage, recursive call support, ability to execute strings(?)
Goal... less than 3KB of MSP430 code (allows running on all micros with atleast 4KB of code space with 1KB for scripts.  Note this is may not be palatable use of space on some embedded systems but
should generally allow comfortable running on most 32bit uCs and all 16bit systems w atleast 8KB of code.  But then it is a script interpreter so ...)
=====================================================================================
## Calling functions:
Calling functions/code/scripts involves the following choices:
	* C or Script caller  	(C_code calls fn,  or Script code calls fn)
	* C or Script function 	(callee is C Code or Script code)
	* arguments passing 
	* return values 		(e.g. result stack)
	* storing Variables 	()
	* access to script enviornment (e.g. this, bufptr to current instruction in script)

### call C func from inside CLI / script:	cmdname arg1 arg2 ...  argn #  variadic args are passed as string. 
all of these have compiled C-code function bodies
	* 1st token is searched for in available commands.  if found then code executed.  Possible C command prototypes:
		* C code:  funcName(int)			    // simplest no context, no args, used in KEY mode.  can also be used in CLI via built-in "skc a" execute key command.
		* C code:  funcName(xelp * ths, int)    // allows C func to have script var context
 		* C code:  funcName(char * args, alen)  // NO access to script env.
		* C code:  funcName(xelp* ths, char * args, int alen) // same but now can call script vars, functions from C
		* C code:  funcName(xelp* ths, char * args, int alen, char *buf, int blen, int bpos);  //passes calling context for fns such as _go:
Note that in constrained memory systems a global instance can be created of the parser which could be shared, without instance based prototypes.

### call Script func from inslide CLI / script: cmdname arg1 arg2 ...  argn #  variadic args are passed as string. 
	* 1st token in searched for in avail commands. 
		* script code: (   		   char *code )  						  # script code is "just" a string.  this is the C equiv of a func ptr.  the 
		* script code: (		   char *code, char * args, int alen)    # the args passed same as C code
		* script code: (xelp *ths, char *code, char * args, int alen) 	 			  # full context of ths, code, args passed
		* script code: (xelp *ths, char *code, char * args, int alen, char *sbuf, int slen, int *spos) # passes calling context for statements such as _go: .. still need way to chance spos

## C Version
xelpCallFunc(xelp* ths, 						//instance pointer
			char* code, int codelen,			//script instructions buffer ??require null term?
			char* args, int alen,				//argument buf and len
			char* sbuf, int slen, int *spos      //script buffer, len and current execuation position.
			 )
## C++ ish version
xelp.callFunc(string code, string args, string sbuf, int *spos);

typedef xelpContext {
	
}
typedef struct
{
	char* mpCode;		//null terminated. if creaed at run-time sense=r
	char* mpCmdName;	//null terminated
	char* mpHelpString; //null terminated
}XELPScriptFuncMapEntry; 

XELPRESULT XelpExecScript (xelp* ths, const char *code, int clen, char *args, int alen) ; // C func runs interpreter with arguments

to run a function (not just a codeStringScript from C)	

XELPRESULT DioExecFunc (xelp* ths, const char *funcName, int flen, char *args, int alen) ; 
	* C interp looks up func and executes if found
		* script run-time table (scripts declared at run-time from code)
		* scripts stored statically in C
		* C named functions

		looks up func by name. if found calls func based on table stored in.

Need to address _go, _if, label finding.

if C func:
	name table finds func ptr
	args passed as (ths, char *, len) --> to func ptr
if script func:
	name table finds ptr char * : str-of-statements.
	args passed as (ths, char*, len)  --> to string of script statement

from in C:
	XELPParseFull(ths,tok0,tok0len, args, arglen)
--> XELPParseFull(ths,tok0,tok0len,tok1,line_end-tok1_start);
	exec [command arg arg] arg  <-- TCL style.  Here command evaluates first.  Need to figure out how to return value.  incl no-value returned.  
	exec $varname arg           <-- TCL style.  $varname substituted w var value.  Note if memory laid out correctly can just point to that.
	note for user script C fn need way to eval [ ] or get value of $variable. eg. in C call a fn or get the value of a stored var.

or
existingStringVar arg1 arg2 etc  <-- if a string has been stored as a variable, it can be run just by typing its name - just like any other fn  ?need executable bit? 

### Variadic Arguments
+ 23 23 093 34  <--- variadic operators
foo a1 a2 a3 a4 .. ==> calls foo with args a1... a3.  
add (x, y) { return x / y} // named params
add x y ==> return (+ @1 @2)   # _fname=@0  ... naming args _x=@1 _y=@2

## Variables
in script:
	set <varname> <vale> # set a var
	del <varname>        # remove var
	$name 				 # retriev value

in C
	XELPSetVar(ths,"name","value");

get value:

variable storage : both for runtime possibly for load/save -- packing:
	[byte0] :
		3bit num (hi bits)
			000 string
			001 executable string
			010 integer (sizeof int)
			011 float32 in 8.24 format
			111 <extended form (future var)>
		length of name in lower 5 bits

using this:
	string: [000:namlen_5_bits][name_str][2byte-len][string-content] #just a string
	code  : [001:namlen_5_bits][name_str][2byte-len][string-content] #just a string
	int   : [010:namlen_5_bits][name_str][sizeof(int)bytes] #platform dep
	float : [011:namlen_5_bits][name_str][4bytes]
	ext   : [111:namlen_5_bits][name_str][4byte-len][content] #content is TBD.  this allows 4GB of space.  could use int-num.

using ths scheme these minima exist:
	string : 2 bytes for name, 3 bytes (2for len, 1 for a char) = 5 bytes e.g. $a "x" #string named "a" w single char "x" as value, 4 bytes for null string.
	code: same as string 
	int:     2 bytes for name, 2 bytes for value = 4 bytes		= $a 234    #int
	float:   2 bytes for name, 4 bytes for vallue = 6 bytes		= $a 12.3	#floating

vars   --> built-in CLI command dumps all vars to screen.
vars h --> dumps all vars in hex format (incl descriptor bits)

var life cycle:
	$name value   	#value required as it determines type.	
	$name 34h		#declares an integer, inital value is 34 hex
	$name 34		#declares an integer, intial value is 34 decimal
	$name "string"	#declares a string. quoted required.
	$name f "statements"	#declares a code script.  can be as many code statemnets as desired

	del name 		#attempts to delete a var from memory regardless of type.  C functions, lang commands can't be deleted ... don't name vars same as these

## from C:
XELPParseFull(ths, char *func, int flen, char * args, int alen);  // allows arguments to be passed to script from C.  also allows script fn to call with args?


? value returns..
return stack:
func args; #last value on return stack
func1 func2 args : args;   #func2 result on stack, can be used by func1.


## Parameter passing
passing values (for lang extensions):
"string or commands"	 	# string or commands  --> a string is commands if its passed to the interpreter .. made executable.
2345						# decimal number
1234h						# hexadecimal number

## Storing scripted functions.
in C:
typedef struct {
	char *fbody;	// this is instead of the function pointer
	char *fname;	// name is visible inside any script ... or from C --> XELPParseFull(ths,"name",strlen(name),char args, int alen);
	char *help;		// help string is same for C declared function
}

inside a script or at CLI .. ideas here:
var name "code goes in here" "optional help string" #quoted string means ... a quoted string  concern: quotes inside quotes
var $name .........................................;   # same idea but every after $name is the string.  concern: spaces, ;
	--> $name <-- $ means a string variable
var %name     <-- means a numeric variable

calling a string function from inside cli or script

funcname args args args  #TBD prob want script name space first.  
	first tries C namespace.  If found calls C function.
	next calls Script namespace.  If found calls function like this:
		XELPParseFull(ths,fname,flen,args,alen);

#name space hierachy:
1 scripted functions stored at runtime
2 scripted functions stored in memory (statically allocated in C)
3 programmar supplied C functions
4 lang-extension functions (e.g. goto, poke, peek)

## return values ..
as string then use quotes for a string.  if not in quotes then number?
when function returns its value is on stack.  (e.g. last func)

## expression handling / math
See TCL docs (wikipidia and others)
Note TCL uses [] and {} as types of "quotes" but handles them differently.  

add num1 num2 #prefix (Polish) notation --> like TCL, like command line thinking

any string can be executed as script?

$varname value  

argument reference  @0 @1 @2  <-- refers to arguments passed to a script?
	numargs?

### lisp style thinking:
(operand args args (operand (operand args args  ) args args))

**Note** TCL uses [] for this purpose
becomes
cmd arg ( cmd arg arg );   # parens --> by-convention evaluation operator. space/tab required
if cmd is a lang cmd it can scan the line for parens, do whatever.
cmd --> find operator

math expresions... ( * ( 3 + 3h) 4 )

expressions .... 
http://scanftree.com/Data_Structure/infix-to-prefix 
http://cforfrnds.blogspot.com/2013/09/infix-to-prefix-conversion-stacks-in-c.html

--------------------------------------------------------------------------
## lang-core functions
givens:
	ths
	bufStart *  //this is the beginning of the script
	bufPos	 *  //this is where current buf pointer is
	buflen 		//this is how long the buf is..

"peek addr"		--> return hex byte
"peek addr num" --> return num bytes starting at addr in 

XELPRESULT XELPs2i (char *buf, int buflen, *num);  --> returns true if num else XELP_E_PARSE_ERR
XELPRESULT XELPOutnum2hex ()	--> print a hex nmber to XELPOut()
XELPRESULT peek(const char* buf, blen) {  --> writes to XELPOut()  internally
	int i,a;
	if XELPNextTok(buf,blen,..) == XELP_S_OK { //skip over "peek"
		if XELPNextTok(buf, blen, ....) == XELP_S_OK {
			if (XELP_S_OK == XELPs2i(buf,a)
				XELPOutnum2hex(*a);
				_PUTC('h');
		}
	}
	return XELP_E_ARGS;
}

## bracket / parens parsing
_SEEK:
  [  --> enter bracket, 
_TOK0:
  [  --> enter bracket, 
_QUOT, QUOT_ESC, _SEOL,_ESCA:
  <nothing>
_PREV:
  <N/A> //>

_BRKT: 
  [  --> up bracket count, bkt_cnt++, stay in BRKT
  ]  --> dn bracket count, bkt_cnt--, if bkt_cnt == 0 
  	ACTION ... call parse
  	NOTE:  next state ... stay in bracket until bkt_cnt ==0 

Actions:
	if (*s & EF_BS) { // opening bracket
		if (bkt_cnt == 0) { // just entered
			bkt_start_pos_ptr = p;
		}
		bkt_cnt++;
	}
	if (*s & EF_BE) { // closing bracket
		bkt_cnt--;
		
		if (bkt_cnt == 0) {
			//bracket context now available [bkt_start_pos_ptr : p]
			XelpParse(ths, bkt_start_pos_ptr, p-bkt_start_pos_ptr, <no args>, sbuf, spos, slen); // need to get return value from this..
			//TODO also allow brkt_end_next state 
		}
		if (bkt_cnt < 0) {
			//signal error..
		}

	}
	// ..
	tmp = cs;
	cs = ((*s == _PS_PREV) && (bkt_cnt == 0)) ? prev : (*s); //need to check this is OK or need to add anohter state such as  _PS_PBKT 
	prev = tmp;
	/* end of parser 

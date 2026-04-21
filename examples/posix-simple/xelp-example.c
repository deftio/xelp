/*

@xelp-example.c - implementation
@copy Copyright (C) <2012>  <M. A. Chatterjee>
@author M A Chatterjee <deftio [at] deftio [dot] com>

@license:

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

 */

//these includes are only for demo purposes.
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>
#include <termios.h>
#include <time.h>
#include <ncurses.h>

#define XELPGLOBAL_DEFAULTS

#include "../../src/xelp.h"

// setup keypress handling for unix
int getkey() /* unix non-blocking key get */
{
    int character=0;
    struct termios orig_term_attr;
    struct termios new_term_attr;

    /* set the terminal to raw mode */
    tcgetattr(fileno(stdin), &orig_term_attr);
    memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

    /* read a character from the stdin stream without blocking */
    /*   returns EOF (-1) if no character is available */
    character = fgetc(stdin);

    /* restore the original terminal attributes */
    tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);
    fflush(stdin);

    return character;
}

void gPutChar(char c) {
	addch(c);
	refresh();
}


void handleBackspace() {
	//ncurses stuff
	int r,c;
	getyx(stdscr, r, c);
	move(r, c-1);   
	delch();
	refresh();
}
// end key press handling (curses, unix)
//This is the only include for xelp

#define XELPGLOBAL_DEFAULTS
#include "xelp.h"

XELP example; //global declarator for an interperter.  Note this can be instance based.
int gExit=0;  //global flag for when to quit interpretor loop, not part of XELP, just the demo


/****
 begin user defined functions for XELP cli  -- key mode
 */
XELPRESULT fooExit(XELP *ths, XELPKEYCODE c)
{
	(void)ths;
	printw("fooExit(%lx) invoked\n",c);
	gExit=1;  // modify the global flag so we quite the  interpretor loop
	return XELP_S_OK;
}

XELPRESULT fooBar(XELP *ths, XELPKEYCODE c)
{
	(void)ths;
	printw("fooBar(%c) invoked (single-key mode)\n",(char)c);
	return XELP_S_OK;
}
XELPRESULT fooPrint(XELP *ths, XELPKEYCODE c)
{
	(void)ths;
	printw("fooPrint(%lx) invoked (single-key mode)\n",c);
	return XELP_S_OK;
}

XELPRESULT fooHelp(XELP *ths, XELPKEYCODE c)
{
	(void)c;
	return XELPHelp(ths);
}
XELPRESULT printBanner(XELP *ths, XELPKEYCODE c) {
	(void)c;
	XELPOut(ths,XELP_BANNER_STR,-1); // XELPOut ==> print out a null terminated string, in this case the XELP banner in ascii
	return XELP_S_OK;
}
void fooNormal(char c)
{
	printw("fooNormal called (single-key mode)");
	printw("%c",c);
}


//esc or single-key mode commads
XELPKeyFuncMapEntry gMyKeyCommands[] =
{
	{&fooHelp  ,'h', "Help()"        },
	{&fooBar   ,'f', "fooBar()"      },
	{&fooPrint ,'p', "Prints stuff"  },
	{&fooPrint ,'w', "Prints stuff"  },
    {&printBanner,'b',"Print XELP   "},
	{&fooExit  ,'x', "Exit()"        },
	XELP_FUNC_ENTRY_LAST
	//{0         , 0 , ""              }
};

XELPRESULT cmdCLS (XELP *ths, const char* args, int maxlen)
{
	(void)ths; (void)args; (void)maxlen;
	printw("\x1B");
	printw("C");
	return XELP_S_OK;
}

XELPRESULT banner (XELP *ths, const char* args, int maxlen)
{
	(void)args; (void)maxlen;
	printBanner(ths, 'b');
	return XELP_S_OK;
}
XELPRESULT cmdHome (XELP *ths, const char* args, int maxlen)
{
	(void)ths; (void)args; (void)maxlen;
	printw("\x1B");
	printw("H");
	return XELP_S_OK;
}
XELPRESULT cmdEcho (XELP *ths, const char* args, int maxlen)
{
	XELPOut(ths, "<<", 0);
	XELPOut(ths, args, maxlen);
	XELPOut(ths, ">>\n", 0);
	return XELP_S_OK;
}

XELPRESULT cmdNumToks (XELP *ths, const char* args, int maxlen)
{
    XelpBuf b;

    int n;
    (void)ths;
    XELP_XB_INIT(b,args,maxlen);
    XELPNumToks(&b,&n);
	printw(" XELPNumToks %d\n",n);

    return XELP_S_OK;
};
XELPRESULT cmdPrintR (XELP *ths, const char* args, int maxlen){
	(void)args; (void)maxlen;
	printw("R0=%d R1=%d R2=%d R3=%d\n",
	       XELP_R0(*ths), XELP_R1(*ths), XELP_R2(*ths), XELP_R3(*ths));
	return XELP_R0(*ths);
}

XELPRESULT cmdDivmod (XELP *ths, const char* args, int maxlen){
	XelpBuf b, tok;
	int dividend, divisor;
	XELP_XB_INIT(b, (char*)args, maxlen);

	XELP_XB_TOP(b);
	XELPTokN(&b, 1, &tok);
	dividend = XELPStr2Int(tok.s, tok.p - tok.s);

	XELP_XB_TOP(b);
	XELPTokN(&b, 2, &tok);
	divisor = XELPStr2Int(tok.s, tok.p - tok.s);

	if (divisor == 0) {
		printw("divmod: division by zero\n");
		return XELP_E_ERR;
	}
	ths->mR[1] = dividend / divisor;
	ths->mR[2] = dividend % divisor;
	printw("%d / %d = %d remainder %d\n", dividend, divisor, ths->mR[1], ths->mR[2]);
	return XELP_S_OK;
}


XELPRESULT cmdListToks (XELP *ths, const char* args, int maxlen)
{

#ifdef XELP_ENABLE_CLI
    XelpBuf b,tok;
    int n,i;
    XELPRESULT r;
    XELP_XB_INIT(b,args,maxlen);
    XELPNumToks(&b,&n);
    XELP_XB_TOP(b);
    printw("[%d]",n);
	for (i=0; i< n; i++) {
        XELP_XB_TOP(b);
        r = XELPTokN( &b,i,&tok);
        printw("<");
        printw("%d:",i);
		XELPOut(ths,tok.s,tok.p-tok.s);
		printw(">");
	}
#endif
	printw("\n");

	return XELP_S_OK;
};

XELPRESULT cmdHelp (XELP *ths, const char* args, int maxlen)
{
	(void)args; (void)maxlen;
	return XELPHelp(ths);
}

XELPRESULT cmdExit (XELP *ths, const char* args, int maxlen) {
	(void)ths; (void)args; (void)maxlen;
	gExit = 1;
	return XELP_S_OK;
}
XELPRESULT cmdPrintNum (XELP *ths, const char *args, int maxlen) {
	XelpBuf b,tok;
    int n;
    (void)ths;

    XELP_XB_INIT(b,args,maxlen);
    XELPTokN(&b,1,&tok),

	printw("[%d]\n",XELPStr2Int(tok.s,tok.p-tok.s));
	return XELP_S_OK;
}

XELPRESULT cmdMath (XELP *ths, const char* args, int maxlen) {
	XelpBuf b,tok;
    int i,j,k;
    int op;

    XELP_XB_INIT(b,args,maxlen);
    XELPTokN(&b,0,&tok),


	op = *b.s;

    XELP_XB_TOP(b);
    XELPTokN(&b,1,&tok);
    i = XELPStr2Int(tok.s,tok.p-tok.s);

    XELP_XB_TOP(b);
    XELPTokN(&b,2,&tok);
	j =XELPStr2Int(tok.s,tok.p-tok.s);

	switch(op) {
		case '+':
			k=i+j;	break;
		case '-':
			k=i-j;	break;
		case '*':
			k=i*j;	break;
		case '/':
			k=i/j;	break;
		case '|':
			k=i|j;	break;
		case '&':
			k=i&j;	break;
		case '^':
			k=i&j;	break;
		case '%':
			k=i%j;  break;
	}
	printw("%d %c %d = %d",i,(char) op,j,k);
	XELPOut(ths,"\n",1);
	return XELP_S_OK;
}
//declare a command map for functions in parse mode
XELPCLIFuncMapEntry gMyCLICommands[] =
{
    {&banner    		, "banner"	,  "print XELP banner in ASCII" },
	{&cmdEcho    		, "echo"	,  "print args to screen"       },
	{&cmdNumToks 		, "numtoks" ,  "print number of arguments"  },
	{&cmdListToks		, "lt"      ,  "list parsed tokens"         },
	{&cmdHelp	 		, "help"    ,  "help"						},
	{&cmdPrintNum       , "num"     ,  "print num to console"       },
	{&cmdCLS			, "cls"		,  "clear screen (uses ASCII ESC seq"},
	{&cmdHome			, "home"	,  "Set cursor to home"			},
	{&cmdMath           , "+"       ,  "add two numbers"            },
	{&cmdMath           , "-"       ,  "sub two numbers"            },
	{&cmdMath           , "*"       ,  "mul two numbers"            },
	{&cmdMath           , "/"       ,  "div two numbers"            },
	{&cmdPrintR			, "pr"      ,  "print all registers"        },
	{&cmdDivmod			, "divmod"  ,  "divmod <a> <b>: R1=a/b R2=a%b"},
	{&cmdExit           , "exit"    ,  "quit demo program"          },
	XELP_FUNC_ENTRY_LAST
};

//modeChangeMsg is a callback when mode is switched from CLI / Thru / Key
void modeChangeMsg(int mode) {
	if (mode == XELP_MODE_CLI) {
		printw("Mode changed to CLI\n");
		return;	
	}
	if (mode == XELP_MODE_KEY) {
		printw("Mode changed to KEY\n");
		return;	
	}	
	if (mode == XELP_MODE_THR) {
		printw("Mode changed to THR\n");
		return;	
	}
	printw("Unknown mode. %d",mode);
	return;
}




//===============================================
//main program for testing the functions
int main (int argc, char *argv[])
{

	/* ncurses setup (nothing to do with xelp) */
	initscr();
	cbreak();
	noecho();
    nodelay( stdscr, TRUE ); //setup non blocking io in ncurses.  ncurses is just used for terminal debugging in linux
    keypad( stdscr, TRUE);   //allow capture of special keys eg delete etc
    scrollok(stdscr, TRUE);
    /* end of curses setup*/

    
    int ret_val = 0;
	int i=0;
	
	//begin XELP setup
	char *pAboutStr = "\nExample Ver XELP Intrpreter \n By deftio\n\nEsc: single-key fns. \n(x) to exit\n  \nCTRL-P: CLI (Command line interpeter) mode\nCTRL-T: thru mode\n\n";
	XELPInit(&example,	pAboutStr); // set the about string for the interpreter and initialize internal state  

	//example.mKeyBKSP = 0x7; //ncurses on linux
#ifdef XELP_ENABLE_CLI	
	XELP_SET_FN_BKSP(example,&handleBackspace);
	//example.mpfBksp = &handleBackspace; // this is the other way to set up backspace handling (applies to CLI parse mode only)
#endif
	XELP_SET_FN_EMCHG(example,&modeChangeMsg);  // optional call back when mode changed (if enabled see xelpcfg.h)
	//example.mpfEditModeChg = &modeChangeMsg;  //emit message when key entry mode changes see xelpcfg.h for more details
	
	XELP_SET_FN_OUT(example,&gPutChar);  
	XELP_SET_FN_ERR(example,&gPutChar);        // optional - send error message to stream
	XELP_SET_FN_KEY(example,gMyKeyCommands);   // map the single key commands
	XELP_SET_FN_CLI(example,gMyCLICommands);   // map the cli commands
	XELP_SET_VAL_CLI_PROMPT(example,"xelp>");  // if using per-instance prompt...
	
	// end of setup


	printw("\n============================================================\n");
	printBanner (&example, 0);
	
	XELPHelp(&example); // print out help to start off program.  help is per-instance

	printw("\n............\n");
	
	printw("\n==================\n");
	printw("\nEntering Main Loop\n");
	printw("\nXELP size: %d\n",(int)sizeof(XELP));
	XELPParseKey(&example,'\n'); //hack to do first prompt;
	
	do
	{
		i = getch();

		if (i!=-1)
			XELPParseKey(&example,i);

		i=-1;
	}while (!gExit); // gExit is a global variable that is called if the exit command is called ("exit" in CLI mode or "x" in KEY mode)

	endwin(); // clean up curses

	printf("\n");
	return ret_val;
}


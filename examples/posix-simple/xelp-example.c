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

#include <stdio.h>
#include <ncurses.h>

#include "../../src/xelp.h"

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
XELP example; /* xelp instance -- one per UART/console you want to control */
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
	return XelpHelp(ths);
}
XELPRESULT printBanner(XELP *ths, XELPKEYCODE c) {
	(void)c;
	XelpOut(ths,XELP_BANNER_STR,-1); // XelpOut ==> print out a null terminated string, in this case the XELP banner in ascii
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
	clear();
	refresh();
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
	move(0, 0);
	refresh();
	return XELP_S_OK;
}
XELPRESULT cmdEcho (XELP *ths, const char* args, int maxlen)
{
	XelpOut(ths, "<<", 0);
	XelpOut(ths, args, maxlen);
	XelpOut(ths, ">>\n", 0);
	return XELP_S_OK;
}

XELPRESULT cmdNumToks (XELP *ths, const char* args, int maxlen)
{
    XelpBuf b;

    int n;
    (void)ths;
    XELP_XB_INIT(b,(char*)args,maxlen);
    XelpNumToks(&b,&n);
	printw(" XelpNumToks %d\n",n);

    return XELP_S_OK;
};
XELPRESULT cmdPrintR (XELP *ths, const char* args, int maxlen){
	(void)args; (void)maxlen;
	printw("R0=%d R1=%d R2=%d R3=%d\n",
	       XELP_R0(*ths), XELP_R1(*ths), XELP_R2(*ths), XELP_R3(*ths));
	return XELP_R0(*ths);
}

XELPRESULT cmdDivmod (XELP *ths, const char* args, int maxlen){
	XelpArgs a;
	int dividend, divisor;
	XelpArgsInit(&a, args, maxlen);
	XelpNextTok(&a, 0);              /* skip command name */
	XelpNextInt(&a, &dividend);
	XelpNextInt(&a, &divisor);

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
    XELP_XB_INIT(b,(char*)args,maxlen);
    XelpNumToks(&b,&n);
    XELP_XB_TOP(b);
    printw("[%d]",n);
	for (i=0; i< n; i++) {
        XELP_XB_TOP(b);
        XelpTokN( &b,i,&tok);
        printw("<");
        printw("%d:",i);
		XelpOut(ths,tok.s,tok.p-tok.s);
		printw(">");
	}
#endif
	printw("\n");

	return XELP_S_OK;
};

XELPRESULT cmdHelp (XELP *ths, const char* args, int maxlen)
{
	(void)args; (void)maxlen;
	return XelpHelp(ths);
}

XELPRESULT cmdExit (XELP *ths, const char* args, int maxlen) {
	(void)ths; (void)args; (void)maxlen;
	gExit = 1;
	return XELP_S_OK;
}
XELPRESULT cmdPrintNum (XELP *ths, const char *args, int maxlen) {
	XelpBuf b,tok;
    (void)ths;

    XELP_XB_INIT(b,(char*)args,maxlen);
    XelpTokN(&b,1,&tok),

	printw("[%d]\n",XelpStr2Int(tok.s,tok.p-tok.s));
	return XELP_S_OK;
}

XELPRESULT cmdMath (XELP *ths, const char* args, int maxlen) {
	XelpBuf b,tok;
    int i,j,k=0;
    int op;

    XELP_XB_INIT(b,(char*)args,maxlen);
    XelpTokN(&b,0,&tok),


	op = *b.s;

    XELP_XB_TOP(b);
    XelpTokN(&b,1,&tok);
    i = XelpStr2Int(tok.s,tok.p-tok.s);

    XELP_XB_TOP(b);
    XelpTokN(&b,2,&tok);
	j =XelpStr2Int(tok.s,tok.p-tok.s);

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
	XelpOut(ths,"\n",1);
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
	{&cmdPrintNum       , "num"     ,  "print integer to console"    },
	{&cmdCLS			, "cls"		,  "clear screen"                },
	{&cmdHome			, "home"	,  "cursor to top-left"          },
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




/*
 * ===================================================================
 * main -- ncurses terminal setup + xelp main loop
 *
 * ncurses is used here only as a portable terminal abstraction for
 * this POSIX demo.  On a real embedded target you would read bytes
 * from a UART and call XelpParseKey() directly -- ncurses is not
 * involved at all.
 * ===================================================================
 */
int main (int argc, char *argv[])
{
	int i = 0;
	(void)argc; (void)argv;

	/* --- ncurses terminal setup --------------------------------- */
	initscr();
	cbreak();    /* disable line buffering, pass each char to us     */
	noecho();    /* don't echo typed chars -- xelp handles its own   */
	nodelay(stdscr, TRUE);  /* non-blocking getch()                  */
	scrollok(stdscr, TRUE); /* allow the window to scroll            */

	/*
	 * IMPORTANT: keypad is OFF.
	 *
	 * When keypad(stdscr, TRUE) is set, ncurses intercepts multi-byte
	 * escape sequences (e.g. ESC [ D for left-arrow) and returns a
	 * single cooked constant like KEY_LEFT (260).  xelp has its own
	 * escape-sequence accumulator (_xelpKeyAccum) that expects to
	 * receive the raw bytes one at a time: 0x1B, '[', 'D'.
	 *
	 * With keypad OFF, ncurses passes the raw bytes through.  xelp
	 * reassembles them into XELP_KEYCODE_LEFT etc. and the built-in
	 * line editor handles cursor movement, Home/End, Delete, etc.
	 *
	 * This matches how xelp works on a real microcontroller: the UART
	 * delivers one byte at a time and xelp sorts it out.
	 */
	keypad(stdscr, FALSE);
	/* --- end ncurses setup -------------------------------------- */


	/* --- xelp instance setup ------------------------------------ */
	const char *pAboutStr =
		"\nxelp posix demo\n"
		"\n"
		"ESC     : single-key mode  (x = exit, h = help)\n"
		"CTRL-P  : CLI mode         (type command + ENTER)\n"
		"CTRL-T  : pass-through mode\n"
		"UP/DOWN : recall previous commands\n"
		"\n";

	XelpInit(&example, pAboutStr);

	XELP_SET_FN_OUT(example,  &gPutChar);        /* character output   */
	XELP_SET_FN_ERR(example,  &gPutChar);        /* error output       */
	XELP_SET_FN_BKSP(example, &handleBackspace); /* destructive bksp   */
	XELP_SET_FN_EMCHG(example,&modeChangeMsg);   /* mode change notify */
	XELP_SET_FN_KEY(example,   gMyKeyCommands);   /* single-key table   */
	XELP_SET_FN_CLI(example,   gMyCLICommands);   /* CLI command table   */
	XELP_SET_VAL_CLI_PROMPT(example, "xelp>");    /* per-instance prompt */
	/* --- end xelp setup ----------------------------------------- */


	printw("\n============================================================\n");
	printBanner(&example, 0);
	XelpHelp(&example);

	printw("\nXELP struct size: %d bytes\n", (int)sizeof(XELP));
	XelpParseKey(&example, '\n'); /* emit first prompt */

	/*
	 * Main loop: read one byte at a time from ncurses and feed it to
	 * xelp.  Because keypad is OFF, arrow keys arrive as raw escape
	 * sequences (ESC [ A/B/C/D) and xelp's key accumulator reassembles
	 * them.  getch() returns ERR (-1) when no input is available.
	 */
	do {
		i = getch();
		if (i != ERR)
			XelpParseKey(&example, (char)i);
	} while (!gExit);

	endwin();
	printf("\n");
	return 0;
}


/*

@xelp-example.c - implementation
@copy Copyright (C) <2012>  <M. A. Chatterjee>
@author M A Chatterjee <deftio [at] deftio [dot] com>
@version 0.2 M. A. Chatterjee


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

//This is the only include for xelp
#define XELPGLOBAL_DEFAULTS
#include "xelp.h"

char gPokeBuf[200];  //for testing poke, peek commands

XELP example; //global declarator for an interperter.  Note this can be instance based.

// default err functiom
void XELPErr(XELP* ths, int e) {
	addch('E');
	addch('r');
	addch('r');
	addch(':');
	addch((char)('a'+((e>>4)&0xf)));
	addch((char)('a'+((e&0xf))));
	addch(' ');
    addch('\n');
}

XELPRESULT fooBar(int c)
{
	printw("fooBar(%c) invoked (single-key mode)\n",c);
	return XELP_S_OK;
}
XELPRESULT fooPrint(int c)
{
	printw("fooPrint(%x) invoked (single-key mode)\n",c);
	return XELP_S_OK;
}

int gExit=0;
XELPRESULT fooExit(int c)
{
	printw("fooExit(%x) invoked\n",c);
	gExit=1;
	return XELP_S_OK;
}
XELPRESULT fooHelp(int c)
{
	return XELPHelp(&example);
}
XELPRESULT printBanner( int c) {
	printw("\n");
	printw(XELP_BANNER_STR);
	return XELP_S_OK;
}
void fooNormal(char c)
{
	printw("fooNormal called (single-key mode)");
	printw("%c",c);
}

void gPutChar(char c) {
	addch(c);
	refresh();
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

XELPRESULT cmdCLS (const char* args, int maxlen)
{
	printw("\x1B");
	printw("C");
	return XELP_S_OK;
}

XELPRESULT banner (const char* args, int maxlen)
{
	printBanner('b');
	return XELP_S_OK;
}
XELPRESULT cmdHome (const char* args, int maxlen)
{
	printw("\x1B");
	printw("H");
	return XELP_S_OK;
}
XELPRESULT cmdEcho (const char* args, int maxlen)
{
	int i;
	printw("<<");
	for(i=0; i<maxlen; i++)
		addch(args[i]);
	printw(">>\n");
	return XELP_S_OK;
}

XELPRESULT cmdNumToks (const char* args, int maxlen)
{
    XelpBuf b;
 
    int n;
    XELP_XBInit(b,args,maxlen);
    XelpNumToks(&b,&n);
	printw(" XelpNumToks %d\n",n);
	
    return XELP_S_OK;
};
XELPRESULT cmdPrintR (const char* args, int maxlen){
	int i = example.mR[0];
	printw("%x\n",i);
	return i;  // leaves mR[0] unchanged
}

XELPRESULT cmdPrintS (const char* args, int maxlen){
	printw("not implemented\n");
	return XELP_W_Warn;  
}
XELPRESULT cmdPrintPokeBuf (const char* args, int maxlen){
	int i=0;
	for (i=0;i<31; i++){
		printw("%c",gPokeBuf[i]);
	}
	printw("\n");
	return XELP_S_OK;
}

XELPRESULT cmdListToks (const char* args, int maxlen)
{
    
#ifdef XELP_ENABLE_CLI
    XelpBuf b,tok;
    int n,i;
    XELPRESULT r;
    XELP_XBInit(b,args,maxlen);
    XelpNumToks(&b,&n);
    XELP_XBTOP(b);
    printw("[%d]",n);
	for (i=0; i< n; i++) {
        XELP_XBTOP(b);
        r = XELPTokN( &b,i,&tok);
        printw("<");
        printw("%d:",i);
		XELPOut(&example,tok.s,tok.p-tok.s);
		printw(">");
	}
#endif
	printw("\n");
		
	return XELP_S_OK;
};

XELPRESULT cmdHelp (const char* args, int maxlen)
{
	return XELPHelp(&example);
}

XELPRESULT cmdExit (const char* args, int maxlen) {
	gExit = 1;
	return XELP_S_OK;
}
XELPRESULT cmdPrintNum (const char *args, int maxlen) {
	XelpBuf b,tok;
    int n;

    XELP_XBInit(b,args,maxlen);
    XELPTokN(&b,1,&tok),
    
	printw("[%d]\n",XELPStr2Int(tok.s,tok.p-tok.s));
	return XELP_S_OK;
}

XELPRESULT cmdMath (const char* args, int maxlen) {
	XelpBuf b,tok;
    int i,j,k;
    int op;

    XELP_XBInit(b,args,maxlen);
    XELPTokN(&b,0,&tok),


	op = *b.s;
	
    XELP_XBTOP(b);
    XELPTokN(&b,1,&tok);
    i = XELPStr2Int(tok.s,tok.p-tok.s);
    
    XELP_XBTOP(b);
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
	printw("%d %d %d",i,j,k);
	XELPOut(&example,"\n",1);
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
	{&cmdPrintPokeBuf	, "ppb"		,  "print poke test buf"		},
	{&cmdPrintNum       , "num"     ,  "print num to console"       },
	{&cmdCLS			, "cls"		,  "clear screen (uses ASCII ESC seq"},
	{&cmdHome			, "home"	,  "Set cursor to home"			},
	{&cmdMath           , "+"       ,  "add two numbers"            },
	{&cmdMath           , "-"       ,  "sub two numbers"            },
	{&cmdMath           , "*"       ,  "mul two numbers"            },
	{&cmdMath           , "/"       ,  "div two numbers"            },
	{&cmdPrintR			, "pr"      ,  "print Xelp regs"            },
	{&cmdPrintS			, "pstk"    ,  "print Xelp stack"           },
	{&cmdExit           , "exit"    ,  "quit demo program"          },
	XELP_FUNC_ENTRY_LAST
};

void handleBackspace() {
	//ncurses stuff
	int r,c;
	getyx(stdscr, r, c);
	move(r, c-1);   
	delch();
	refresh();
}
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

    //raw()					//captures CTRL-C, CTRL-Z etc
    // http://hughm.cs.ukzn.ac.za/~murrellh/os/notes/ncurses.html for catching special keys

    // end of ncurses setup
	int ret_val = 0;
	int i=0;
	for (i=0; i<100; i++)
		gPokeBuf[i] = 'a'+(i&0xf);
	printw("\n============================================================\n");
	printw("xelp simple interpreter quickie example program\n");
	printw("M. A. Chatterjee (c) 2012\n\n");
	printw("\n");
	
	char *pAboutStr = "\nExample Ver XELP Intrpreter v0.01\n By deftio\n\nEsc: single-key fns. \n(x) to exit\n  \nCTRL-P: CLI (Command line interpeter) mode\nCTRL-T: thru mode\n\n";
	XELPInit(&example,	pAboutStr);

	//example.mKeyBKSP = 0x7; //ncurses on linux
#ifdef XELP_ENABLE_CLI	
	XELP_SET_FN_BKSP(example,&handleBackspace);
	//example.mpfBksp = &handleBackspace; // setup backpace platform dependant visual handling.  (applies to CLI parse mode only)
#endif
	//example.mpfEditModeChg = &modeChangeMsg;  //emit message when key entry mode changes
	XELP_SET_FN_EMCHG(example,&modeChangeMsg);

	//XELPSetIO(&example,&gPutChar,&gPutChar);
	XELP_SET_FN_OUT(example,&gPutChar);
	XELP_SET_FN_ERR(example,&gPutChar);
	XELP_SET_FN_KEY(example,gMyKeyCommands);
	XELP_SET_FN_CLI(example,gMyCLICommands);  
	XELP_SET_VAL_CLI_PROMPT(example,"xelp>");  // if using per-instance prompt...
	XELPHelp(&example); // print out help to start off program.

	printw("\n............\n");
	char *c4 =
	"#sample script\necho this; #first instruction\n\techo next;\n\techo next : this and that : that;\n\techo and this #with a comment\necho one more thing.\nlt foo bar \"this and that\" ;\nnumtoks foo bar \"this and that\"  ;\n";
	printw("\n%s\n",c4);
#ifdef XELP_ENABLE_CLI

    XelpBuf xb;
    XELP_XBInit(xb,c4,XELPStrLen(c4))
    XELPParseXB(&example,&xb);
#endif

	printw("\n==================\n");
	printw("\nEntering Main Loop\n");
	printw("\nXELP size: %d\n",(int)sizeof(XELP));
	printw("gPokeBuf Addr = %ld\n",gPokeBuf);
	XELPParseKey(&example,'\n'); //hack to do first prompt;
	//printw("msgIndex %d\n",example.mCmdMsgIndex);
	do
	{
		//i=(char)getkey();
		i = getch();

		//i=(char)fgetc(stdin);
		if (i!=-1)
			XELPParseKey(&example,i);

		i=-1;
	}while (!gExit);

	endwin();

	printf("\n\n");
	return ret_val;
}


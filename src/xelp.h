/*

  @xelp.h - header file for xelp command interpreter
 		
  @copy Copyright (C) <2011>  <M. A. Chatterjee>
  @author M A Chatterjee <deftio [at] deftio [dot] com>
  
  This file contains header defintions for the xelp simple embedded command interpreter.

  @license: 
	Copyright (c) 2011, M. A. Chatterjee <deftio at deftio dot com>
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

 */

#ifndef __XELPh__
#define __XELPh__

#include "xelpcfg.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define XELP_VERSION (0x0021)  /* HEX internal version 00.00 */

#if defined (__SDCC_mcs51)   /* the SDCC 8051 compiler needs this for setting  */
#define REENTRANT_SDCC __reentrant 
#else 
#define REENTRANT_SDCC 
#endif 

#ifdef XELP_ENABLE_FULL 			/* see xelpcfg.h */
#define XELP_ENABLE_KEY 		1   /* enable direct key press mode                            */
#define XELP_ENABLE_CLI         1   /* enable command line prompt, scripting abilities         */
#define XELP_ENABLE_THR 		1   /* enable THRU mode (redirect to other perphierals)        */
#define XELP_ENABLE_HELP		1   /* compile in built-in help function.               	   */
#define XELP_ENABLE_LCORE       1   /* enable script language features such poke, peek, go     */
#endif

#ifndef XELP_REGS_SZ
#define XELP_REGS_SZ 			(1) /* there is always atleast 1 XELP_REG.  R0 is used for fn return vals */
#endif

#if (XELP_REGS_SZ<=1)
#undef XELP_REGS_SZ
#define XELP_REGS_SZ (1)
#endif 

/*****************************************************************************
 XELP_BANNER_STR stores the following logo as a string:
           _       
       ___| |_ __  
 \ \/ / _ \ | '_ \ 
  >  <  __/ | |_) |
 /_/\_\___|_| .__/ 
            |_|    

 no compiled library space is used for this string.  
 XELP_BANNER_STR is only stored in client code if its used in the target application.
 (e.g. its a MACRO so its not put in your code unless its used.)

 If used it takes 115 bytes (incl null terminator) 
 Note: there are 6 rows of 19 chars each for those wanting to use skip-ptr logic for char displays.

 */
#define XELP_BANNER_STR  "          _       \n__  _____| |_ __  \n\\ \\/ / _ \\ | '_ \\ \n >  <  __/ | |_) |\n/_/\\_\\___|_| .__/ \n           |_|    \n"

/*****************************************************************************
 error code handling.  {errors < 0, OK==0, warnings > 0}
 Note that success is 0 (like  posix command line return
*/

typedef int XELPRESULT; 		

#ifndef XELPREG
typedef int XELPREG;
#endif

#define XELP_S_NOTFOUND	    (2)
#define XELP_W_Warn   		(1)
#define XELP_S_OK	 		(0)

#define XELP_E_Err			(-1)
#define XELP_E_CmdBufFull 	(-2)


#define XELP_T_OK(r) ((r)>=0) 	/* simple macro for testing OK or warning only */

#define XELP_CMDBUFSZ 		(64) 

/**
 used by tokenizer funciton
 */
#define XELP_TOK_ONLY 		(0x0)
#define XELP_TOK_LINE		(0x1)

/*****************************************************************************
 XelpBuf struct contains a text buffer. this is used by both the tokenizers and script engines

 when manually setting the params make sure the following relations
 are true as they are required for proper parsing:
 s <= p < e

 */
typedef struct {
    const char* s;  /* start of buf                    */
    const char* p;  /* current position                */
    const char* e;  /* s + buflen (end of buffer)      */
}XelpBufC;  /* const buffer */


typedef struct {
    char* s;  /* start of buf                    */
    char* p;  /* current position                */
    char* e;  /* s + buflen  (end of buffer)     */
} XelpBufW;  /* Writable eg non-const buffer     */

#define XelpBuf XelpBufW

/* XelpBuf MACROS */
#define XELP_XBInit(xb,buf,buflen)       {xb.s=buf; xb.p=buf; xb.e = xb.s+buflen;}      /* init from raw ptr  with length              */
#define XELP_XBInitPtrs(xb,bs,bp,be)     {xb.s=bs; xb.p=bp; xb.e=be;}                   /* init from 3 raw pointrs                     */
#define XELP_XBInitBP(xb,buf,pos,buflen) {xb.s=buf; xb.p=buf+pos; xb.e = xb.s+buflen;}  /* init from raw pts and set 'cursor' position */

/* XelpBuf const macros (no data changed) */
#define XELP_XBPCopy(a,b)				 {b.s=a.s; b.p=a.p; b.e=a.e;}                   /* copy params from XelpBuf a to XelpBuf b     */
#define XELP_XBGetBufPtr(x)              (x.s)                                          /* get start pos                               */
#define XELP_XBBufLen(x)                 ((int)((x.e)- (x.s))                           /* get length in bytes of XelpBuf              */
#define XELP_XBGetPos(x)                 ((int)(x.p-x.s))                               /* return current position as int              */
#define XELP_XBGetBuf(x,ptr,len)         {ptr=x.s; len= (x.e) - (x.s)}                  /* get the start ptr, and total length of the Xelp buf*/

/* XelpBuf writing and setting */
#define XELP_XBPUTC(x,ch)                {if (x.p<x.e){*(x.p)++ =ch;}}                   /* write char to buf */
#define XELP_XBPUTC_RAW(x,ch)            {*(x.p)++=ch;}                                  /* write char to buf no bounds check*/
#define XELP_XBGETC(x,ch)                {if (x.p<x.e)(ch=(*x.e);x.e++})                 /* get next char */
#define XELP_XBTOP(x)                    {x.p=x.s;}                                      /* set pos ptr to beginning */


/*****************************************************************************
 KeyFuncMap declares single key launched functions
 all functions must take a single integer as the parameter
 */
typedef struct
{
	XELPRESULT (*mFunPtr)(int) REENTRANT_SDCC;	/* function pointer to user-supplied fnc(int) */
	char  mKey;								    /* key press code                             */
	char* mpHelpString;						    /* use NULL or 0 if no help string is to be provided */
}XELPKeyFuncMapEntry;
/* #define XELP_KEYFUNCENTRY_LAST {0,0,""}          function list terminator */


/*****************************************************************************
 CLIFuncMap declares functions that are launched in command line mode which take
 a single string as a param.  xelp does no parsing in an argv/argc sense
 instead it just passes the "arguments" as a single string to the function pointer.
 the arguments include the name assigned to the function e.g. 
 myFunction arg1 arg2 : arg3 arg4;  
 */
typedef struct
{
	XELPRESULT (*mFunPtr)(const char *pArgString, int maxbuflen) REENTRANT_SDCC ;	/* fn ptr to command */
	char* mpCmd;                               /* name of cmd at run-time / in script                    */
	char* mpHelpString;                        /* optional help string                                   */
}XELPCLIFuncMapEntry; 
/*#define XELP_CLIFUNCENTRY_LAST {0,"",""}			 function list terminator */

#define XELP_FUNC_ENTRY_LAST	{0,0,0}


/*****************************************************************************
 key code mappings.  useful as defaults but you can any other keys in xelpcfg.h
*/
#define XELPKEY_CTA      (0x01)  /* CTRL-A  */
#define XELPKEY_CTC      (0x03)  /* CTRL-C  */
#define XELPKEY_CTK      (0x0b)  /* CTRL-K  key mode defualt key */
#define XELPKEY_CTP      (0x10)  /* CTRL-P  key mode default CLI */
#define XELPKEY_CTS      (0x13)  /* CTRL-S  */
#define XELPKEY_CTT      (0x14)  /* CTRL-T  thru mode default key */
#define XELPKEY_CTX      (0x18)  /* CTRL-X  */

#define XELPKEY_ENTER    ('\n')  /* Enter Key for Cmd Mode */
#define XELPKEY_SPC      (0x20)  /* space char             */
#define XELPKEY_BKSP	 (0x7)	 /* back space             */
#define XELPKEY_DEL		 (0x7f)	 /* DEL                    */
#define XELPKEY_ESC 	 (0x1b)  /* Escape                 */


/*****************************************************************************
 Live command modes:
 XELP_MODE_CLI   // each key is stored in buffer until <ENTER> pressed. (default)
 XELP_MODE_KEY   // each single key press is evaluated as a command
 XELP_MODE_THR   // each key is passed to the mpfThru() function.  (see docs)

 See also xelpcfg.h  which has compilation control directives if some modes are not needed.

 */
 
#define XELP_MODE_CLI 	(0x00)  
#define XELP_MODE_KEY	(0x01)	
#define XELP_MODE_THR	(0x02)


/*****************************************************************************
 XELP definition
 A runtime instance of the interpretor.  If enough memory exists several instances can
 be run at the same time.

 see xelpcfg.h for configuration options.
*/

typedef struct
{
	/* commandline state managemment [CLI | KEY | THR] */
	int						mCurMode;	     /* current mode of Xelp inst - skc/CLI/thru    */
	int						mEchoState; 	 /* whether to echo each key to the output      */

	const char* 			mpAboutMsg;      /* Used as beginning of help message           */

	XELPREG					mR[XELP_REGS_SZ];/* mR is the reg(s) used for func retn, ifOK etc (see docs) */

#ifdef XELP_ENABLE_KEY						 /* if single-key commands enabled              */
	XELPKeyFuncMapEntry		*mpKeyModeFuncs; /* key mode function dispatch                  */
#endif
	
#ifdef XELP_ENABLE_CLI						 /* if CLI and script support enabled           */
	XELPCLIFuncMapEntry		*mpCLIModeFuncs; /* command mode function dispatch              */
	char					mCmdMsgBuf[XELP_CMDBUFSZ]; 	/* cli string buffer storage        */
    XelpBuf                 mCmdXB;          /* buffer ptrs for parsing                     */
#endif


#ifdef XELP_CLI_PROMPT 						 /* prompt for CLI enabled                      */
	const char*				mpPrompt;		 /* prompt at beginning of CLI e.g. xelp>		*/
#endif	

	/****
	platform dependant dispatch functions  (light-weight hardware abstraction layer)
	note that if any are left unset (zero) this is OK as system will not call null ptrs.
	*/
	void (*mpfOut)(char); 		  /* function to emit chars to console                       */
	void (*mpfErr)(char);		  /* function to handle errors (optional callback)           */
	void (*mpfEditModeChg)(int);  /* function called when key entry mode changed (optional)  */

#ifdef XELP_ENABLE_THR	
	void (*mpfPassThru)(char);    /* function to pass keys in thru mode                      */
#endif 	
#ifdef XELP_ENABLE_CLI	
	void (*mpfBksp)();			  /* function to handle destructive backspace at CLI prompt  */
#endif

#ifdef XELP_STACK_MACHINE
	int 					mS[XELP_STACK_DEPTH]; /* integer stack for XelpStackMachine 	 */
#endif 
}XELP;


/*****************************************************************************
 XELP API Functions Here
 */

XELPRESULT XELPInit (XELP *ths, const char *pAboutMsg);			    /* initialize instance             */

/*  Macros to set function pointer arrays	  */
#define XELP_SET_FN_CLI(ths,pfaCLI)	  (ths.mpCLIModeFuncs=pfaCLI)   /* load CLI fns table              */
#define XELP_SET_FN_KEY(ths,pfaKey)	  (ths.mpKeyModeFuncs=pfaKey)   /* load KEY fns table              */

/*  Macros to set Platform Abstraction Layer Functions */
#define XELP_SET_FN_OUT(ths,pfOut)     (ths.mpfOut=pfOut)           /* print out chars                 */
#define XELP_SET_FN_THR(ths,pfThru)    (ths.mpfPassThru=pfThru)     /* Thru callback                   */
#define XELP_SET_FN_ERR(ths,pfErr)     (ths.mpfErr=pfErr)           /* Error callback                  */
#define XELP_SET_FN_EMCHG(ths,pfEMCHG) (ths.mpfEditModeChg=pfEMCHG) /* Entry Mode Change               */
#define XELP_SET_FN_BKSP(ths,pfBKSP)   (ths.mpfBksp=pfBKSP)	        /* Handle Backspace                */

#define XELP_SET_VAL_CLI_PROMPT(ths,prompt)	(ths.mpPrompt=prompt)   /* set per instnce prompt if enabled in xelpcfg.h  */

#ifdef XELP_ENABLE_HELP
XELPRESULT XELPHelp	        (XELP *ths);                             /* print online help (if avail)    */
#endif

/* Xelp API functions */
XELPRESULT XELPOut 		    (XELP *ths, const char* msg, int maxlen);/* print function                  */
#define XELPOutXB(x,xb)     (XELPOut(x,xb.p,(int((xb.e-xb.p)))       /* print a XelpBuf from cur pos    */
XELPRESULT XELPExecKC		(XELP *ths, char key);				     /* execute key command             */
XELPRESULT XELPParse 		(XELP *ths, const char *buf, int blen);  /* execute CLI or script commands  */
XELPRESULT XELPParseXB      (XELP *ths, XelpBuf *script);            /* execute CLI or script commands  */
XELPRESULT XELPParseKey 	(XELP *ths, char key);				     /* handle keypress at CLI          */

/* XELPTokLine is the main tokenizer which can get next token or line at time                           */
/* XELPRESULT XELPTokLine (const char *buf, int blen, const char **t0s, const char **t0e, const char **eol, int srchType); */
//XELPRESULT XELPTokLine ( char *buf, char *bufend, const char **t0s, const char **t0e, const char **eol, int srchType); 
XELPRESULT XELPTokLineXB (XelpBuf *buf, XelpBuf *tok, int srchType);
XELPRESULT XELPTokN (XelpBuf *buf, int n, XelpBuf *tok);
XELPRESULT XelpNumToks (XelpBuf *buf, int *n);

/* XELPNEXTTOK get next token in a string buffer.  This is just a macro call to XELPTokLine             */
//#define    XELPNEXTTOK(buf,blen,tok_s,tok_e)    (XELPTokLine(buf, buf+blen, tok_s, tok_e, 0, XELP_TOK_ONLY))
int        XELPStrLen(const char* c);                               /* compute length of null terminated string. */ 
XELPRESULT XELPStrEq (const char* pbuf, int blen, const char *cmd);
XELPRESULT XELPStrEq2 (const char* pbuf, const char* pend, const char *cmd);
int        XELPStr2Int(const char* s,int  maxlen);                  /* parse a str->int accepts hex as 123h or signed decimal num.  no safety for non-num chars */   
XELPRESULT XelpParseNum (const char* s, int maxlen, int* n);        /* parse a str returns a number if successful */
XELPRESULT XELPFindTok(XelpBuf *x, const char *t0s, const char *t0e, int srchType); /* find matching tok (next tok || next label) */

/* XelpBufCmp() compare buffers / string */
XELPRESULT XelpBufCmp (const char *as, const char *ae, const char *bs, const char * be, int cmpType); 
#define XELP_CMP_TYPE_BUF   (0x00) /* both buffers are only tested for byte for byte comparison by length (\0 is ignored)       */
#define XELP_CMP_TYPE_A0    (0x01) /* buffer a also treats \0 as a end of buffer                                                */
#define XELP_CMP_TYPE_A0B0  (0x11) /* if either buffer has \0 that is treated as the end of the buffer like in stdlib::strcmp() */

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


#ifdef __cplusplus
}
#endif

#endif /* __XELPh__ */

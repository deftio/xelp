/*

  @xelp.h - header file for xelp command interpreter
 		
  @copy Copyright (C) <2012>  <M. A. Chatterjee>
  @author M A Chatterjee <deftio [at] deftio [dot] com>
  @version 0.21 M. A. Chatterjee
 
  This file contains header defintions for the xelp simple embedded command interpreter.

  @license: 
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

 */

#ifndef __XELPh__
#define __XELPh__

#include "xelpcfg.h"

#ifdef __cplusplus
extern "C"
{
#endif


#if defined (SDCC_mcs51)   /* the SDCC 8051 compiler needs this for setting interrupts */
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

/********************************
 error code handling.  {errors < 0, OK==0, warnings > 0}
 Note that success is 0 (like  posix command line return
*/

typedef int XELPRESULT; 		

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



/**********************************************************
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


/**********************************************************
 CLIFuncMap declares functions that are launched in command line mode which take
 a single string as a param.  xelp does no parsing in an argv/argc sense
 instead it just passes the "arguments" as a single string to the function pointer.
 the arguments include the name assigned to the function e.g. 
 myFunction arg1 arg2 : arg3 arg4;  
 */
typedef struct
{
	XELPRESULT (*mFunPtr)(const char *pArgString, int maxbuflen) REENTRANT_SDCC ;	
	char* mpCmd;
	char* mpHelpString;
}XELPCLIFuncMapEntry; 
/*#define XELP_CLIFUNCENTRY_LAST {0,"",""}			 function list terminator */

#define XELP_FUNC_ENTRY_LAST	{0,0,0}


/** 
 key code mappings.  useful as defaults but you can supply any in the xelpcfg.h
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


/***********************************************************
 Live command modes:
 XELP_MODE_CLI   // each key is stored in buffer until <ENTER> pressed. (default)
 XELP_MODE_KEY   // each single key press is evaluated as a command
 XELP_MODE_THR   // each key is passed to the mpfThru() function.  (see docs)

 See also xelpcfg.h  which has compilation control directives if some modes are not needed.

 */
 
#define XELP_MODE_CLI 	(0x00)  
#define XELP_MODE_KEY	(0x01)	
#define XELP_MODE_THR	(0x02)


/** 
 XELP
 A runtime instance of the interpretor.  If enough memory exists several instances can
 be run at the same time.

 see xelpcfg.h for configuration options.

*/

typedef struct
{
	/* commandline state managemment [CLI | KEY | THR] */
	int						mCurMode;	
	int						mEchoState; 	 /* whether to echo each key to the output      */

	const char* 			mpAboutMsg;      /* Used as beginning of help message           */

#ifdef XELP_ENABLE_KEY						 /* if single-key commands enabled              */
	XELPKeyFuncMapEntry		*mpKeyModeFuncs; /* key mode function dispatch                  */
#endif
	
#ifdef XELP_ENABLE_CLI						 /* if CLI and script support enabled           */
	XELPCLIFuncMapEntry		*mpCLIModeFuncs; /* command mode function dispatch              */
	char					mCmdMsgBuf[XELP_CMDBUFSZ]; 	/* cli string buffer.               */
	int 					mCmdBufSize;     /* cli Buf size                                */
	int						mCmdMsgIndex;    /* current cursor position                     */
#endif


#ifdef XELP_CLI_PROMPT 						 /* prompt for CLI enabled                      */
	const char*				mpPrompt;		 /* prompt at beginning of CLI e.g. dio>		*/
#endif	

	/****
	platform dependant dispatch functions  (light-weight hardware abstraction layer)
	note that if any are left unset (zero) this is OK as system will not call null ptrs.
	*/
	void (*mpfOut)(char); 		  /* function to emit chars to console                       */
	void (*mpfErr)(char);		  /* function to handle errors (optional callback)           */
	void (*mpfEditModeChg)(int);  /* function called when key mode is changed.               */

#ifdef XELP_ENABLE_THR	
	void (*mpfPassThru)(char);    /* function to pass keys in thru mode                      */
#endif 	
#ifdef XELP_ENABLE_CLI	
	void (*mpfBksp)();			  /* function to handle destructive backspace at CLI prompt  */
#endif
}XELP;


/***************************************************************
 * XELP API Begins Here
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
#define XELP_SET_FN_BKSP(ths,pfBKSP)	  (ths.mpfBksp=pfBKSP)	    /* Handle Backspace                */

#define XELP_SET_VAL_CLI_PROMPT(ths,prompt)	(ths.mpPrompt=prompt)  /* set per instnce prompt if enabled in xelpcfg.h  */

#ifdef XELP_ENABLE_HELP
XELPRESULT XELPHelp		(XELP *ths);                                /* print online help (if avail)    */
#endif

XELPRESULT XELPOut 		    (XELP *ths, const char* msg, int maxlen);  /* print function                  */
XELPRESULT XELPExecKC		(XELP *ths, char key);				     /* execute key command             */
XELPRESULT XELPParse 		(XELP *ths, const char *buf, int blen);  /* execute CLI or script commands  */
XELPRESULT XELPParseKey 	(XELP *ths, char key);				     /* handle keypress at CLI          */

/* XELPTokLine is the main tokenizer which can get next token or line at time                          */
XELPRESULT XELPTokLine (const char *buf, int blen, const char **t0s, const char **t0e, const char **eol, int srchType); 

/* XELPNEXTTOK get next token in a string buffer.  This is just a macro call to DioTokLine             */
#define   XELPNEXTTOK(buf,blen,tok_s,tok_e)    (XELPTokLine(buf, blen, tok_s, tok_e, 0, XELP_TOK_ONLY)) 
int       XELPStr2Int (const char* s,int  maxlen);                  /* parse a str->int accepts hex as 123h or signed decimal num.  no safety for non-num chars */   



#ifdef __cplusplus
}
#endif

#endif /* __XELPh__ */

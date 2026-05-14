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

#include "xelpcfg.h"  /* this file has config options such as escape chars and included modules */

#ifdef __cplusplus
extern "C"
{
#endif

#define XELP_VERSION      (0x00000303UL) /* 32-bit version: 0x00MMmmpp (major.minor.patch) */
#define XELP_VER_MAJOR(v) (((v) >> 16) & 0xFF)
#define XELP_VER_MINOR(v) (((v) >>  8) & 0xFF)
#define XELP_VER_PATCH(v) ( (v)        & 0xFF)

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
#endif

#ifndef XELP_REGS_SZ
#define XELP_REGS_SZ 			(4) /* 4 callee-clobbers-all return registers per instance */
#endif

#if (XELP_REGS_SZ < 4)
#undef XELP_REGS_SZ
#define XELP_REGS_SZ (4)
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

 printf style usage:
 
 printf(XELP_BANNER_STR);


 */
#define XELP_BANNER_STR  "          _       \n__  _____| |_ __  \n\\ \\/ / _ \\ | '_ \\ \n >  <  __/ | |_) |\n/_/\\_\\___|_| .__/ \n           |_|    \n"

/*****************************************************************************
 error code handling.  {errors < 0, OK==0, warnings > 0}
 Note that success is 0 (like  posix command line return)
*/

typedef int XELPRESULT; 		

#ifndef XELPREG
typedef int XELPREG;
#endif

typedef unsigned long XELPKEYCODE;

/* XELPKEYCODE constants for multi-byte keys.
   Packed little-endian: byte[0] in bits 0-7, byte[1] in 8-15, etc.
   Single-char keys are their natural value (e.g. 'a' == 0x61).
   Multi-byte keys are always >= 0x100 (byte[1] != 0). */
#define XELP_KEYCODE_UP    (0x00415B1BUL)  /* ESC [ A */
#define XELP_KEYCODE_DOWN  (0x00425B1BUL)  /* ESC [ B */
#define XELP_KEYCODE_RIGHT (0x00435B1BUL)  /* ESC [ C */
#define XELP_KEYCODE_LEFT  (0x00445B1BUL)  /* ESC [ D */
#define XELP_KEYCODE_HOME  (0x00485B1BUL)  /* ESC [ H */
#define XELP_KEYCODE_END   (0x00465B1BUL)  /* ESC [ F */
#define XELP_KEYCODE_INS   (0x7E325B1BUL)  /* ESC [ 2 ~ */
#define XELP_KEYCODE_KDEL  (0x7E335B1BUL)  /* ESC [ 3 ~ */
#define XELP_KEYCODE_PGUP  (0x7E355B1BUL)  /* ESC [ 5 ~ */
#define XELP_KEYCODE_PGDN  (0x7E365B1BUL)  /* ESC [ 6 ~ */

#define XELP_KC_B0(k)  ((char)( (k)        & 0xFF))
#define XELP_KC_B1(k)  ((char)(((k) >>  8) & 0xFF))
#define XELP_KC_B2(k)  ((char)(((k) >> 16) & 0xFF))
#define XELP_KC_B3(k)  ((char)(((k) >> 24) & 0xFF))
#define XELP_KC_IS_MULTI(k) ((k) >= 0x100UL)

#define XELP_S_NOTFOUND	    (2)
#define XELP_W_WARN   		(1)
#define XELP_S_OK	 		(0)

#define XELP_E_ERR			(-1)
#define XELP_E_CMDBUFFULL 	(-2)
#define XELP_E_CMDNOTFOUND  (-3)

#define XELP_T_OK(r) ((r)>=0) 	/* simple macro for testing OK or warning (e.g. not a failure) */


#ifndef XELP_HIST_DEPTH
#define XELP_HIST_DEPTH		(4)  /* history ring depth (overridable)     */
#endif

#ifndef XELP_CMDBUFSZ
#define XELP_CMDBUFSZ 		(64)
#endif

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
#define XELP_XB_INIT(xb,buf,buflen)       {xb.s=buf; xb.p=buf; xb.e = xb.s+buflen;}      /* init from raw ptr  with length              */
#define XELP_XB_INIT_PTRS(xb,bs,bp,be)    {xb.s=bs; xb.p=bp; xb.e=be;}                   /* init from 3 raw pointrs                     */
#define XELP_XB_INIT_BP(xb,buf,pos,buflen) {xb.s=buf; xb.p=buf+pos; xb.e = xb.s+buflen;} /* init from raw pts and set 'cursor' position */

/* XelpBuf const macros (no data changed) */
#define XELP_XB_COPY(a,b)                 {b.s=a.s; b.p=a.p; b.e=a.e;}                   /* copy params from XelpBuf a to XelpBuf b     */
#define XELP_XB_PTR(x)                    (x.s)                                           /* get start pos                               */
#define XELP_XB_LEN(x)                    ((int)((x.e)- (x.s)))                           /* get length in bytes of XelpBuf              */
#define XELP_XB_POS(x)                    ((int)(x.p-x.s))                                /* return current position as int              */

/* XelpBuf writing and setting */
#define XELP_XB_PUTC(x,ch)                {if (x.p<x.e){*(x.p)++ =ch;}}                  /* write char to buf */
#define XELP_XB_PUTC_RAW(x,ch)            {*(x.p)++=ch;}                                  /* write char to buf no bounds check*/
#define XELP_XB_GETC(x,ch)                {if (x.p<x.e){ch=(*x.p);x.p++;}}               /* get next char */
#define XELP_XB_TOP(x)                    {x.p=x.s;}                                      /* set pos ptr to beginning */


/*****************************************************************************
 Forward declaration -- allows function pointers to reference the XELP instance
 */
struct XELP_tag;

/*****************************************************************************
 KeyFuncMap declares single key launched functions
 all functions must take a single integer as the parameter
 */
typedef struct
{
	XELPRESULT (*mFunPtr)(struct XELP_tag *, XELPKEYCODE) REENTRANT_SDCC;	/* function pointer to user-supplied fnc(ths, keycode) */
	XELPKEYCODE mKey;						    /* key press code (single char or packed multi-byte)  */
	const char* mpHelpString;				    /* use NULL or 0 if no help string is to be provided  */
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
	XELPRESULT (*mFunPtr)(struct XELP_tag *, const char *pArgString, int maxbuflen) REENTRANT_SDCC ;	/* fn ptr to command */
	const char* mpCmd;                         /* name of cmd at run-time / in script                    */
	const char* mpHelpString;                  /* optional help string                                   */
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
#define XELPKEY_BKSP	 (0x7)	 /* back space (legacy)    */
#define XELPKEY_BS       (0x08)  /* ASCII BS               */
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

typedef struct XELP_tag
{
	/* commandline state managemment [CLI | KEY | THR] */
	int						mCurMode;	     /* current mode of Xelp inst - skc/CLI/thru    */
	char					mOutEnable;		 /* 0 = mute all output, nonzero = normal        */
	char					mEchoChar;		 /* '\0' = normal, '\1' = suppress, else mask    */

	const char* 			mpAboutMsg;      /* Used as beginning of help message           */

	XELPREG					mR[XELP_REGS_SZ]; /* return registers (callee-clobbers-all, see docs) */

#ifdef XELP_ENABLE_KEY						 /* if single-key commands enabled              */
	XELPKeyFuncMapEntry		*mpKeyModeFuncs; /* key mode function dispatch                  */
	XELPRESULT (*mpfDefKey)(struct XELP_tag *, XELPKEYCODE) REENTRANT_SDCC; /* default handler for unmapped keys */
#endif

#ifdef XELP_ENABLE_CLI						 /* if CLI and script support enabled           */
	XELPCLIFuncMapEntry		*mpCLIModeFuncs; /* command mode function dispatch              */
	XELPRESULT (*mpfDefCLI)(struct XELP_tag *, const char *, int) REENTRANT_SDCC; /* default handler for unknown commands */
	char					mCmdMsgBuf[XELP_CMDBUFSZ]; 	/* cli string buffer storage        */
    XelpBuf                 mCmdXB;          /* buffer ptrs for parsing                     */
#endif

#ifdef XELP_ENABLE_ARGV						 /* if structured argv parsing enabled          */
	char					mArgvBuf[XELP_ARGVBUFSZ]; /* scratch buffer for XelpBuf2Argv    */
#endif

#ifdef XELP_CLI_PROMPT 						 /* prompt for CLI enabled                      */
	const char*				mpPrompt;		 /* prompt at beginning of CLI e.g. xelp>		*/
#endif	

	/* Key accumulator for multi-byte sequences (e.g. arrow keys) */
	XELPKEYCODE				mKeyAccum;		 /* packed key being assembled                  */
	char					mKeyLen;		 /* bytes accumulated (0 = idle)                */

#if defined(XELP_ENABLE_CLI) && defined(XELP_ENABLE_LINE_EDIT)
	char*					mCur;			 /* cursor position in [mCmdXB.s .. mCmdXB.p]  */
#endif

#if defined(XELP_ENABLE_CLI) && defined(XELP_ENABLE_HISTORY)
	char  mHistBuf[XELP_HIST_DEPTH][XELP_CMDBUFSZ]; /* history ring             */
	char  mHistWrite;    /* next write slot (ring index)                          */
	char  mHistCount;    /* entries stored (0..DEPTH)                             */
	char  mHistBrowse;   /* browse position (-1 = not browsing)                  */
	char  mHistSaved[XELP_CMDBUFSZ]; /* stash of in-progress line on first UP   */
	char  mHistSavedLen; /* length of saved in-progress line                     */
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

}XELP;


/*****************************************************************************
 XELP API Functions Here
 */

XELPRESULT XelpInit (XELP *ths, const char *pAboutMsg);			    /* initialize instance             */
#define XELP_SET_ABOUT(ths,pAboutMsg)     (ths.mpAboutMsg=pAboutMsg)/* change the about message        */

/*  Macros to set function pointer arrays	  */
#define XELP_SET_FN_CLI(ths,pfaCLI)	     (ths.mpCLIModeFuncs=pfaCLI)   /* load CLI fns table              */
#define XELP_SET_FN_KEY(ths,pfaKey)	     (ths.mpKeyModeFuncs=pfaKey)   /* load KEY fns table              */
#define XELP_SET_FN_DEF_CLI(ths,pfDef)   (ths.mpfDefCLI=pfDef)        /* default handler for unknown cmds*/
#define XELP_SET_FN_DEF_KEY(ths,pfDef)   (ths.mpfDefKey=pfDef)        /* default handler for unmapped keys*/

/*  Macros to set Platform Abstraction Layer Functions */
#define XELP_SET_FN_OUT(ths,pfOut)     (ths.mpfOut=pfOut)           /* print out chars                 */
#define XELP_SET_FN_THR(ths,pfThru)    (ths.mpfPassThru=pfThru)     /* Thru callback                   */
#define XELP_SET_FN_ERR(ths,pfErr)     (ths.mpfErr=pfErr)           /* Error callback                  */
#define XELP_SET_FN_EMCHG(ths,pfEMCHG) (ths.mpfEditModeChg=pfEMCHG) /* Entry Mode Change               */
#define XELP_SET_FN_BKSP(ths,pfBKSP)   (ths.mpfBksp=pfBKSP)	        /* Handle Backspace                */

#define XELP_SET_VAL_CLI_PROMPT(ths,prompt)	(ths.mpPrompt=prompt)   /* stored by ptr: must be \0 terminated, must outlive instance */

/* Register access macros -- callee-clobbers-all convention.
   R0: command status (written by engine after dispatch).
   R1-R3: command-specific return values (engine never touches these).
   All registers may be overwritten by any command call. 
   */
#define XELP_R0(ths) ((ths).mR[0])
#define XELP_R1(ths) ((ths).mR[1])
#define XELP_R2(ths) ((ths).mR[2])
#define XELP_R3(ths) ((ths).mR[3])

#define XELP_SET_R0(ths,val) ((ths).mR[0]=(val))
#define XELP_SET_R1(ths,val) ((ths).mR[1]=(val))
#define XELP_SET_R2(ths,val) ((ths).mR[2]=(val))
#define XELP_SET_R3(ths,val) ((ths).mR[3]=(val))


/* Echo and output control constants */
#define XELP_ECHO_NORMAL  '\0'   /* echo typed char as-is (default) */
#define XELP_ECHO_OFF     '\1'   /* suppress echo entirely          */

/* Echo and output control macros */
#define XELP_SET_OUT_ENABLE(ths, val)  ((ths).mOutEnable = (val))
#define XELP_GET_OUT_ENABLE(ths)       ((ths).mOutEnable)
#define XELP_SET_ECHO(ths, ch)         ((ths).mEchoChar = (ch))
#define XELP_GET_ECHO(ths)             ((ths).mEchoChar)

#ifdef XELP_ENABLE_HELP
XELPRESULT XelpHelp	        (XELP *ths);                             /* print online help (if avail)    */
#endif

/* Xelp API functions */
XELPRESULT XelpOut 		    (XELP *ths, const char* msg, int maxlen);/* print string                    */
XELPRESULT XelpPutc         (XELP *ths, char c);                     /* print single char               */
#define XELP_XB_OUT(x,xb)   (XelpOut(x,xb.p,(int)(xb.e-xb.p)))       /* print a XelpBuf from cur pos    */
XELPRESULT XelpExecKC		(XELP *ths, XELPKEYCODE key);		     /* execute key command             */
XELPRESULT XelpParse 		(XELP *ths, const char *buf, int blen);  /* execute CLI or script commands  */
XELPRESULT XelpParseXB      (XELP *ths, XelpBuf *script);            /* execute CLI or script commands  */
XELPRESULT XelpParseKey 	(XELP *ths, char key);				     /* handle keypress at CLI          */

/* XelpTokLine is the main tokenizer which can get next token or line at time                           */
/* XELPRESULT XelpTokLine (const char *buf, int blen, const char **t0s, const char **t0e, const char **eol, int srchType); */
/* XELPRESULT XelpTokLine ( char *buf, char *bufend, const char **t0s, const char **t0e, const char **eol, int srchType); */
XELPRESULT XelpTokLineXB (XelpBuf *buf, XelpBuf *tok, int srchType);
XELPRESULT XelpTokN (XelpBuf *buf, int n, XelpBuf *tok);
XELPRESULT XelpNumToks (XelpBuf *buf, int *n);

/*****************************************************************************
 XelpArgs -- sequential argument iterator for CLI command handlers.

 Provides O(1)-per-token left-to-right iteration.  argv[0] is the command
 name (no auto-skip).  XelpNextTok yields a XelpBuf (tok.s = start,
 tok.p = end); use XELP_XB_PTR/XELP_XB_LEN or pass to XelpStrEq2.
 Tokens are NOT null-terminated (buffer is not modified).
 Token pointers are valid only during the callback.
 */
typedef struct {
    XelpBuf buf;    /* tokenizer state (cursor advances as tokens are consumed) */
} XelpArgs;

XELPRESULT XelpArgsInit  (XelpArgs *a, const char *args, int len);
XELPRESULT XelpNextTok   (XelpArgs *a, XelpBuf *tok);
XELPRESULT XelpNextInt   (XelpArgs *a, int *val);
XELPRESULT XelpArgCount  (XelpArgs *a, int *n);

/* Direct-access argument helpers (random access, O(N) per call).
   Arg 0 is the command name, arg 1 is the first real argument. */
XELPRESULT XelpArgInt (const char *args, int len, int n, int *val);
XELPRESULT XelpArgStr (const char *args, int len, int n,
                       const char **s, int *slen);

#ifdef XELP_ENABLE_ARGV
/* Tokenize args into argc/argv using ths->mArgvBuf as scratch buffer.
   Strips quotes, processes escape sequences, null-terminates each token.
   argv[0] = command name per argc/argv convention.
   Returns XELP_E_ERR if input exceeds scratch buffer or too many args. */
XELPRESULT XelpBuf2Argv(XELP *ths, const char *args, int len,
                         int *argc, const char **argv, int maxargs);

/* Get argv[n] as an integer.  Returns XELP_E_ERR if out of range or not numeric. */
XELPRESULT XelpArgvInt(const char **argv, int argc, int n, int *val);

/* Get argv[n] as a string pointer and length. */
XELPRESULT XelpArgvStr(const char **argv, int argc, int n, const char **s, int *slen);

/* Convenience macro for command handlers (C99+ / C++).
   Declares local 'int argc' and 'const char *argv[XELP_ARGV_MAX]',
   tokenizes args, returns XELP_E_ERR on failure.
   Place at the top of a handler body, before other statements. */
#define XELP_PARSE_ARGV(ths, args, len) \
    const char *argv[XELP_ARGV_MAX]; \
    int argc = 0; \
    if (XelpBuf2Argv((ths), (args), (len), &argc, argv, XELP_ARGV_MAX) \
        != XELP_S_OK) \
        return XELP_E_ERR
#endif

/* XELPNEXTTOK get next token in a string buffer.  This is just a macro call to XelpTokLine             */
/* #define    XELPNEXTTOK(buf,blen,tok_s,tok_e)    (XelpTokLine(buf, buf+blen, tok_s, tok_e, 0, XELP_TOK_ONLY)) */
int        XelpStrLen(const char* c);                               /* compute length of null terminated string. */ 
XELPRESULT XelpStrEq (const char* pbuf, int blen, const char *cmd);
XELPRESULT XelpStrEq2 (const char* pbuf, const char* pend, const char *cmd);
int        XelpStr2Int(const char* s,int  maxlen);                  /* parse a str->int accepts hex as 123h or signed decimal num.  no safety for non-num chars */   
XELPRESULT XelpParseNum (const char* s, int maxlen, int* n);        /* parse a str returns a number if successful */
XELPRESULT XelpFindTok(XelpBuf *x, const char *t0s, const char *t0e, int srchType); /* find matching tok (next tok || next label) */

/* XelpBufCmp() compare buffers / string */
XELPRESULT XelpBufCmp (const char *as, const char *ae, const char *bs, const char * be, int cmpType); 
#define XELP_CMP_TYPE_BUF   (0x00) /* both buffers are only tested for byte for byte comparison by length (\0 is ignored)       */
#define XELP_CMP_TYPE_A0    (0x01) /* buffer a also treats \0 as a end of buffer                                                */
#define XELP_CMP_TYPE_A0B0  (0x11) /* if either buffer has \0 that is treated as the end of the buffer like in stdlib::strcmp() */


#ifdef __cplusplus
}
#endif

#endif /* __XELPh__ */

/*
 @xelp.c - implementation
 		
 @copy Copyright (C) <2012>  <M. A. Chatterjee>
 @author M A Chatterjee <deftio [at] deftio [dot] com>
 
 This file contains implementation for the xelp simple embedded command  interpreter.
 
 @license: 
	Copyright (c) M. A. Chatterjee <deftio at deftio dot com>
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
#include "xelp.h" 		 

/**
local defines (this file only)
 */
#ifndef _PUTC
#define _PUTC(c)	((ths->mpfOut)((char)(c)))   /* write char to output */
#endif
/***************************************** 
 XELPStrLen() - find length of a string in bytes assuming its null terminated
 */
int XELPStrLen (const char* c) {
    int l=0;
    while (*c++ != 0) {
        l++;
    }
    return l;
}

/***************************************** 
 XELPOut() - print a string.
 takes a length specified string and prints to output stream
 */
XELPRESULT XELPOut (XELP *ths, const char* msg, int maxlen)
{
	//maxlen = (maxlen == 0) ? -1 : maxlen;  //truly a C hack.  let maxlen wrap around
	if ((0 == msg) || (0==ths->mpfOut))
		return XELP_S_OK;
	while ((*msg !=0) && (maxlen--)){
		(ths->mpfOut)(*msg++);
	}
	return XELP_S_OK;	
}
/******************************************
 XELPHelp() - print out help strings for functions
 see xelpcfg.h for setting or overriding
 XELP_HELP_ABT_STR, XELP_HELP_KEY_STR, XELP_HELP_CLI_STR
 */
#ifdef XELP_ENABLE_HELP
XELPRESULT XELPHelp(XELP* ths)
{
#ifdef XELP_ENABLE_KEY
	XELPKeyFuncMapEntry *e = ths->mpKeyModeFuncs;
#endif
#ifdef XELP_ENABLE_CLI
	XELPCLIFuncMapEntry *s = ths->mpCLIModeFuncs;
#endif
	const int x=0xff;

	XELPOut(ths,XELP_HELP_ABT_STR,x);
#ifdef XELP_ENABLE_KEY
	if (e->mFunPtr) { //check and see if first entry is not terminator
		XELPOut(ths,XELP_HELP_KEY_STR,x);
		while (e->mFunPtr)	{
			_PUTC(e->mKey);
            //XELPOut(ths,&(e->mKey),1);
			_PUTC(':');
			XELPOut(ths,e->mpHelpString,x);
			_PUTC('\n');
			e++;
		}
	}
#endif
#ifdef XELP_ENABLE_CLI
	if (s->mFunPtr) {
		XELPOut(ths,XELP_HELP_CLI_STR,x);
		while (s->mFunPtr)	{
			XELPOut(ths,s->mpCmd,x);
			_PUTC(':');
			XELPOut(ths,s->mpHelpString,x);			
			_PUTC('\n');	
			s++;	
		}
	}
#endif
	return XELP_S_OK;
}
#endif
XELPRESULT XELPInit 	 (
						XELP *ths,
						const char *			pAboutMsg
						)
{
	/* manuual zeroing out code.  allows dynamic creation of XELP * objects and needed.  
	   some _older_ compilers didn't support zeroed out static initialization constructs 
	   e.g = {0} etc 
	 */
	int i=sizeof(XELP);
 	char *p = (char *) ths;  
	while (i--)
		*p=0;
	
	ths->mpAboutMsg = pAboutMsg;
	XELP_XBInit(ths->mpCmdBuf,ths->mCmdMsgBuf,XELP_CMDBUFSZ-1);
	/* comand mode mssage index 
	ths->mCmdMsgIndex = 0;  //set to 0 by ptr loop at top
	*/
	/* i/o handlers			note: each set to 0 using ptr loop
	ths->mpfOut=0; 	  		// used for data output
	ths->mpfPassThru=0;   	// function to pass keys in normal mode
	ths->mpfBksp=0;			// function to handle destructive backspace
	ths->mpErr=0;			// function to handle error reporting
	ths->mpfEditModeChg=0;	// function for CLI/Key/Thru mode change callbacks
	*/
	
	return XELP_S_OK;
}
/****************************
 XELPStrEq() : test if 2 strings are equal.  used for parsing commands at CLI, scripts
 cmd is assumed to be 0 terminated e.g. "mycommand" === mycommand0 
*/
#ifdef XELP_ENABLE_CLI
XELPRESULT XELPStrEq (const char* pbuf, int blen, const char *cmd)
{
	if (0 == blen)
		return XELP_S_NOTFOUND;
	while(blen--){
		if (*cmd == 0) 
			return XELP_S_NOTFOUND;
		if (*pbuf++ != *cmd++)
			return XELP_S_NOTFOUND;
	}
	if (*cmd != 0)
		return XELP_S_NOTFOUND;
	return XELP_S_OK;
}

XELPRESULT XELPStrEq2 (const char* pbuf, const char* pend, const char *cmd)
{
	while(pbuf<pend){
		if (*cmd == 0) 
			return XELP_S_NOTFOUND;
		if (*pbuf++ != *cmd++)
			return XELP_S_NOTFOUND;
	}
	if (*cmd != 0)
		return XELP_S_NOTFOUND;
	return XELP_S_OK;
}
/********************************************************
 XELPBufCmp() : test if 2 buffers have byte for byte equality.  Used for finding if tokens match commands or labels
 
 cmpType: (comparison type)
 XELP_CMP_TYPE_BUF : both buffers are only tested for byte for byte comparison by length (\0 is not treated as end-of-buf)
 XELP_CMP_TYPE_A0  : buffer a also treats \0 as a end of buffer 
 XELP_CMP_TYPE_A0B0  : if either buffer has \0 that is treated as the end of the buffer in addition to reaching the end ptr
 
*/
XELPRESULT XelpBufCmp (const char *as, const char *ae, const char *bs, const char *be, int cmpType) 
{
    while ((as < ae) && (bs < be) ) {
        if (*as != *bs)
            return XELP_S_NOTFOUND;

        if (cmpType == XELP_CMP_TYPE_A0) {
            if (*as == 0) {ae = as+1;}
        }

        if (cmpType == XELP_CMP_TYPE_A0B0) {
            if (*as == 0) {ae = as+1;}
            if (*bs == 0) {be = bs+1;}
        }
        
        as++;
        bs++;
    }
    if ((as == ae) && (bs == be)) /* both ptrs shoud be at end of buf now (includes null term case if applicable) */
        return XELP_S_OK;

    return XELP_S_NOTFOUND; 
}
/********************************************************
 XELPFindTok() : find a matching token in a buffer.  The buffer is a XelpBuf, the token is passed as 
    ptr to its beginning and a ptr to one position beyond its end 
 
 srchType: 
 XELP_TOK_ONLY  : find the next token (if any) that matches the supplied token
 XELP_TOK_LINE  : find the next token that matches the supplied token, but only if its the first token of the line.
 if successful returns XELP_S_OK and x->p points to the position _just_after_ the found token.
 else returns XELP_S_NOTFOUND and (x->p) == (x->e)

*/

XELPRESULT XELPFindTok(XelpBuf *x, const char *t0s, const char *t0e, int srchType) 
{
    XelpBuf tok;
    
    while (XELP_S_OK == XELPTokLineXB(x,&tok,srchType)) {
        if (XELP_S_OK == XelpBufCmp(t0s,t0e,tok.s,tok.p,XELP_CMP_TYPE_BUF))
            return XELP_S_OK;
    }
    return XELP_S_NOTFOUND;
}
#endif
/********************************************************
XELPRESULT XELPExecKC(char)  : (execute key-command)

Attempts to execute first matching single-key command.  
the key value is passed to the command as an int

This is used in KEY (menu driven) mode or can be called from any C function.  

e.g.
XelpExecKC(myInstance,'a');  // execute the single key command 'a' if it exists

*/
#ifdef XELP_ENABLE_KEY
XELPRESULT XELPExecKC(XELP* ths, char key) {
	XELPKeyFuncMapEntry *p = ths->mpKeyModeFuncs;
	while (p->mFunPtr) {
		if (p->mKey == key)			{
			ths->mR[0] = p->mFunPtr((int)key);
			return ths->mR[0];
		}
		p++;
	}
    ths->mR[0] = XELP_S_NOTFOUND;
	return ths->mR[0];
}
#endif

#ifdef XELP_ENABLE_CLI
/*****************************/
/* machine generated section */
/**
 begin parser state machine model.
 */
#define _EF_TS     (0x01) /* set token 0 start (1st token from buf start) */
#define _EF_TE     (0x02) /* set token 0 end */
#define _EF_LE     (0x04) /* set line end */

#define _PS_SEEK   (0x00)  /* seek next token0 */
#define _PS_ESCA   (0x01)  /* esc sequence */
#define _PS_TOK0   (0x02)  /* token0 (the command / opterator */
#define _PS_CMNT   (0x03)  /* single-line comment */
#define _PS_SEOL   (0x04)  /* seek end-of-line */
#define _PS_QUOT   (0x05)  /* quoted string */
#define _PS_QESC   (0x06)  /* quoted esc char */
#define _PS_QEND   (0x07)  /* quoted end */
#define _PS_PREV   (0x08)  /* use previous state (spec case) */
#define _PS_EOS    (0xff)  /* end of table.  actually never used */

static const char gPSMStates[94]= {
/* _PS_SEEK */ ' '            ,0                ,_PS_SEEK, /*space is token separator                        */
/* _PS_SEEK */ '\t'           ,0                ,_PS_SEEK, /*tab is also token sep                           */
/* _PS_SEEK */ '\n'           ,0                ,_PS_SEEK, /*newline is token sep                            */
/* _PS_SEEK */ ';'            ,0                ,_PS_SEEK, /*; don't bother with termi if no tokn started    */
/* _PS_SEEK */  XELP_CLI_ESC  ,0                ,_PS_ESCA, /*enter CLI escape mode                           */
/* _PS_SEEK */ '#'            ,0                ,_PS_CMNT, /*enter single line comment                       */
/* _PS_SEEK */ '\"'           ,_EF_TS           ,_PS_QUOT, /*enter quoted string token                       */
/* _PS_SEEK */  0             ,_EF_TS           ,_PS_TOK0, /*default .. enter token                          */
/* _PS_ESCA */  0             ,0                ,_PS_PREV, /*any char returns from esc state to pre stte     */
/* _PS_TOK0 */ ' '            ,_EF_TE           ,_PS_SEOL, /*end of 1st token                                */
/* _PS_TOK0 */ '\t'           ,_EF_TE           ,_PS_SEOL, /*end of 1st token                                */
/* _PS_TOK0 */ '#'            ,_EF_TE | _EF_LE  ,_PS_CMNT, /*end of line due to commnt, aslo end of token    */
/* _PS_TOK0 */ ';'            ,_EF_TE | _EF_LE  ,_PS_SEEK, /*end of tok, terminator end of line              */
/* _PS_TOK0 */ '\n'           ,_EF_TE | _EF_LE  ,_PS_SEEK, /*end of line, end of line                        */
/* _PS_TOK0 */  0             ,0                ,_PS_TOK0, /*keep adding to token                            */
/* _PS_CMNT */ '\n'           ,0                ,_PS_SEEK, /*end of line terminates comment                  */
/* _PS_CMNT */  0             ,0                ,_PS_CMNT, /*keep eating chars until eol reached             */
/* _PS_SEOL */ ';'            ,_EF_LE           ,_PS_SEEK, /*end of statement reached                        */
/* _PS_SEOL */ '\n'           ,_EF_LE           ,_PS_SEEK, /*end of line reached                             */
/* _PS_SEOL */ '#'            ,_EF_LE           ,_PS_CMNT, /*comment start                                   */
/* _PS_SEOL */  XELP_CLI_ESC  ,0                ,_PS_ESCA, /*esc char -- skip next char                      */
/* _PS_SEOL */ '\"'           ,0                ,_PS_QUOT, /*enter quoted str (uses diff esc, exit states)   */
/* _PS_SEOL */  0             ,0                ,_PS_SEOL, /*keep seeking EOL                                */
/* _PS_QUOT */ '\"'           ,0                ,_PS_QEND, /*hit end of quote, go to QEND to advnce 1 char   */
/* _PS_QUOT */  XELP_QUO_ESC  ,0                ,_PS_QESC, /*handle esc inside quoted str                    */
/* _PS_QUOT */  0             ,0                ,_PS_QUOT, /*keep going thru quoted string                   */
/* _PS_QESC */  0             ,0                ,_PS_QUOT, /*skip over next char (esc'd)                     */
/* _PS_QEND */ '#'            ,_EF_TE | _EF_LE  ,_PS_CMNT, /*exit quote in to comment                        */
/* _PS_QEND */ ';'            ,_EF_TE | _EF_LE  ,_PS_SEEK, /*exit quote with terminal                        */
/* _PS_QEND */ '\n'           ,_EF_TE | _EF_LE  ,_PS_SEEK, /*exit quote at end of line                       */
/* _PS_QEND */  0             ,_EF_TE           ,_PS_SEOL, /*exit quote                                      */
  (char)              _PS_EOS
};

static unsigned char const gPSMJumpTable[8]= {
 0,/* _PS_SEEK */
 24,/* _PS_ESCA */
 27,/* _PS_TOK0 */
 45,/* _PS_CMNT */
 51,/* _PS_SEOL */
 69,/* _PS_QUOT */
 78,/* _PS_QESC */
 81 /* _PS_QEND */
};

XELPRESULT XELPTokLine (const char *bs, const char *be, const char **t0s, const char **t0e, const char **eol, int srchType) {
 	const char *s;		 /* state ptr */
	char cs=_PS_SEEK,prev=_PS_SEEK,tmp;   
	int tm=1; /*  (token mode) allows capture of t0e, t0s only for first token seen */

	while (bs<be) {
		s = gPSMStates+(int)(gPSMJumpTable[(unsigned int)cs]);//index in to state array quickly
		/* while (*s != _PS_EOS) { //technically can be while(1) since each state _MUST_ have a default */
		while (1) { 
			if ( ( 0 == *s) || (*bs == (*s)) )// default in this state or char is match
				break;	
			s+=3; //goto next iteration in this state.  
		}	/* now we've found the correct state.  do any actions */

		s++; /* advance ptr to exec flags byte */
		/* if (*s)		// if there are any exec flags.. technically not needed but it can speed things up */
		{ 
			if (tm) {
				if ((*s) & _EF_TS) { *t0s =  bs; };
				if ((*s) & _EF_TE) { *t0e =  bs;  if (XELP_TOK_ONLY == srchType) return XELP_S_OK; tm=0;};
			}
			if ((*s) & _EF_LE) { *eol  = bs; return XELP_S_OK;};
		}
		s++; /* advance ptr to next_state byte */
		tmp = cs;
		cs = (*s == _PS_PREV) ? prev : (*s);
		prev = tmp; 
		/* end of parser state update */

		bs++; /* advance char ptr */
	}
	return XELP_S_NOTFOUND;
}
/*
XELPRESULT XELPTokLineX ( char *bs,  char *be, const char **t0s, const char **t0e, const char **eol, int srchType) {
    XelpBuf b,tok;
    XELPRESULT r;
    XELP_XBInitPtrs(b,bs,bs,be);

    r=XELPTokLineXB(&b,&tok,srchType);
    *t0s = tok.s;
    *t0e = tok.p;
    *eol = tok.e;
    return r;
}
*/
/********************************************************
  XELPTokLineXB(buf, output, srch) - main tokenizer - handles whitespaces, linefeeds, comments, quoted strings

  if srchType == XELP_TOK_ONLY ==> looks for next token starting from position buf->p.  
  if srchType == XELP_TOK_LINE ==> looks for entire line with tok(s,p,e)  returning (tok0 start, tok0 end, end of line) 

 */
XELPRESULT XELPTokLineXB (XelpBuf *buf, XelpBuf *tok, int srchType) {
 	const char *s;		 /*parser state ptr */
	char cs=_PS_SEEK,prev=_PS_SEEK,tmp;   
	int tm=1; /*  (token mode) allows capture of t0e, t0s only for first token seen */

	while ((buf->p) < (buf->e)) {
		s = gPSMStates+(int)(gPSMJumpTable[(unsigned int)cs]);//index in to state array quickly
		/* while (*(buf->p) != _PS_EOS) { //technically can be while(1) since each state _MUST_ have a default */
		while (1) { 
			if ( ( 0 == *s) || (*(buf->p) == (*s)) )// default in this state or char is match
				break;	
			s+=3; //goto next iteration in this state.  
		}	/* now we've found the correct state.  do any actions */

		s++; /* advance ptr to exec flags byte */
		/* if (*s)		// if there are any exec flags.. technically not needed but it can speed things up */
		{ 
			if (tm) {
				if ((*s) & _EF_TS) { tok->s =  (buf->p); };
				if ((*s) & _EF_TE) { tok->p =  (buf->p);  
                    if (XELP_TOK_ONLY == srchType)  { tok->e= buf->e; return XELP_S_OK;} 
                    tm=0;
                };
			}
			if ((*s) & _EF_LE) { tok->e  = (buf->p); return XELP_S_OK;};
		}
		s++; /* advance ptr to next_state byte */
		tmp = cs;
		cs = (*s == _PS_PREV) ? prev : (*s);
		prev = tmp; 
		/* end of parser state update */

		(buf->p)++; /* advance char ptr */
	}
	return XELP_S_NOTFOUND;
}

XELPRESULT XELPParseXB (XELP* ths, XelpBuf *args) {
	XelpBuf line;
	XELPCLIFuncMapEntry   *f;

	while (XELP_S_OK ==  XELPTokLineXB(args,&line,XELP_TOK_LINE) ) {
        f=ths->mpCLIModeFuncs;
        while(f->mpCmd) {    
            if (XELP_S_OK == XELPStrEq(line.s,(int)(line.p-line.s),f->mpCmd)){
                ths->mR[0] = (f->mFunPtr)(line.s,(int)(line.e-line.s));	break;
            }
            f++;
        }
	}
	return XELP_S_OK;
}
#endif /* XELP_ENABLE_CLI */

/**
	XELPParseKey() 
	live command line handling. 
	first looks for mode switch commans (single-key --> cli ---> thru)
	then if in single-key mode looks up single key.
	then if in command mode looks for <ENTER> and the attempts to parse current buffer.

*/
XELPRESULT XELPParseKey (XELP *ths, char key)
{
	int i=ths->mCurMode;
    XelpBuf ax; 
	/* 	first we test to see if we should switch modes.  this is a "key" difference  btw 
	just submitting a buffer to be parsed and running at command line
	*/
	switch (key) {
#ifdef XELP_ENABLE_CLI
		case XELPKEY_CLI:
			ths->mCurMode = (ths->mpCLIModeFuncs) ? XELP_MODE_CLI : i;
			break;
#endif
#ifdef XELP_ENABLE_KEY
		case XELPKEY_KEY:
			ths->mCurMode = (ths->mpKeyModeFuncs) ? XELP_MODE_KEY : i;
			break;
#endif
#ifdef XELP_ENABLE_THR			
		case XELPKEY_THR:
			ths->mCurMode = (ths->mpfPassThru)	  ? XELP_MODE_THR : i;
			break;
#endif /* XELP_ENABLE_THR */
		default:
			break;
	}
	
	if (ths->mCurMode != i) {
		if (ths->mpfEditModeChg)
			ths->mpfEditModeChg(ths->mCurMode);
	}
	else {
		switch(ths->mCurMode) {
			case XELP_MODE_KEY:
#ifdef XELP_ENABLE_KEY
				XELPExecKC(ths,key);
#endif
				break;
			case XELP_MODE_THR:
#ifdef XELP_ENABLE_THR				
				if (ths->mpfPassThru)
					ths->mpfPassThru(key);
#endif
				break;
			default: /* XELP_MODE_CLI */
#ifdef XELP_ENABLE_CLI
				if ((key == XELPKEY_BKSP) && (ths->mCmdMsgIndex )) {
					ths->mCmdMsgIndex --;
					if(ths->mpfBksp)
						ths->mpfBksp();
				}
				else {
					_PUTC(key);
					if (key == XELPKEY_ENTER )	{
						ths->mCmdMsgBuf[ths->mCmdMsgIndex]=';'; /*';' is used by the parser as statement terminator*/
						ths->mCmdMsgIndex++;
						//XELPParse(ths,ths->mCmdMsgBuf,ths->mCmdMsgIndex);
                        
                        XELP_XBInit(ax,ths->mCmdMsgBuf,ths->mCmdMsgIndex);
                        XELPParseXB(ths,&ax);
                        ths->mCmdMsgIndex=0; /* reset after command attempt */
#ifdef XELP_CLI_PROMPT
						XELPOut(ths,XELP_CLI_PROMPT,-1);
#endif
					}
					else {
						if (ths->mCmdMsgIndex<(XELP_CMDBUFSZ-2) ) 	{ /* make sure still be able to press ENTER even if buf full */
							ths->mCmdMsgBuf[ths->mCmdMsgIndex]=key;
							ths->mCmdMsgIndex++;
						}
					}
				}
#endif
				break;
		}
	}
	
	return XELP_S_OK;
}
/**
  XELPSStr2Int()
  parse a string return an integer
  345  --> read as decimal
  345h --> read as hex
 */
#define FR_SMUL10(x)	(((x)<<3)+(((x)<<1)))  /* many micros don't have multiply in core inst set */

int XELPStr2Int (const char* s,int  maxlen) {
	const char *p = s+maxlen-1;
	int r=0,x=0,d;

	if ('h' == *p) { /* hexadecimal */
		while (s<p) {
			d = (*s > '9') ? (*s-'a'+0xa) : (*s-'0');
			r = (r<<4)|d;
			s++;
		}
	}
	else { /* base 10 */
		if (*s == '-') {
			x = -1;
			s++;
		} else {
			if (*s == '+') {
				s++;
			}
		}
		while (s<=p) {
			d = *s - '0';
			r = FR_SMUL10(r) + d;
			s++;
		}
		r = x ? -r : r;
	}
	return r;
}
/*
XELPRESULT XelpStackOp (int opcode) {

	switch (opcode) {
        case XELP_STACK_NOP:
            return XELP_E_Err;
            break;
        case XELP_STACK_PUSH:
            return XELP_E_Err;
            break;
        case XELP_STACK_POP:
            return XELP_E_Err;
            break;
        case XELP_STACK_XOR:
            break;
        case XELP_STACK_NOT:
        case XELP_STACK_ADD:
        case XELP_STACK_INC:
        case XELP_STACK_DEC:
        case XELP_STACK_SUB:
        case XELP_STACK_MUL:
        case XELP_STACK_AND:
        case XELP_STACK_OR :
        default:
            return XELP_W_Warn;
	}
    
	return XELP_S_OK;
}
*/
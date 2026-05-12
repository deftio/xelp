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
/* static version kept separate from public XelpPutc -- the compiler can
   inline this at each call site, saving ~44 bytes vs routing through the
   non-static XelpPutc which must emit a callable symbol. */
static void _xelp_putc(XELP *ths, char c) {
    if (ths->mOutEnable && ths->mpfOut) ths->mpfOut(c);
}
#define _PUTC(c)    _xelp_putc(ths, (c))
#define _XOUTC(x,c) _xelp_putc((x), (c))

#ifdef XELP_ENABLE_CLI
static void _xelp_echo(XELP *ths, char c) {
    if (!ths->mOutEnable || !ths->mpfOut) return;
    if (ths->mEchoChar == XELP_ECHO_NORMAL)   ths->mpfOut(c);
    else if (ths->mEchoChar != XELP_ECHO_OFF) ths->mpfOut(ths->mEchoChar);
}
#define _ECHO(c)    _xelp_echo(ths, (c))
#endif

/* Enter key test for interactive mode — respects XELP_ENTER_CR / XELP_ENTER_LF
   from xelpcfg.h.  Only used in XelpParseKey; script parsing is unaffected. */
#if defined(XELP_ENTER_CR) && defined(XELP_ENTER_LF)
#define _XELP_IS_ENTER(ch) ((ch) == '\n' || (ch) == '\r')
#elif defined(XELP_ENTER_CR)
#define _XELP_IS_ENTER(ch) ((ch) == '\r')
#else
#define _XELP_IS_ENTER(ch) ((ch) == '\n')
#endif

#if defined(XELP_ENABLE_CLI) && defined(XELP_ENABLE_LINE_EDIT)
static void _xelp_cursor(XELP *ths, char c) {
    if (ths->mEchoChar != XELP_ECHO_OFF && ths->mOutEnable && ths->mpfOut)
        ths->mpfOut(c);
}
#define _CURSOR(c)  _xelp_cursor(ths, (c))
/*****************************************
 _xelp_memmove() - overlap-safe byte copy (avoids stdlib dependency for bare-metal).
 Copies bytes from [src .. src+n) to [dst .. dst+n).  Handles the overlapping-
 region case that arises when shifting characters within the CLI buffer
 (insert → shift right, delete → shift left).
 dst, src may overlap.  n==0 is a no-op.
 */
static void _xelp_memmove(char *dst, const char *src, int n) {
    if (dst < src) {                   /* shift-left: copy forward  */
        const char *e = src + n;
        while (src < e) *dst++ = *src++;
    } else if (dst > src) {            /* shift-right: copy backward */
        const char *e = src;
        dst += n; src += n;
        while (src > e) *--dst = *--src;
    }
}
#endif

/*****************************************
 _xelpKeyAccum() - key-input accumulator state machine.

 This is separate from the main CLI/script parser (the PSM tokenizer in
 gPSMStates).  The two state machines handle different layers:

   _xelpKeyAccum   → byte layer: assembles raw bytes into keycodes
   gPSMStates/PSM  → token layer: splits a text buffer into commands

 _xelpKeyAccum runs inside XelpParseKey() which feeds one hardware byte
 at a time.  It packs bytes into a XELPKEYCODE (unsigned long, little-endian):

   byte[0] in bits  0-7   (always present)
   byte[1] in bits  8-15  (ESC [ ... sequences)
   byte[2] in bits 16-23
   byte[3] in bits 24-31

 Single chars complete immediately (keycode == the char value).
 ESC is held: if the next byte is '[' we enter CSI mode and collect
 until a terminator (letter or '~'); if the next byte is anything else,
 ESC is flushed as a standalone keycode and the current byte is flagged
 for reprocessing (*reprocess=1).

 The PSM tokenizer, by contrast, works on whole buffers submitted via
 XelpParse/XelpParseXB and never sees raw hardware bytes.

 Returns: 1 = complete key in ths->mKeyAccum, 0 = need more bytes.
 */
static int _xelpKeyAccum(XELP *ths, char byte, int *reprocess) {
    unsigned char ub = (unsigned char)byte;
    *reprocess = 0;

    /* clear accumulator at start of a new keycode */
    if (ths->mKeyLen == 0) ths->mKeyAccum = 0;

    /* pack byte into accumulator at current position */
    ths->mKeyAccum |= ((XELPKEYCODE)ub) << (ths->mKeyLen * 8);
    ths->mKeyLen++;

    switch (ths->mKeyLen) {
    case 1:  /* first byte */
        if (ub == 0x1B) return 0;              /* ESC: wait for next */
        break;                                 /* ordinary char: done */

    case 2:  /* got ESC, peeking */
        if (ub == '[') return 0;               /* CSI intro: keep going */
        ths->mKeyAccum = 0x1B;                 /* flush bare ESC */
        *reprocess = 1;
        break;

    case 3:  /* ESC [ x */
        if (ub >= 0x40 && ub <= 0x7E) break;  /* letter terminator: done */
        if (ub >= '0' && ub <= '9') return 0;  /* digit param: keep going */
        break;                                 /* anything else: flush */

    case 4:  /* ESC [ digit ~ (or overflow) */
        break;
    }

    ths->mKeyLen = 0;
    return 1;
}

#if defined(XELP_ENABLE_CLI) && defined(XELP_ENABLE_LINE_EDIT)
/*****************************************
 _xelpRedrawFromCursor() - reprint from cursor to end, erase trailing, reposition
 */
static void _xelpRedrawFromCursor(XELP *ths) {
    char *p;
    int tail;
    if (!ths->mOutEnable || !ths->mpfOut) return;
    if (ths->mEchoChar == XELP_ECHO_OFF) return;
    /* print from cursor to end of content */
    for (p = ths->mCur; p < ths->mCmdXB.p; p++) {
        if (ths->mEchoChar != XELP_ECHO_NORMAL) ths->mpfOut(ths->mEchoChar);
        else ths->mpfOut(*p);
    }
    /* erase one trailing char (covers deletion case) */
    ths->mpfOut(' ');
    /* backspace to cursor position */
    tail = (int)(ths->mCmdXB.p - ths->mCur) + 1;
    while (tail-- > 0)
        ths->mpfOut('\b');
}
#endif

#if defined(XELP_ENABLE_CLI) && defined(XELP_ENABLE_LINE_EDIT) && defined(XELP_ENABLE_HISTORY)

/*****************************************
 _xelpHistReplaceLine() - clear displayed line and load new content.
 */
static void _xelpHistReplaceLine(XELP *ths, const char *src, int slen) {
    int oldlen = (int)(ths->mCmdXB.p - ths->mCmdXB.s);
    int i;

    /* move cursor to beginning of line (visual) */
    while (ths->mCur > ths->mCmdXB.s) {
        ths->mCur--;
        _CURSOR('\b');
    }
    /* overwrite old content with spaces */
    for (i = 0; i < oldlen; i++) _CURSOR(' ');
    /* backspace back to start */
    for (i = 0; i < oldlen; i++) _CURSOR('\b');

    /* copy new content into command buffer */
    for (i = 0; i < slen && i < XELP_CMDBUFSZ - 1; i++)
        ths->mCmdMsgBuf[i] = src[i];
    ths->mCmdXB.p = ths->mCmdXB.s + slen;
    ths->mCur = ths->mCmdXB.p;

    /* echo new content */
    for (i = 0; i < slen; i++) _ECHO(src[i]);
}

/*****************************************
 _xelpHistSave() - save command to history ring.
 Called from _xelpHandleEnter before buffer reset.
 Skips empty commands and consecutive duplicates.
 */
static void _xelpHistSave(XELP *ths) {
    int len = (int)(ths->mCmdXB.p - ths->mCmdXB.s);
    int prev;
    ths->mHistBrowse = -1;  /* ENTER always ends browse session */
    if (len <= 0) return;

    /* skip consecutive duplicate */
    if (ths->mHistCount > 0) {
        prev = ((int)ths->mHistWrite - 1 + XELP_HIST_DEPTH) % XELP_HIST_DEPTH;
        if (XelpStrEq(ths->mCmdXB.s, len, ths->mHistBuf[prev]) == XELP_S_OK)
            return;
    }

    /* copy command into ring slot */
    {
        char *dst = ths->mHistBuf[(int)ths->mHistWrite];
        int i;
        for (i = 0; i < len && i < XELP_CMDBUFSZ - 1; i++)
            dst[i] = ths->mCmdXB.s[i];
        dst[i] = 0;
    }
    ths->mHistWrite = (char)(((int)ths->mHistWrite + 1) % XELP_HIST_DEPTH);
    if (ths->mHistCount < XELP_HIST_DEPTH)
        ths->mHistCount++;
}

/*****************************************
 _xelpHistRecall() - handle UP/DOWN arrow for history browsing.
 dir: -1 = UP (older), +1 = DOWN (newer)
 */
static void _xelpHistRecall(XELP *ths, int dir) {
    if (dir < 0) {
        /* UP: go to older entry */
        if (ths->mHistCount == 0) return;

        if (ths->mHistBrowse == -1) {
            /* first UP: save in-progress line */
            int len = (int)(ths->mCmdXB.p - ths->mCmdXB.s);
            int i;
            for (i = 0; i < len; i++)
                ths->mHistSaved[i] = ths->mCmdXB.s[i];
            ths->mHistSaved[len] = 0;
            ths->mHistSavedLen = (char)len;
            /* start at most recent entry */
            ths->mHistBrowse = ths->mHistCount - 1;
        } else if (ths->mHistBrowse > 0) {
            ths->mHistBrowse--;
        } else {
            return; /* already at oldest */
        }
    } else {
        /* DOWN: go to newer entry */
        if (ths->mHistBrowse == -1) return; /* not browsing */

        if (ths->mHistBrowse < ths->mHistCount - 1) {
            ths->mHistBrowse++;
        } else {
            /* past newest: restore in-progress line */
            ths->mHistBrowse = -1;
            _xelpHistReplaceLine(ths, ths->mHistSaved, (int)ths->mHistSavedLen);
            return;
        }
    }

    /* load the entry at mHistBrowse (0=oldest, count-1=newest) */
    {
        int slot = ((int)ths->mHistWrite - (int)ths->mHistCount + (int)ths->mHistBrowse + XELP_HIST_DEPTH) % XELP_HIST_DEPTH;
        _xelpHistReplaceLine(ths, ths->mHistBuf[slot], XelpStrLen(ths->mHistBuf[slot]));
    }
}
#endif /* XELP_ENABLE_CLI && XELP_ENABLE_LINE_EDIT && XELP_ENABLE_HISTORY */

#if defined(XELP_ENABLE_HELP) && defined(XELP_ENABLE_KEY)
/*****************************************
 _xelpPrintKeyName() - print human-readable name for a keycode in help output
 */
static void _xelpPrintKeyName(XELP *ths, XELPKEYCODE key) {
    static const XELPKEYCODE codes[] = {
        XELP_KEYCODE_UP,  XELP_KEYCODE_DOWN, XELP_KEYCODE_LEFT, XELP_KEYCODE_RIGHT,
        XELP_KEYCODE_HOME,XELP_KEYCODE_END,  XELP_KEYCODE_KDEL, XELP_KEYCODE_INS,
        XELP_KEYCODE_PGUP,XELP_KEYCODE_PGDN };
    static const char names[] = "UpDnLtRtHmEnDlInPUPD";
    static const char hex[]   = "0123456789ABCDEF";
    int i;
    if (!XELP_KC_IS_MULTI(key)) { _XOUTC(ths, (char)key); return; }
    for (i = (int)(sizeof(codes)/sizeof(codes[0])); i--;)
        if (key == codes[i]) { _XOUTC(ths, names[i*2]); _XOUTC(ths, names[i*2+1]); return; }
    _XOUTC(ths, '0'); _XOUTC(ths, 'x');
    for (i = 28; i >= 0; i -= 4) {
        char nib = (char)((key >> i) & 0xF);
        if (nib || i < 8) _XOUTC(ths, hex[(int)nib]);
    }
}
#endif

/*****************************************
 XelpStrLen() - find length of a string in bytes assuming its null terminated
 */
int XelpStrLen (const char* c) {
    int l=0;
    while (*c++ != 0) {
        l++;
    }
    return l;
}

/***************************************** 
 XelpOut() - print a string.
 takes a length specified string and prints to output stream
 */
XELPRESULT XelpOut (XELP *ths, const char* msg, int maxlen)
{
	if (!ths->mOutEnable) return XELP_S_OK;
	if ((0 != msg) && (0 !=ths->mpfOut)) {
		while (*msg != 0) {
			(ths->mpfOut)(*msg++);
			if ((maxlen > 0) && (--maxlen == 0)) break;
		}
	}
	return XELP_S_OK;
}

XELPRESULT XelpPutc(XELP *ths, char c)
{
	_xelp_putc(ths, c);  /* reuse static version to avoid code duplication */
	return XELP_S_OK;
}
/******************************************
 XelpHelp() - print out help strings for functions
 see xelpcfg.h for setting or overriding
 XELP_HELP_ABT_STR, XELP_HELP_KEY_STR, XELP_HELP_CLI_STR
 */
#ifdef XELP_ENABLE_HELP
XELPRESULT XelpHelp(XELP* ths)
{
#ifdef XELP_ENABLE_KEY
	XELPKeyFuncMapEntry *e = ths->mpKeyModeFuncs;
#endif
#ifdef XELP_ENABLE_CLI
	XELPCLIFuncMapEntry *s = ths->mpCLIModeFuncs;
#endif
	const int x=0xff;

	XelpOut(ths,XELP_HELP_ABT_STR,x);
#ifdef XELP_ENABLE_KEY
	if (e) { //check and see if first entry is not terminator
		XelpOut(ths,XELP_HELP_KEY_STR,x);
		do 	{
			_xelpPrintKeyName(ths, e->mKey);
			_PUTC('\t');
			XelpOut(ths,e->mpHelpString,x);
			_PUTC('\n');
			e++;
		} while (e->mFunPtr);
	}
#endif
#ifdef XELP_ENABLE_CLI
	if (s) {
		XelpOut(ths,XELP_HELP_CLI_STR,x);
		do	{
			XelpOut(ths,s->mpCmd,x);
			_PUTC('\t');
			XelpOut(ths,s->mpHelpString,x);
			_PUTC('\n');	
			s++;	
		} while (s->mFunPtr);
	}
#endif
	return XELP_S_OK;
}
#endif
XELPRESULT XelpInit 	 (
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
		*p++=0;
	
	ths->mOutEnable = 1;
	ths->mpAboutMsg = pAboutMsg;
#ifdef XELP_ENABLE_CLI
	/* Guard needed: mCmdXB and mCmdMsgBuf only exist in the struct when
	   XELP_ENABLE_CLI is defined (see xelp.h).  Without this guard,
	   KEY-only builds (XELP_CONFIG_OVERRIDE with only XELP_ENABLE_KEY)
	   fail to compile. */
	XELP_XB_INIT(ths->mCmdXB,ths->mCmdMsgBuf,XELP_CMDBUFSZ-1);
#endif
#if defined(XELP_ENABLE_CLI) && defined(XELP_ENABLE_LINE_EDIT)
	ths->mCur = ths->mCmdXB.s;
#endif
#if defined(XELP_ENABLE_CLI) && defined(XELP_ENABLE_HISTORY)
	ths->mHistBrowse = -1;
#endif
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
/********************************************************
 XelpStrEq2 (pbuf, pend, cmd)
  Compare a pointer-pair buffer [pbuf..pend) to a null-terminated string cmd.
  This is the primary comparison function; XelpStrEq is a thin wrapper.
 */
#ifdef XELP_ENABLE_CLI
XELPRESULT XelpStrEq2 (const char* pbuf, const char* pend, const char *cmd)
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
/****************************
 XelpStrEq() : length-based wrapper around XelpStrEq2.
 cmd is assumed to be 0 terminated e.g. "mycommand" === mycommand\0
*/
XELPRESULT XelpStrEq (const char* pbuf, int blen, const char *cmd)
{
	return XelpStrEq2(pbuf, pbuf + blen, cmd);
}
/********************************************************
 XelpBufCmp() : test if 2 buffers have byte for byte equality.  Used for finding if tokens match commands or labels
 The buffers are specified by their start and end pointers [as .. ae] and [bs .. be]
 cmpType: (comparison type)
 XELP_CMP_TYPE_BUF : both buffers are only tested for byte for byte comparison by length (\0 is not treated as end-of-buf)
 XELP_CMP_TYPE_A0  : buffer as..ae also treats \0 as a end of buffer 
 XELP_CMP_TYPE_A0B0  : if either buffer has \0 that is treated as the end of the buffer in addition to reaching the end ptr

 returns XELP_S_OK if they are equal else XELP_S_NOTFOUND 
 
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
 XelpFindTok() : find a matching token in a buffer.  The buffer is a XelpBuf, the token is passed as 
    ptr to its beginning and a ptr to one position beyond its end 
 
 srchType: 
 XELP_TOK_ONLY  : find the next token (if any) that matches the supplied token
 XELP_TOK_LINE  : find the next token that matches the supplied token, but only if its the first token of the line.
 if successful returns XELP_S_OK and x->p points to the position _just_after_ the found token.
 else returns XELP_S_NOTFOUND and (x->p) == (x->e)

*/

XELPRESULT XelpFindTok(XelpBuf *x, const char *t0s, const char *t0e, int srchType) 
{
    XelpBuf tok;
    
    while (XELP_S_OK == XelpTokLineXB(x,&tok,srchType)) {
        if (XELP_S_OK == XelpBufCmp(t0s,t0e,tok.s,tok.p,XELP_CMP_TYPE_BUF))
            return XELP_S_OK;
    }
    return XELP_S_NOTFOUND;
}
#endif
/********************************************************
XELPRESULT XelpExecKC(char)  : (execute key-command)

Attempts to execute first matching single-key command.  
the key value is passed to the command as an int

This is used in KEY (menu driven) mode or can be called from any C function.  

e.g.
XelpExecKC(myInstance,'a');  // execute the single key command 'a' if it exists

*/
#ifdef XELP_ENABLE_KEY
XELPRESULT XelpExecKC(XELP* ths, XELPKEYCODE key) {
	XELPKeyFuncMapEntry *p = ths->mpKeyModeFuncs;
    if (p)
    {
        while (p->mFunPtr) {
            if (p->mKey == key)			{
                ths->mR[0] = p->mFunPtr(ths, key);
                return ths->mR[0];
            }
            p++;
        }
    }
    if (ths->mpfDefKey) {
        ths->mR[0] = ths->mpfDefKey(ths, key);
    } else {
        ths->mR[0] = XELP_S_NOTFOUND;
    }
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
#define _PS_TOK0   (0x02)  /* token0 (the command / opterator)*/
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
#ifdef XTOKLINE_OLD
XELPRESULT XelpTokLine (const char *bs, const char *be, const char **t0s, const char **t0e, const char **eol, int srchType) {
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
#endif
/*
XELPRESULT XelpTokLine(char* bs, char* be, const char **t0s, const char **t0e, const char **eol, int srchType) {
    XelpBuf xc,tok;
    XELPRESULT r;

    XELP_XB_INIT_PTRS(xc,bs,bs,be);

    r=XelpTokLineXB(&xc,&tok,srchType);
    *t0s = tok.s;
    *t0e = tok.p;
    *eol = tok.e;
    return r;
}
*/

/********************************************************
  XelpTokLineXB(buf, output, srch) - main tokenizer - handles whitespaces, linefeeds, comments, quoted strings

  if srchType == XELP_TOK_ONLY ==> looks for next token starting from position buf->p.  
  if srchType == XELP_TOK_LINE ==> looks for entire line with tok(s,p,e)  returning (tok0 start, tok0 end, end of line) 

 */
XELPRESULT XelpTokLineXB (XelpBuf *buf, XelpBuf *tok, int srchType) {
 	const char *s;		 /*parser state ptr */
	char cs=_PS_SEEK,prev=_PS_SEEK,tmp;   
	int tm=1; /*  (token mode) allows capture of t0e, t0s only for first token seen */

    if ((buf->p) >= (buf->e)) {  return XELP_S_NOTFOUND; }


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
    /* Buffer exhausted before a token was completed.  If tok->s was never
       assigned (_EF_TS never fired) we must return NOTFOUND — otherwise the
       caller would use the uninitialised tok->s pointer, causing a SEGV in
       XelpStrEq during command dispatch.  States where _EF_TS has not fired:
         _PS_SEEK  – still looking for a token
         _PS_CMNT  – inside a comment (no token started)
         _PS_ESCA  – consumed a CLI escape char at end-of-buffer
       Bug found by libFuzzer: input " ` \n" (backtick = XELP_CLI_ESC) left
       the tokeniser in _PS_ESCA at EOB, returning OK with garbage tok->s. */
    if (tm && (cs == _PS_SEEK || cs == _PS_CMNT || cs == _PS_ESCA))
        return XELP_S_NOTFOUND;
    if (tm)
        tok->p  = buf->p;
    tok->e  = buf->p;

    return XELP_S_OK;
    
}

/********************************************************
 XelpParseXB() parse buffer and execute commands 
 */

XELPRESULT XelpParseXB (XELP* ths, XelpBuf *args) {
	XelpBuf line;
	XELPCLIFuncMapEntry   *f;

	while (XELP_S_OK ==  XelpTokLineXB(args,&line,XELP_TOK_LINE) ) { /* for each logical line */
        
        f=ths->mpCLIModeFuncs;
        if (f) { /* make sure fn dispatch table exists */
        	ths->mR[0] = XELP_E_CMDNOTFOUND;
            while(f->mpCmd) {    
                if (XELP_S_OK == XelpStrEq2(line.s,line.p,f->mpCmd)){
                    
                    ths->mR[0] = (f->mFunPtr)(ths, line.s,(int)(line.e-line.s));
                    break;
                }
                f++;
            }
            if (ths->mR[0] == XELP_E_CMDNOTFOUND) {
            	if (ths->mpfDefCLI)
            		ths->mR[0] = ths->mpfDefCLI(ths, line.s,(int)(line.e-line.s));
            }
        }
	}
	return XELP_S_OK;
}
XELPRESULT XelpParse 		(XELP *ths, const char *buf, int blen)
{
    XelpBuf args;
    XELP_XB_INIT(args,(char*)buf,blen); /* const discard is safe: tokenizer only reads */
    return XelpParseXB(ths,&args);
}
/********************************************************
 XelpTokN() find the nth token (if it exists) - useful for parsing arguments

 XelpTokN finds the nth token (starting from the current buffer position buf->p);
 note: tok has last successfully found token regardless of result (check return value == XELP_S_OK)
 buf.p is pointing to position just after nth token.

 */
XELPRESULT XelpTokN (XelpBuf *buf, int n, XelpBuf *tok)
{
    XELPRESULT r;
    buf->p = buf->s;
    do {
        r = XelpTokLineXB(buf,tok,XELP_TOK_ONLY);
        if (XELP_S_OK != r) {
            tok->p = tok->s;
            tok->e = tok->s;
            break;
        }
    }while (n--);
    
    return r;
}

/********************************************************
 XelpNumToks() find the number of tokens in a buffer.

 */

XELPRESULT XelpNumToks (XelpBuf *b, int *n)
{
    XelpBuf t;
    *n=0;
    while (XELP_S_OK == XelpTokLineXB(b,&t,XELP_TOK_ONLY))
        (*n)++;

    return XELP_S_OK;
}

/********************************************************
 XelpArgs -- sequential argument iterator.
 See xelp.h for API documentation.
 */

XELPRESULT XelpArgsInit (XelpArgs *a, const char *args, int len)
{
    XELP_XB_INIT(a->buf, (char*)args, len);
    return XELP_S_OK;
}

XELPRESULT XelpNextTok (XelpArgs *a, XelpBuf *tok)
{
    XelpBuf t;
    XELPRESULT r = XelpTokLineXB(&a->buf, &t, XELP_TOK_ONLY);
    if (r != XELP_S_OK) {
        if (tok) { tok->s = 0; tok->p = 0; }
        return r;
    }
    if (tok) *tok = t;
    return XELP_S_OK;
}

XELPRESULT XelpNextInt (XelpArgs *a, int *val)
{
    XelpBuf tok;
    XELPRESULT r = XelpNextTok(a, &tok);
    if (r != XELP_S_OK) return r;
    return XelpParseNum(tok.s, (int)(tok.p - tok.s), val);
}

XELPRESULT XelpArgCount (XelpArgs *a, int *n)
{
    XelpBuf save;
    XELP_XB_COPY(a->buf, save);
    XELP_XB_TOP(a->buf);
    XelpNumToks(&a->buf, n);
    XELP_XB_COPY(save, a->buf);
    return XELP_S_OK;
}

/********************************************************
 XelpArgInt() - get the Nth argument as an integer (random access).
 Arg 0 is the command name. Returns XELP_E_ERR if arg N doesn't exist
 or is not a valid number.
 */
XELPRESULT XelpArgInt (const char *args, int len, int n, int *val)
{
    XelpBuf b, tok;
    XELP_XB_INIT(b, (char*)args, len);
    if (XelpTokN(&b, n, &tok) != XELP_S_OK) return XELP_E_ERR;
    return XelpParseNum(tok.s, (int)(tok.p - tok.s), val);
}

/********************************************************
 XelpArgStr() - get the Nth argument as a string span (random access).
 Sets *s to start of token and *slen to its length.
 Token is NOT null-terminated (buffer is not modified).
 Returns XELP_E_ERR if arg N doesn't exist.
 */
XELPRESULT XelpArgStr (const char *args, int len, int n,
                       const char **s, int *slen)
{
    XelpBuf b, tok;
    XELP_XB_INIT(b, (char*)args, len);
    if (XelpTokN(&b, n, &tok) != XELP_S_OK) return XELP_E_ERR;
    *s = tok.s;
    *slen = (int)(tok.p - tok.s);
    return XELP_S_OK;
}

#endif /* XELP_ENABLE_CLI */

#ifdef XELP_ENABLE_ARGV
/********************************************************
 XelpBuf2Argv() - tokenize args into argc/argv using mArgvBuf as scratch.
 Strips quotes, processes escape sequences, null-terminates each token.
 argv[0] = command name per argc/argv convention.
 Returns XELP_E_ERR if input exceeds scratch buffer or too many args.
 */
XELPRESULT XelpBuf2Argv(XELP *ths, const char *args, int len,
                         int *argc, char **argv, int maxargs)
{
    char *r, *w, *end, c;
    int ac = 0, i, q;

    *argc = 0;
    if (len <= 0) return XELP_S_OK;
    if (len >= XELP_CMDBUFSZ) return XELP_E_ERR;

    r = ths->mArgvBuf;
    for (i = 0; i < len; i++) r[i] = args[i];
    end = r + len;
    w = r;

    while (r < end) {
        while (r < end && (*r == ' ' || *r == '\t')) r++;
        if (r >= end || *r == ';' || *r == '\n' || *r == '#') break;
        if (ac >= maxargs) return XELP_E_ERR;
        argv[ac] = w;
        q = (*r == '"');
        if (q) r++;

        while (r < end) {
            c = *r;
            if (q) {
                if (c == '"')  { r++; break; }
                if (c == XELP_QUO_ESC && r + 1 < end) {
                    const char *m = XELP_ESC_MAP;
                    c = *++r;
                    while (*m) { if (c == m[0]) { c = m[1]; break; } m += 2; }
                }
            } else {
                if (c == ' ' || c == '\t' ||
                    c == ';' || c == '\n' || c == '#') break;
                if (c == XELP_CLI_ESC) { if (++r >= end) break; c = *r; }
            }
            *w++ = c;
            r++;
        }
        {   /* check stop char before null-write can overwrite it */
            int stop = !q && r < end &&
                       (*r == ';' || *r == '\n' || *r == '#');
            *w++ = '\0';
            ac++;
            if (stop) break;
        }
        if (w > r) r = w;
    }
    *argc = ac;
    return XELP_S_OK;
}

/********************************************************
 XelpArgvInt() - get argv[n] as an integer.
 Returns XELP_E_ERR if n is out of range or not a valid number.
 */
XELPRESULT XelpArgvInt(char **argv, int argc, int n, int *val)
{
    if (n < 0 || n >= argc) return XELP_E_ERR;
    return XelpParseNum(argv[n], XelpStrLen(argv[n]), val);
}

/********************************************************
 XelpArgvStr() - get argv[n] as a string pointer and length.
 Returns XELP_E_ERR if n is out of range.
 */
XELPRESULT XelpArgvStr(char **argv, int argc, int n, const char **s, int *slen)
{
    if (n < 0 || n >= argc) return XELP_E_ERR;
    *s = argv[n];
    *slen = XelpStrLen(argv[n]);
    return XELP_S_OK;
}
#endif /* XELP_ENABLE_ARGV */

/********************************************************
	XelpParseKey() 
	live command line handling. 
	first looks for mode switch commans (single-key --> cli ---> thru)
	then if in single-key mode looks up single key.
	then if in command mode looks for <ENTER> and the attempts to parse current buffer.

*/
#ifdef XELP_ENABLE_LINE_EDIT
/* move cursor left (dir<0) or right (dir>0); all=1 moves to boundary */
static void _xelpCursorMove(XELP *ths, int dir, int all) {
	do {
		if (dir < 0) {
			if (ths->mCur <= ths->mCmdXB.s) break;
			ths->mCur--;
			_CURSOR('\b');
		} else {
			if (ths->mCur >= ths->mCmdXB.p) break;
			_CURSOR(*ths->mCur);
			ths->mCur++;
		}
	} while (all);
}
#endif

#ifdef XELP_ENABLE_CLI
/* handle ENTER: echo newline, save to history, execute buffer, reset, show prompt */
static void _xelpHandleEnter(XELP *ths) {
	XelpBuf line;
#if defined(XELP_ENABLE_LINE_EDIT) && defined(XELP_ENABLE_HISTORY)
	_xelpHistSave(ths);
#endif
	_PUTC(XELPKEY_ENTER);
	XELP_XB_INIT_PTRS(line, ths->mCmdXB.s, ths->mCmdXB.s, ths->mCmdXB.p);
	XelpParseXB(ths, &line);
	XELP_XB_TOP(ths->mCmdXB);
#ifdef XELP_ENABLE_LINE_EDIT
	ths->mCur = ths->mCmdXB.s;
#endif
#ifdef XELP_CLI_PROMPT
	XelpOut(ths, XELP_CLI_PROMPT, -1);
#endif
}
#endif

XELPRESULT XelpParseKey (XELP *ths, char key)
{
	int reprocess;
	do {
		XELPKEYCODE keycode;
		int is_single;
		reprocess = 0;

		if (!_xelpKeyAccum(ths, key, &reprocess))
			return XELP_S_OK; /* incomplete sequence */

		keycode = ths->mKeyAccum;
		ths->mKeyLen = 0;
		is_single = !XELP_KC_IS_MULTI(keycode);

		/* mode-switch check (only for single-char keys) */
		if (is_single) {
			char ch = (char)keycode;
			int i = ths->mCurMode;
			int modeChangeAttempt = 1;

			switch (ch) {
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
					ths->mCurMode = (ths->mpfPassThru) ? XELP_MODE_THR : i;
					break;
#endif
				default:
					modeChangeAttempt = 0;
					break;
			}

			if ((ths->mCurMode != i) && (modeChangeAttempt)) {
				if (ths->mpfEditModeChg)
					ths->mpfEditModeChg(ths->mCurMode);
				continue; /* if reprocess is set, loop; else done */
			}
		}

		/* dispatch by current mode */
		switch(ths->mCurMode) {
			case XELP_MODE_KEY:
#ifdef XELP_ENABLE_KEY
				XelpExecKC(ths, keycode);
#endif
				break;
			case XELP_MODE_THR:
#ifdef XELP_ENABLE_THR
				if (is_single && ths->mpfPassThru)
					ths->mpfPassThru((char)keycode);
#endif
				break;
			default: /* XELP_MODE_CLI */
#ifdef XELP_ENABLE_CLI
			{
#ifdef XELP_ENABLE_LINE_EDIT
				/* --- Line editing enabled --- */
				if (!is_single) {
					/* multi-byte key in CLI mode */
					switch (keycode) {
						case XELP_KEYCODE_LEFT:  _xelpCursorMove(ths, -1, 0); break;
						case XELP_KEYCODE_RIGHT: _xelpCursorMove(ths, +1, 0); break;
						case XELP_KEYCODE_HOME:  _xelpCursorMove(ths, -1, 1); break;
						case XELP_KEYCODE_END:   _xelpCursorMove(ths, +1, 1); break;
						case XELP_KEYCODE_KDEL: {
							if (ths->mCur < ths->mCmdXB.p) {
								int tail = (int)(ths->mCmdXB.p - ths->mCur - 1);
								_xelp_memmove(ths->mCur, ths->mCur + 1, tail);
								ths->mCmdXB.p--;
								_xelpRedrawFromCursor(ths);
							}
							break;
						}
						case XELP_KEYCODE_UP:
#ifdef XELP_ENABLE_HISTORY
							_xelpHistRecall(ths, -1);
#endif
							break;
						case XELP_KEYCODE_DOWN:
#ifdef XELP_ENABLE_HISTORY
							_xelpHistRecall(ths, +1);
#endif
							break;
						default:
							/* silently drop other multi-byte keys */
							break;
					}
				} else {
					/* single-char key in CLI mode with line editing */
					char ch = (char)keycode;
					if (ch == XELPKEY_BKSP || ch == XELPKEY_BS || ch == XELPKEY_DEL) {
						/* delete char before cursor */
						if (ths->mCur > ths->mCmdXB.s) {
							int tail = (int)(ths->mCmdXB.p - ths->mCur);
							ths->mCur--;
							_xelp_memmove(ths->mCur, ths->mCur + 1, tail);
							ths->mCmdXB.p--;
							_CURSOR('\b');
							_xelpRedrawFromCursor(ths);
						}
					} else if (_XELP_IS_ENTER(ch)) {
						_xelpHandleEnter(ths);
					} else if (ch >= 0x20 && ch <= 0x7E) {
						/* printable character */
						if (ths->mCur == ths->mCmdXB.p) {
							/* append at end (fast path) */
							XELP_XB_PUTC(ths->mCmdXB, ch);
							if (ths->mCmdXB.p > ths->mCur) {
								ths->mCur++;
								_ECHO(ch);
							}
						} else {
							/* insert at cursor */
							if (ths->mCmdXB.p < ths->mCmdXB.e) {
								int tail = (int)(ths->mCmdXB.p - ths->mCur);
								_xelp_memmove(ths->mCur + 1, ths->mCur, tail);
								*ths->mCur = ch;
								ths->mCmdXB.p++;
								ths->mCur++;
								_ECHO(ch);
								_xelpRedrawFromCursor(ths);
							}
						}
					}
					/* other control chars silently dropped */
				}
#else
				/* --- Line editing NOT enabled (append-only, old behavior) --- */
				if (!is_single) {
					/* silently drop multi-byte keys */
				} else {
					char ch = (char)keycode;
					if (ch == XELPKEY_BKSP || ch == XELPKEY_BS) {
						if (ths->mCmdXB.p > ths->mCmdXB.s) {
							(ths->mCmdXB.p)--;
							if (ths->mpfBksp)
								ths->mpfBksp();
						}
					} else if (_XELP_IS_ENTER(ch)) {
						_xelpHandleEnter(ths);
					} else {
						_ECHO(ch);
						XELP_XB_PUTC(ths->mCmdXB,ch);
					}
				}
#endif /* XELP_ENABLE_LINE_EDIT */
			}
#endif /* XELP_ENABLE_CLI */
				break;
		}
	} while (reprocess);

	return XELP_S_OK;
}
#define XELP_MUL10(x)	(((x)<<3)+(((x)<<1)))  /* many old micros don't have multiply in core inst set */
#define XELP_INT_MAX    ((int)(((unsigned)-1) >> 1))  /* portable INT_MAX without <limits.h> */
/********************************************************
  XelpParseNum()
  parse a string, return an integer via *n.
  Returns XELP_S_OK on success, XELP_E_ERR on invalid input.
  345   --> decimal
  345h  --> hex (suffix)
  0x345 --> hex (prefix)
 */
XELPRESULT XelpParseNum (const char* s, int maxlen, int* n) {
	const char *end = s + maxlen; /* one past last byte -- never dereferenced */
	int r=0, neg=0, d, isHex=0;

	if (maxlen <= 0) return XELP_E_ERR;

	/* detect hex: 0x prefix takes priority over h suffix */
	if (maxlen >= 3 && s[0] == '0' && s[1] == 'x') {
		isHex = 1;
		s += 2;                    /* skip "0x", end stays */
	} else if (s[maxlen-1] == 'h') {
		isHex = 1;
		end--;                     /* exclude trailing 'h' */
	}

	if (isHex) {
		if (s >= end) return XELP_E_ERR; /* no hex digits */
		while (s < end) {
			if      (*s >= 'a' && *s <= 'f') d = *s - 'a' + 0xa;
			else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 0xa;
			else if (*s >= '0' && *s <= '9') d = *s - '0';
			else return XELP_E_ERR;
			if ((unsigned)r > ((unsigned)XELP_INT_MAX >> 4)) return XELP_E_ERR;
			r = (r << 4) | d;
			s++;
		}
	}
	else { /* base 10 */
		if      (*s == '-') { neg = 1; s++; }
		else if (*s == '+') {          s++; }
		if (s >= end) return XELP_E_ERR; /* sign only, no digits */
		while (s < end) {
			d = *s - '0';
			if (d < 0 || d > 9) return XELP_E_ERR;
			if (r > (XELP_INT_MAX - d) / 10) return XELP_E_ERR;
			r = XELP_MUL10(r) + d;
			s++;
		}
		if (neg) r = -r;
	}
	*n = r;

	return XELP_S_OK;
}
/********************************************************
  XelpStr2Int()
  Convenience wrapper: parse a string, return an integer directly.
  Calls XelpParseNum internally.  Returns 0 on invalid input.
 */
int XelpStr2Int (const char* s, int maxlen) {
	int n = 0;
	XelpParseNum(s, maxlen, &n);
	return n;
}

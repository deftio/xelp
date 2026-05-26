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
    } else {                           /* shift-right: copy backward */
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
    {
        char ec = ths->mEchoChar;
        for (p = ths->mCur; p < ths->mCmdXB.p; p++)
            ths->mpfOut(ec != XELP_ECHO_NORMAL ? ec : *p);
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
    {
        int back = (int)(ths->mCur - ths->mCmdXB.s);
        for (i = 0; i < back; i++) _CURSOR('\b');
        ths->mCur = ths->mCmdXB.s;
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
        const char *src = ths->mCmdXB.s;
        int n = (len < XELP_CMDBUFSZ - 1) ? len : XELP_CMDBUFSZ - 1;
        while (n-- > 0) *dst++ = *src++;
        *dst = 0;
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
            const char *src = ths->mCmdXB.s;
            char *dst = ths->mHistSaved;
            int n = len;
            while (n-- > 0) *dst++ = *src++;
            *dst = 0;
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
	if (e && e->mFunPtr) {
		XelpOut(ths,XELP_HELP_KEY_STR,x);
		while (e->mFunPtr) {
			_xelpPrintKeyName(ths, e->mKey);
			_PUTC('\t');
			XelpOut(ths,e->mpHelpString,x);
			_PUTC('\n');
			e++;
		}
	}
#endif
#ifdef XELP_ENABLE_CLI
	if (s && s->mFunPtr) {
		XelpOut(ths,XELP_HELP_CLI_STR,x);
		while (s->mFunPtr) {
			XelpOut(ths,s->mpCmd,x);
			_PUTC('\t');
			XelpOut(ths,s->mpHelpString,x);
			_PUTC('\n');
			s++;
		}
	}
#endif
	return XELP_S_OK;
}
#endif
#ifdef XELP_ENABLE_SCRIPT
static void _xelpArenaInit(XELP *ths);
static XELPRESULT _xelpEvalLoop(XELP *ths, int targetDepth);
static int _xelpStackEntrySize(const char *p);
/* Internal signals for iterative eval loop */
#define XELP_S_CALL   (3)  /* frame pushed by function call, continue loop */
#define XELP_S_RETURN (4)  /* _return executed, pop frame */
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
#ifdef XELP_ENABLE_SCRIPT
	_xelpArenaInit(ths);
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

        if (cmpType >= XELP_CMP_TYPE_A0) {
            if (*as == 0) ae = as+1;
            if (cmpType == XELP_CMP_TYPE_A0B0 && *bs == 0) be = bs+1;
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
		s = gPSMStates+(int)(gPSMJumpTable[(unsigned int)cs]); /* index in to state array quickly */
		/* while (*(buf->p) != _PS_EOS) { technically can be while(1) since each state _MUST_ have a default } */
		while (1) {
			if ( ( 0 == *s) || (*(buf->p) == (*s)) ) /* default in this state or char is match */
				break;
			s+=3; /* goto next iteration in this state */
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
 _xelpBuf2Argv() - tokenize args into argc/argv using mArgvBuf as scratch.
 Strips quotes, processes escape sequences, null-terminates each token.
 argv[0] = command name per argc/argv convention.
 Returns XELP_E_ERR if input exceeds scratch buffer or too many args.
 Reads directly from the source buffer (ROM-safe) and writes tokens into
 mArgvBuf — no upfront copy needed since escape expansion only shrinks.
 */
#ifndef XELP_ENABLE_SCRIPT
static XELPRESULT _xelpBuf2Argv(XELP *ths, const char *r, int len,
                         int *argc)
{
    const char *end = r + len;
    char *w = ths->mArgvBuf, c;
    int ac = 0;

    *argc = 0;
    if (len <= 0) return XELP_S_OK;
    if (len >= XELP_ARGVBUFSZ) return XELP_E_ERR;

    while (r < end) {
        while (r < end && (*r == ' ' || *r == '\t')) r++;
        if (r >= end) break;
        if (ac >= (int)XELP_ARGV_CAP) return XELP_E_ERR;
        ths->mArgv[ac++] = w;
        if (*r == '"') {
            r++;                                       /* skip open quote  */
            while (r < end && *r != '"') {
                c = *r++;
                if (c == XELP_QUO_ESC && r < end) {
                    const char *m = XELP_ESC_MAP;
                    c = *r++;
                    while (*m) { if (c == m[0]) { c = m[1]; break; } m += 2; }
                }
                *w++ = c;
            }
            if (r < end) r++;                          /* skip close quote */
        } else {
            while (r < end && *r != ' ' && *r != '\t') {
                c = *r++;
                if (c == XELP_CLI_ESC && r < end) c = *r++;
                *w++ = c;
            }
        }
        *w++ = '\0';
    }
    *argc = ac;
    return XELP_S_OK;
}
#endif /* !XELP_ENABLE_SCRIPT */

/********************************************************
 XelpParseXB() parse buffer and execute commands.
 Tokenizes each command line into argc/argv before calling the handler.
 */

XELPRESULT XelpParseXB (XELP* ths, XelpBuf *args) {
#ifdef XELP_ENABLE_SCRIPT
	{
	XELPRESULT r;
	ths->mpScriptS = args->s;
	ths->mpScriptP = args->s;
	ths->mpScriptE = args->e;
	r = _xelpEvalLoop(ths, 0);
	/* Clean up any leftover results on stack (SC-07) */
	ths->mSP = ths->mArena;
	return r;
	}
#else
	XelpBuf line;
	XELPCLIFuncMapEntry   *f;
	int argc;

	while (XELP_S_OK ==  XelpTokLineXB(args,&line,XELP_TOK_LINE) ) { /* for each logical line */

        f=ths->mpCLIModeFuncs;
        if (f) { /* make sure fn dispatch table exists */
        	ths->mR[0] = XELP_E_CMDNOTFOUND;
            while(f->mpCmd) {
                if (XELP_S_OK == XelpStrEq2(line.s,line.p,f->mpCmd))
                    break;
                f++;
            }
            if (f->mpCmd || ths->mpfDefCLI) {
                argc = 0;
                if (XELP_S_OK == _xelpBuf2Argv(ths, line.s, (int)(line.e-line.s),
                                               &argc)) {
                    if (f->mpCmd)
                        ths->mR[0] = (f->mFunPtr)(ths, argc, ths->mArgv);
                    else
                        ths->mR[0] = ths->mpfDefCLI(ths, argc, ths->mArgv);
                } else {
                    ths->mR[0] = XELP_E_ERR;
                }
            }
        }
	}
	return XELP_S_OK;
#endif /* XELP_ENABLE_SCRIPT */
}
XELPRESULT XelpParse 		(XELP *ths, const char *buf, int blen)
{
    XelpBuf args;
    XELP_XB_INIT(args,(char*)buf,blen); /* const discard is safe: tokenizer only reads */
    return XelpParseXB(ths,&args);
}
#endif /* XELP_ENABLE_CLI */

#ifdef XELP_ENABLE_CLI
/********************************************************
 XelpArgvInt() - get argv[n] as an integer.
 Returns XELP_E_ERR if n is out of range or not a valid number.
 */
XELPRESULT XelpArgvInt(const char **argv, int argc, int n, int *val)
{
    if (n < 0 || n >= argc) return XELP_E_ERR;
    return XelpParseNum(argv[n], XelpStrLen(argv[n]), val);
}

/********************************************************
 XelpArgvStr() - get argv[n] as a string pointer and length.
 Returns XELP_E_ERR if n is out of range.
 */
XELPRESULT XelpArgvStr(const char **argv, int argc, int n, const char **s, int *slen)
{
    if (n < 0 || n >= argc) return XELP_E_ERR;
    *s = argv[n];
    *slen = XelpStrLen(argv[n]);
    return XELP_S_OK;
}
#endif /* XELP_ENABLE_CLI */

/********************************************************
	XelpParseKey() 
	live command line handling. 
	first looks for mode switch commans (single-key --> cli ---> thru)
	then if in single-key mode looks up single key.
	then if in command mode looks for <ENTER> and the attempts to parse current buffer.

*/
#if defined(XELP_ENABLE_CLI) && defined(XELP_ENABLE_LINE_EDIT)
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
#if defined(XELP_ENABLE_CLI) && defined(XELP_ENABLE_LINE_EDIT) && defined(XELP_ENABLE_HISTORY)
	_xelpHistSave(ths);
#endif
	_PUTC(XELPKEY_ENTER);
	XELP_XB_INIT_PTRS(line, ths->mCmdXB.s, ths->mCmdXB.s, ths->mCmdXB.p);
#ifdef XELP_ENABLE_SCRIPT
	ths->mpScriptS = line.s;
	ths->mpScriptP = line.s;
	ths->mpScriptE = line.e;
	_xelpEvalLoop(ths, 0);
	/* Clean up any leftover results on stack after CLI statement (SC-07) */
	ths->mSP = ths->mArena;
#else
	XelpParseXB(ths, &line);
#endif
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

#if defined(XELP_ENTER_CR) && defined(XELP_ENTER_LF)
	/* Coalesce CRLF: if last key was CR, swallow the trailing LF. */
	if (ths->mLastWasCR) {
		ths->mLastWasCR = 0;
		if (key == '\n') return XELP_S_OK;
	}
	if (key == '\r') ths->mLastWasCR = 1;
#endif

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
					break;
			}

			if (ths->mCurMode != i) {
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
/********************************************************
 XELP Script Engine
 All script code is gated by XELP_ENABLE_SCRIPT.
 ********************************************************/
#ifdef XELP_ENABLE_SCRIPT

/*****************************************
 _xelpArenaInit() - reset arena pointers to empty state.
 SP starts at arena[0], HP starts at arena[end].
 */
static void _xelpArenaInit(XELP *ths) {
    ths->mSP = ths->mArena;
    ths->mHP = ths->mArena + XELP_SCRIPT_ARENA_SZ;
    ths->mFrameDepth = 0;
    ths->mpScriptS = 0;
    ths->mpScriptP = 0;
    ths->mpScriptE = 0;
    ths->mpFrameArgData = 0;
    ths->mFrameArgc = 0;
}

/*****************************************
 Pointer store/load helpers (alignment-safe, byte-by-byte).
 Used to store/load pointers in arena frame entries on targets
 where unaligned pointer access may fault.
 */
#define XELP_PTR_SZ ((int)sizeof(void*))

static void _xelpStorePtr(char *dst, const void *val) {
    int i;
    const char *src = (const char *)&val;
    for (i = 0; i < XELP_PTR_SZ; i++) dst[i] = src[i];
}

static void _xelpLoadPtr(const char *src, void *out) {
    int i;
    char *dst = (char *)out;
    for (i = 0; i < XELP_PTR_SZ; i++) dst[i] = src[i];
}

/*****************************************
 Frame layout on arena stack:

   Offset          Size       Field
   0               1          XELP_VAL_FRAME (0xF0)
   1               PTR_SZ     caller's mpScriptS
   1+PS            PTR_SZ     caller's mpScriptP (resume point)
   1+2*PS          PTR_SZ     caller's mpScriptE
   1+3*PS          PTR_SZ     caller's mpFrameArgData
   1+4*PS          1          caller's mFrameArgc
   2+4*PS          1          new frame's argc
   3+4*PS          2          argdata length (little-endian 16-bit)
   5+4*PS          var        argdata: "cmd\0arg1\0arg2\0"

 Total: 5 + 4*PTR_SZ + argdata_len bytes per frame.
 */
#define XELP_FRAME_HDR (5 + 4 * XELP_PTR_SZ)

/* _xelpFramePush: save caller context, copy argv into arena frame.
   Sets up mpFrameArgData and mFrameArgc for new frame.
   Returns XELP_E_ARENA_FULL if arena is exhausted. */
static XELPRESULT _xelpFramePush(XELP *ths, const char **argv, int argc) {
    int argDataLen = 0;
    int frameSize, i;
    char *p;

    /* Compute total argdata length */
    for (i = 0; i < argc; i++)
        argDataLen += XelpStrLen(argv[i]) + 1;

    frameSize = XELP_FRAME_HDR + argDataLen;
    if (ths->mSP + frameSize > ths->mHP)
        return XELP_E_ARENA_FULL;

    p = ths->mSP;

    /* Write frame header */
    p[0] = (char)XELP_VAL_FRAME;
    _xelpStorePtr(p + 1,              ths->mpScriptS);
    _xelpStorePtr(p + 1 + XELP_PTR_SZ,   ths->mpScriptP);
    _xelpStorePtr(p + 1 + 2*XELP_PTR_SZ, ths->mpScriptE);
    _xelpStorePtr(p + 1 + 3*XELP_PTR_SZ, ths->mpFrameArgData);
    p[1 + 4*XELP_PTR_SZ] = ths->mFrameArgc;
    p[2 + 4*XELP_PTR_SZ] = (char)argc;
    p[3 + 4*XELP_PTR_SZ] = (char)(argDataLen & 0xFF);
    p[4 + 4*XELP_PTR_SZ] = (char)((argDataLen >> 8) & 0xFF);

    /* Copy argv strings into argdata region */
    {
        char *wp = p + XELP_FRAME_HDR;
        for (i = 0; i < argc; i++) {
            const char *s = argv[i];
            while (*s) *wp++ = *s++;
            *wp++ = '\0';
        }
    }

    ths->mSP += frameSize;
    ths->mFrameDepth++;
    ths->mpFrameArgData = p + XELP_FRAME_HDR;
    ths->mFrameArgc = (char)argc;
    return XELP_S_OK;
}

/* _xelpFramePop: restore caller context from top frame on arena stack.
   Preserves any result entries above the frame by shifting them down.
   Assumes mFrameDepth > 0. */
static void _xelpFramePop(XELP *ths) {
    char *base = ths->mArena;
    char *scan = base;
    char *lastFrame = 0;
    int frameSize = 0;

    /* Walk stack to find topmost XELP_VAL_FRAME */
    while (scan < ths->mSP) {
        unsigned char kind = (unsigned char)*scan;
        if (kind == XELP_VAL_FRAME) {
            lastFrame = scan;
            frameSize = _xelpStackEntrySize(scan);
        }
        scan += _xelpStackEntrySize(scan);
    }
    if (!lastFrame) return;

    /* Restore caller context from frame header */
    _xelpLoadPtr(lastFrame + 1,                    &ths->mpScriptS);
    _xelpLoadPtr(lastFrame + 1 + XELP_PTR_SZ,     &ths->mpScriptP);
    _xelpLoadPtr(lastFrame + 1 + 2*XELP_PTR_SZ,   &ths->mpScriptE);
    _xelpLoadPtr(lastFrame + 1 + 3*XELP_PTR_SZ,   &ths->mpFrameArgData);
    ths->mFrameArgc = lastFrame[1 + 4*XELP_PTR_SZ];

    /* Shift any result data above frame down to overwrite it */
    {
        char *frameEnd = lastFrame + frameSize;
        int resultBytes = (int)(ths->mSP - frameEnd);
        if (resultBytes > 0) {
            int i;
            for (i = 0; i < resultBytes; i++)
                lastFrame[i] = frameEnd[i];
            ths->mSP = lastFrame + resultBytes;
        } else {
            ths->mSP = lastFrame;
        }
    }
    ths->mFrameDepth--;
}

/* _xelpFrameArg: get nth argument from current frame's packed argdata.
   Returns pointer to null-terminated string, or "" if out of range. */
static const char *_xelpFrameArg(XELP *ths, int n) {
    const char *p = ths->mpFrameArgData;
    int i;
    if (!p || n < 0 || n >= (int)ths->mFrameArgc) return "";
    for (i = 0; i < n; i++) {
        while (*p) p++;
        p++;
    }
    return p;
}

/*****************************************
 _xelpIntToStr() - convert int to decimal string in caller's buffer.
 Writes digits (and optional leading '-') into buf[0..buflen).
 Returns number of chars written (no null terminator appended).
 */
static int _xelpIntToStr(int val, char *buf, int buflen) {
    char tmp[12]; /* enough for -2147483648 */
    int neg = 0, len = 0, i;

    if (buflen <= 0) return 0;

    if (val < 0) { neg = 1; val = -val; }
    /* special case: val==0 */
    if (val == 0) { tmp[len++] = '0'; }
    else {
        while (val > 0 && len < 11) {
            tmp[len++] = (char)('0' + (val % 10));
            val /= 10;
        }
    }
    if (neg && len < 11) tmp[len++] = '-';

    /* reverse into output buffer */
    if (len > buflen) len = buflen;
    for (i = 0; i < len; i++)
        buf[i] = tmp[len - 1 - i];
    return len;
}

/*****************************************
 _xelpNameHash() - 16-bit hash for variable/proc name lookup.
 Simple djb2-style hash reduced to 16 bits.
 */
static unsigned short _xelpNameHash(const char *name, int len) {
    unsigned short h = 5381;
    int i;
    for (i = 0; i < len; i++)
        h = (unsigned short)(((h << 5) + h) ^ (unsigned char)name[i]);
    return h;
}

/*****************************************
 Arena heap variable layout (grows downward from HP):
   [kind:1][hash:2][nameLen:1][name:nameLen][value...]
   - INT value: 4 bytes (stored as little-endian int)
   - STR value: [strLen:2][strBytes:strLen]
 Total entry size for INT: 1+2+1+nameLen+4 = nameLen+8
 Total entry size for STR: 1+2+1+nameLen+2+strLen = nameLen+strLen+6
 */

/* Internal: scan heap for variable by name. Returns pointer to kind byte or NULL. */
static char *_xelpVarFind(XELP *ths, const char *name, int nlen) {
    unsigned short hash = _xelpNameHash(name, nlen);
    char *p = ths->mHP;
    char *end = ths->mArena + XELP_SCRIPT_ARENA_SZ;

    while (p < end) {
        unsigned char kind = (unsigned char)*p;
        unsigned short eh;
        unsigned char enl;
        int entrySize;

        eh = (unsigned short)(((unsigned char)p[1]) | ((unsigned char)p[2] << 8));
        enl = (unsigned char)p[3];

        if (kind == XELP_VAL_PROC) {
            int blen = (int)(((unsigned char)p[4 + enl]) | ((unsigned char)p[5 + enl] << 8));
            entrySize = 4 + (int)enl + 2 + blen;
        } else if (kind == XELP_VAL_INT) {
            entrySize = 4 + (int)enl + 4;
        } else if (kind == XELP_VAL_STR) {
            int slen = (int)(((unsigned char)p[4 + enl]) | ((unsigned char)p[5 + enl] << 8));
            entrySize = 4 + (int)enl + 2 + slen;
        } else {
            break; /* unknown kind terminates scan */
        }

        if (eh == hash && enl == (unsigned char)nlen) {
            const char *en = p + 4;
            int i, match = 1;
            for (i = 0; i < nlen; i++) {
                if (en[i] != name[i]) { match = 0; break; }
            }
            if (match) return p;
        }

        p += entrySize;
    }
    return 0; /* not found */
}

/* Internal: get the total size of a variable entry pointed to by p */
static int _xelpVarEntrySize(const char *p) {
    unsigned char kind = (unsigned char)*p;
    unsigned char enl = (unsigned char)p[3];
    if (kind == XELP_VAL_INT) return 4 + (int)enl + 4;
    if (kind == XELP_VAL_PROC) {
        int blen = (int)(((unsigned char)p[4 + enl]) | ((unsigned char)p[5 + enl] << 8));
        return 4 + (int)enl + 2 + blen;
    }
    /* STR */
    {
        int slen = (int)(((unsigned char)p[4 + enl]) | ((unsigned char)p[5 + enl] << 8));
        return 4 + (int)enl + 2 + slen;
    }
}

/* Internal: store a 32-bit int as 4 bytes (little-endian) */
static void _xelpStoreInt(char *dst, int val) {
    unsigned int u = (unsigned int)val;
    dst[0] = (char)(u & 0xFF);
    dst[1] = (char)((u >> 8) & 0xFF);
    dst[2] = (char)((u >> 16) & 0xFF);
    dst[3] = (char)((u >> 24) & 0xFF);
}

/* Internal: load a 32-bit int from 4 bytes (little-endian) */
static int _xelpLoadInt(const char *src) {
    unsigned int u = ((unsigned char)src[0])
                   | (((unsigned int)(unsigned char)src[1]) << 8)
                   | (((unsigned int)(unsigned char)src[2]) << 16)
                   | (((unsigned int)(unsigned char)src[3]) << 24);
    return (int)u;
}

/* Set/create variable. Returns XELP_S_OK or XELP_E_ARENA_FULL. */
static XELPRESULT _xelpVarSet(XELP *ths, const char *name, int nlen,
                              unsigned char kind, int intVal,
                              const char *strVal, int strLen) {
    char *existing = _xelpVarFind(ths, name, nlen);
    unsigned short hash = _xelpNameHash(name, nlen);
    int newSize, i;

    if (kind == XELP_VAL_INT) {
        newSize = 4 + nlen + 4;
    } else {
        newSize = 4 + nlen + 2 + strLen;
    }

    if (existing) {
        int oldSize = _xelpVarEntrySize(existing);
        unsigned char oldKind = (unsigned char)*existing;

        /* Fast path: INT overwriting INT (same size) */
        if (kind == XELP_VAL_INT && oldKind == XELP_VAL_INT) {
            _xelpStoreInt(existing + 4 + nlen, intVal);
            return XELP_S_OK;
        }

        /* Need to delete old and create new: shift heap entries */
        {
            char *entryEnd = existing + oldSize;
            char *heapEnd = ths->mArena + XELP_SCRIPT_ARENA_SZ;
            int moveBytes = (int)(existing - ths->mHP);

            /* shift entries that are below this one upward */
            if (moveBytes > 0) {
                char *dst = existing + oldSize - 1;
                char *src = existing - 1;
                while (moveBytes-- > 0) { *dst-- = *src--; }
            }
            ths->mHP += oldSize;
            (void)entryEnd;
            (void)heapEnd;
        }
        existing = 0; /* invalidated */
    }

    /* Check space */
    if (ths->mSP + newSize > ths->mHP - newSize)
        return XELP_E_ARENA_FULL;

    /* Allocate at HP (grows down) */
    ths->mHP -= newSize;
    {
        char *p = ths->mHP;
        p[0] = (char)kind;
        p[1] = (char)(hash & 0xFF);
        p[2] = (char)((hash >> 8) & 0xFF);
        p[3] = (char)nlen;
        for (i = 0; i < nlen; i++) p[4 + i] = name[i];
        if (kind == XELP_VAL_INT) {
            _xelpStoreInt(p + 4 + nlen, intVal);
        } else {
            p[4 + nlen] = (char)(strLen & 0xFF);
            p[4 + nlen + 1] = (char)((strLen >> 8) & 0xFF);
            for (i = 0; i < strLen; i++) p[4 + nlen + 2 + i] = strVal[i];
        }
    }
    return XELP_S_OK;
}

/* Get variable value. Returns XELP_S_OK or XELP_E_UNDEF_VAR. */
static XELPRESULT _xelpVarGet(XELP *ths, const char *name, int nlen, XelpResult *out) {
    char *p = _xelpVarFind(ths, name, nlen);
    unsigned char enl;
    if (!p) return XELP_E_UNDEF_VAR;

    enl = (unsigned char)p[3];
    out->kind = (unsigned char)p[0];
    if (out->kind == XELP_VAL_INT) {
        out->intVal = _xelpLoadInt(p + 4 + enl);
        out->strVal = 0;
        out->strLen = 0;
    } else if (out->kind == XELP_VAL_STR) {
        out->strLen = (int)(((unsigned char)p[4 + enl]) | ((unsigned char)p[5 + enl] << 8));
        out->strVal = p + 4 + enl + 2;
        out->intVal = 0;
    } else {
        /* PROC or other - return as NIL for variable access */
        out->kind = XELP_VAL_NIL;
        out->intVal = 0;
        out->strVal = 0;
        out->strLen = 0;
    }
    return XELP_S_OK;
}

/*****************************************
 Result stack operations (grows up from SP).
 Result entry format: [kind:1][payload:4 for INT, 2+N for STR]
 NIL entry: [0x00] (1 byte)
 INT entry: [0x03][int:4] (5 bytes)
 STR entry: [0x10][len:2][bytes:len] (3+len bytes)
 */

static XELPRESULT _xelpResultPushNil(XELP *ths) {
    if (ths->mSP + 1 > ths->mHP) return XELP_E_ARENA_FULL;
    *ths->mSP++ = (char)XELP_VAL_NIL;
    return XELP_S_OK;
}

static XELPRESULT _xelpResultPushInt(XELP *ths, int val) {
    if (ths->mSP + 5 > ths->mHP) return XELP_E_ARENA_FULL;
    *ths->mSP++ = (char)XELP_VAL_INT;
    _xelpStoreInt(ths->mSP, val);
    ths->mSP += 4;
    return XELP_S_OK;
}

static XELPRESULT _xelpResultPushStr(XELP *ths, const char *s, int slen) {
    int i;
    if (ths->mSP + 3 + slen > ths->mHP) return XELP_E_ARENA_FULL;
    *ths->mSP++ = (char)XELP_VAL_STR;
    *ths->mSP++ = (char)(slen & 0xFF);
    *ths->mSP++ = (char)((slen >> 8) & 0xFF);
    for (i = 0; i < slen; i++) *ths->mSP++ = s[i];
    return XELP_S_OK;
}

/* Peek at the top result without removing it. Returns kind, or XELP_VAL_NIL if empty.
   Used by _if condition evaluation and paren result handling. */
/* Helper: compute size of a stack entry starting at p.
   Handles NIL, INT, STR, and FRAME entries. */
static int _xelpStackEntrySize(const char *p) {
    unsigned char k = (unsigned char)*p;
    if (k == XELP_VAL_INT) return 5;
    if (k == XELP_VAL_STR) {
        int sl = (int)(((unsigned char)p[1]) | ((unsigned char)p[2] << 8));
        return 3 + sl;
    }
    if (k == XELP_VAL_FRAME) {
        int adl = (int)(((unsigned char)p[3 + 4*XELP_PTR_SZ])
                       | ((unsigned char)p[4 + 4*XELP_PTR_SZ] << 8));
        return XELP_FRAME_HDR + adl;
    }
    return 1; /* NIL or unknown: skip byte */
}

/* Pop top result into XelpResult. Skips frame entries. Returns XELP_S_OK or XELP_E_ERR if empty. */
static XELPRESULT _xelpResultPop(XELP *ths, XelpResult *out) {
    char *base = ths->mArena;
    char *p, *last;
    /* Walk from base to find the last non-frame entry.
       If stack is empty or contains only frame entries, last stays 0. */
    last = 0;
    if (ths->mSP > base) {
        p = base;
        while (p < ths->mSP) {
            unsigned char k = (unsigned char)*p;
            if (k != XELP_VAL_FRAME) last = p;
            p += _xelpStackEntrySize(p);
        }
    }
    if (!last) {
        out->kind = XELP_VAL_NIL; out->intVal = 0; out->strVal = 0; out->strLen = 0;
        return XELP_E_ERR;
    }
    {
        unsigned char k = (unsigned char)*last;
        out->kind = k;
        if (k == XELP_VAL_INT) {
            out->intVal = _xelpLoadInt(last + 1);
            out->strVal = 0; out->strLen = 0;
        } else if (k == XELP_VAL_STR) {
            out->strLen = (int)(((unsigned char)last[1]) | ((unsigned char)last[2] << 8));
            out->strVal = last + 3;
            out->intVal = 0;
        } else {
            out->intVal = 0; out->strVal = 0; out->strLen = 0;
        }
        ths->mSP = last; /* pop */
    }
    return XELP_S_OK;
}

/*****************************************
 Public result API
 */
XELPRESULT XelpSetResultInt(XELP *ths, int val) {
    return _xelpResultPushInt(ths, val);
}

XELPRESULT XelpSetResultStr(XELP *ths, const char *s, int slen) {
    return _xelpResultPushStr(ths, s, slen);
}

XELPRESULT XelpGetResult(XELP *ths, XelpResult *result) {
    return _xelpResultPop(ths, result);
}

/*****************************************
 _xelpNextTokSpan() - shared token boundary scanner.
 Reports the next token's start and end in the source buffer without copying.
 Handles: whitespace skip, quoted strings (with escape), unquoted tokens, CLI_ESC.
 Sets *isQuoted = 1 if the token was quoted.
 Returns: 1 if a token was found, 0 if end of input.
 Advances *pos past the token (including closing quote).
 */
static int _xelpNextTokSpan(const char **pos, const char *end,
                            const char **tokStart, const char **tokEnd,
                            int *isQuoted) {
    const char *r = *pos;
    *isQuoted = 0;

    /* skip whitespace */
    while (r < end && (*r == ' ' || *r == '\t')) r++;
    if (r >= end) { *pos = r; return 0; }

    if (*r == '"') {
        /* quoted token: content is between quotes */
        *isQuoted = 1;
        *tokStart = r; /* include the opening quote for type inference */
        r++;
        while (r < end && *r != '"') {
            if (*r == XELP_QUO_ESC && r + 1 < end) r++;
            r++;
        }
        if (r < end) r++; /* skip closing quote */
        *tokEnd = r;
    } else {
        /* unquoted token */
        *tokStart = r;
        while (r < end && *r != ' ' && *r != '\t') {
            if (*r == XELP_CLI_ESC && r + 1 < end) r++;
            r++;
        }
        *tokEnd = r;
    }
    *pos = r;
    return 1;
}

/*****************************************
 Paren pre-pass: copies line into scratch, inserts spaces around ( and )
 so they become separate tokens for the evaluator.
 Returns length of processed string in scratch, or -1 on overflow.
 */
static int _xelpParenPrepass(const char *src, int srcLen, char *scratch, int scratchLen) {
    int si = 0, di = 0;
    int inQuote = 0;

    while (si < srcLen && di < scratchLen - 1) {
        char c = src[si];
        if (c == '"') {
            inQuote = !inQuote;
            scratch[di++] = c;
            si++;
        } else if (!inQuote && (c == '(' || c == ')')) {
            /* insert space before paren if not at start and prev not space */
            if (di > 0 && scratch[di-1] != ' ' && di < scratchLen - 1)
                scratch[di++] = ' ';
            if (di < scratchLen - 1)
                scratch[di++] = c;
            /* insert space after paren */
            if (di < scratchLen - 1)
                scratch[di++] = ' ';
            si++;
        } else if (!inQuote && c == XELP_QUO_ESC && si + 1 < srcLen) {
            scratch[di++] = c; si++;
            if (di < scratchLen - 1) { scratch[di++] = src[si]; si++; }
        } else {
            scratch[di++] = c;
            si++;
        }
    }
    if (di >= scratchLen) return -1;
    scratch[di] = '\0';
    return di;
}

/*****************************************
 Script evaluator: _xelpEvalStatement()
 Evaluates a single statement line (after tokenization by XelpTokLineXB).
 Handles: $var expansion, @n params, builtin dispatch, user command dispatch.
 */

/* Forward declarations */
static XELPRESULT _xelpEvalLoop(XELP *ths, int targetDepth);
static XELPRESULT _xelpEvalStatement(XELP *ths, const char *lineS, int lineLen);

/* Expand $var or @n in a token, writing result into scratch buffer.
   Returns length written, or negative on error. */
static int _xelpExpandToken(XELP *ths, const char *tok, int tokLen,
                            char *buf, int bufLen) {
    /* Only called for $-prefixed tokens */
    XelpResult val;
    XELPRESULT r;
    if (tokLen <= 1) return 0; /* bare $ */

    r = _xelpVarGet(ths, tok + 1, tokLen - 1, &val);
    if (r != XELP_S_OK) return -1; /* XELP_E_UNDEF_VAR */
    if (val.kind == XELP_VAL_INT) {
        return _xelpIntToStr(val.intVal, buf, bufLen);
    } else if (val.kind == XELP_VAL_STR) {
        int i;
        if (val.strLen > bufLen) return -2;
        for (i = 0; i < val.strLen; i++) buf[i] = val.strVal[i];
        return val.strLen;
    }
    return 0; /* NIL expands to empty */
}

/*****************************************
 Builtin dispatch table (internal)
 */

/* _print: concatenate argv[1..] to output */
static XELPRESULT _xelpBuiltin_print(XELP *ths, int argc, const char **argv) {
    int i;
    (void)argv;
    for (i = 1; i < argc; i++) {
        const char *s = argv[i];
        while (*s) { _PUTC(*s); s++; }
    }
    return XELP_S_OK;
}

/* _set: set variable with type inference */
static XELPRESULT _xelpBuiltin_set(XELP *ths, int argc, const char **argv) {
    const char *name;
    int nlen, valInt;

    if (argc < 3) return XELP_E_ERR;

    /* argv[1] = variable name (already expanded by tokenizer if was $var) */
    name = argv[1];
    nlen = XelpStrLen(argv[1]);

    /* argv[2] = value - determine type */
    {
        const char *valStr = argv[2];
        int valLen = XelpStrLen(valStr);

        /* Type inference: unquoted numeric -> INT, else -> STR */
        if (XelpParseNum(valStr, valLen, &valInt) == XELP_S_OK) {
            return _xelpVarSet(ths, name, nlen, XELP_VAL_INT, valInt, 0, 0);
        } else {
            return _xelpVarSet(ths, name, nlen, XELP_VAL_STR, 0, valStr, valLen);
        }
    }
}

/* _mr: read/write mR[] registers, push to result stack */
static XELPRESULT _xelpBuiltin_mr(XELP *ths, int argc, const char **argv) {
    int idx, val;
    if (argc < 2) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &idx) != XELP_S_OK) return XELP_E_ERR;
    if (idx < 0 || idx >= XELP_REGS_SZ) return XELP_E_ERR;

    if (argc >= 3) {
        /* write mode */
        if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &val) != XELP_S_OK) return XELP_E_ERR;
        ths->mR[idx] = val;
        return _xelpResultPushInt(ths, val);
    }
    /* read mode */
    return _xelpResultPushInt(ths, (int)ths->mR[idx]);
}

/* Math builtins */
static XELPRESULT _xelpBuiltin_add(XELP *ths, int argc, const char **argv) {
    int sum = 0, i, val;
    for (i = 1; i < argc; i++) {
        if (XelpParseNum(argv[i], XelpStrLen(argv[i]), &val) != XELP_S_OK)
            return XELP_E_TYPE_ERR;
        sum += val;
    }
    return _xelpResultPushInt(ths, sum);
}

static XELPRESULT _xelpBuiltin_sub(XELP *ths, int argc, const char **argv) {
    int result, val;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &result) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &val) != XELP_S_OK) return XELP_E_TYPE_ERR;
    return _xelpResultPushInt(ths, result - val);
}

static XELPRESULT _xelpBuiltin_mul(XELP *ths, int argc, const char **argv) {
    int result = 1, i, val;
    for (i = 1; i < argc; i++) {
        if (XelpParseNum(argv[i], XelpStrLen(argv[i]), &val) != XELP_S_OK)
            return XELP_E_TYPE_ERR;
        result *= val;
    }
    return _xelpResultPushInt(ths, result);
}

static XELPRESULT _xelpBuiltin_div(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (b == 0) return XELP_E_ERR;
    return _xelpResultPushInt(ths, a / b);
}

static XELPRESULT _xelpBuiltin_mod(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (b == 0) return XELP_E_ERR;
    return _xelpResultPushInt(ths, a % b);
}

/* Bitwise builtins */
static XELPRESULT _xelpBuiltin_band(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    return _xelpResultPushInt(ths, a & b);
}

static XELPRESULT _xelpBuiltin_bor(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    return _xelpResultPushInt(ths, a | b);
}

static XELPRESULT _xelpBuiltin_bxor(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    return _xelpResultPushInt(ths, a ^ b);
}

static XELPRESULT _xelpBuiltin_bnot(XELP *ths, int argc, const char **argv) {
    int a;
    if (argc < 2) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    return _xelpResultPushInt(ths, ~a);
}

static XELPRESULT _xelpBuiltin_shl(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (b < 0 || b >= 32) return XELP_E_ERR;
    return _xelpResultPushInt(ths, (int)((unsigned)a << b));
}

static XELPRESULT _xelpBuiltin_shr(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (b < 0 || b >= 32) return XELP_E_ERR;
    return _xelpResultPushInt(ths, (int)((unsigned int)a >> b));
}

static XELPRESULT _xelpBuiltin_inc(XELP *ths, int argc, const char **argv) {
    XelpResult val;
    const char *name;
    int nlen, newVal;
    if (argc < 2) return XELP_E_ERR;
    name = argv[1]; nlen = XelpStrLen(name);
    if (_xelpVarGet(ths, name, nlen, &val) != XELP_S_OK) return XELP_E_UNDEF_VAR;
    if (val.kind != XELP_VAL_INT) return XELP_E_TYPE_ERR;
    newVal = val.intVal + 1;
    _xelpVarSet(ths, name, nlen, XELP_VAL_INT, newVal, 0, 0);
    return _xelpResultPushInt(ths, newVal);
}

static XELPRESULT _xelpBuiltin_dec(XELP *ths, int argc, const char **argv) {
    XelpResult val;
    const char *name;
    int nlen, newVal;
    if (argc < 2) return XELP_E_ERR;
    name = argv[1]; nlen = XelpStrLen(name);
    if (_xelpVarGet(ths, name, nlen, &val) != XELP_S_OK) return XELP_E_UNDEF_VAR;
    if (val.kind != XELP_VAL_INT) return XELP_E_TYPE_ERR;
    newVal = val.intVal - 1;
    _xelpVarSet(ths, name, nlen, XELP_VAL_INT, newVal, 0, 0);
    return _xelpResultPushInt(ths, newVal);
}

/* Comparison builtins */
static XELPRESULT _xelpBuiltin_eq(XELP *ths, int argc, const char **argv) {
    int a, b, result;
    if (argc < 3) return XELP_E_ERR;
    /* Try numeric comparison first */
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) == XELP_S_OK &&
        XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) == XELP_S_OK) {
        result = (a == b) ? 1 : 0;
    } else {
        /* String comparison */
        result = (XelpBufCmp(argv[1], argv[1] + XelpStrLen(argv[1]),
                             argv[2], argv[2] + XelpStrLen(argv[2]),
                             XELP_CMP_TYPE_BUF) == XELP_S_OK) ? 1 : 0;
    }
    return _xelpResultPushInt(ths, result);
}

static XELPRESULT _xelpBuiltin_neq(XELP *ths, int argc, const char **argv) {
    int a, b, result;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) == XELP_S_OK &&
        XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) == XELP_S_OK) {
        result = (a != b) ? 1 : 0;
    } else {
        result = (XelpBufCmp(argv[1], argv[1] + XelpStrLen(argv[1]),
                             argv[2], argv[2] + XelpStrLen(argv[2]),
                             XELP_CMP_TYPE_BUF) != XELP_S_OK) ? 1 : 0;
    }
    return _xelpResultPushInt(ths, result);
}

static XELPRESULT _xelpBuiltin_gt(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    return _xelpResultPushInt(ths, (a > b) ? 1 : 0);
}

static XELPRESULT _xelpBuiltin_lt(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    return _xelpResultPushInt(ths, (a < b) ? 1 : 0);
}

static XELPRESULT _xelpBuiltin_ge(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    return _xelpResultPushInt(ths, (a >= b) ? 1 : 0);
}

static XELPRESULT _xelpBuiltin_le(XELP *ths, int argc, const char **argv) {
    int a, b;
    if (argc < 3) return XELP_E_ERR;
    if (XelpParseNum(argv[1], XelpStrLen(argv[1]), &a) != XELP_S_OK) return XELP_E_TYPE_ERR;
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &b) != XELP_S_OK) return XELP_E_TYPE_ERR;
    return _xelpResultPushInt(ths, (a <= b) ? 1 : 0);
}

/* Logic builtins - truthiness: 0 and "" are false, everything else true */
static int _xelpTruthy(const char *s) {
    int v;
    if (!s || !*s) return 0; /* empty = false */
    if (XelpParseNum(s, XelpStrLen(s), &v) == XELP_S_OK) return v != 0;
    return 1; /* non-empty non-numeric string = true */
}

static XELPRESULT _xelpBuiltin_and(XELP *ths, int argc, const char **argv) {
    if (argc < 3) return XELP_E_ERR;
    return _xelpResultPushInt(ths, (_xelpTruthy(argv[1]) && _xelpTruthy(argv[2])) ? 1 : 0);
}

static XELPRESULT _xelpBuiltin_or(XELP *ths, int argc, const char **argv) {
    if (argc < 3) return XELP_E_ERR;
    return _xelpResultPushInt(ths, (_xelpTruthy(argv[1]) || _xelpTruthy(argv[2])) ? 1 : 0);
}

static XELPRESULT _xelpBuiltin_not(XELP *ths, int argc, const char **argv) {
    if (argc < 2) return XELP_E_ERR;
    return _xelpResultPushInt(ths, _xelpTruthy(argv[1]) ? 0 : 1);
}

/* _lpad: right-align string with space padding */
static XELPRESULT _xelpBuiltin_lpad(XELP *ths, int argc, const char **argv) {
    int width, slen, pad, i;
    const char *s;
    if (argc < 3) return XELP_E_ERR;
    s = argv[1];
    slen = XelpStrLen(s);
    if (XelpParseNum(argv[2], XelpStrLen(argv[2]), &width) != XELP_S_OK) return XELP_E_TYPE_ERR;
    pad = width - slen;
    for (i = 0; i < pad; i++) _PUTC(' ');
    for (i = 0; i < slen; i++) _PUTC(s[i]);
    return XELP_S_OK;
}

/*****************************************
 Control flow builtins
 */

/* _if: _if <cond> _then <cmd> [_else <cmd>]
   cond is truthy if: non-zero int, non-empty string, or result of () evaluation */
static XELPRESULT _xelpBuiltin_if(XELP *ths, int argc, const char **argv) {
    int condTrue, thenIdx = -1, elseIdx = -1, i;
    const char *condStr;

    if (argc < 4) return XELP_E_ERR; /* minimum: _if cond _then cmd */

    /* Find _then and _else boundaries */
    for (i = 2; i < argc; i++) {
        if (XelpStrEq(argv[i], XelpStrLen(argv[i]), "_then") == XELP_S_OK) thenIdx = i;
        else if (XelpStrEq(argv[i], XelpStrLen(argv[i]), "_else") == XELP_S_OK) elseIdx = i;
    }
    if (thenIdx < 0) return XELP_E_ERR;

    /* Evaluate condition: argv[1] */
    condStr = argv[1];
    condTrue = _xelpTruthy(condStr);

    /* Execute appropriate branch */
    if (condTrue && thenIdx + 1 < argc) {
        /* Build command string from _then+1 to _else (or end) */
        int cmdEnd = (elseIdx > 0) ? elseIdx : argc;
        /* Execute single command: argv[thenIdx+1] with remaining args */
        if (thenIdx + 1 < cmdEnd) {
            /* Reconstruct command line for the _then branch */
            char cmdBuf[XELP_ARGVBUFSZ];
            int pos = 0, j;
            for (j = thenIdx + 1; j < cmdEnd && pos < XELP_ARGVBUFSZ - 2; j++) {
                const char *s = argv[j];
                if (j > thenIdx + 1 && pos < XELP_ARGVBUFSZ - 1) cmdBuf[pos++] = ' ';
                while (*s && pos < XELP_ARGVBUFSZ - 1) cmdBuf[pos++] = *s++;
            }
            cmdBuf[pos] = '\0';
            return _xelpEvalStatement(ths, cmdBuf, pos);
        }
    } else if (!condTrue && elseIdx >= 0 && elseIdx + 1 < argc) {
        /* Execute _else branch */
        char cmdBuf[XELP_ARGVBUFSZ];
        int pos = 0, j;
        for (j = elseIdx + 1; j < argc && pos < XELP_ARGVBUFSZ - 2; j++) {
            const char *s = argv[j];
            if (j > elseIdx + 1 && pos < XELP_ARGVBUFSZ - 1) cmdBuf[pos++] = ' ';
            while (*s && pos < XELP_ARGVBUFSZ - 1) cmdBuf[pos++] = *s++;
        }
        cmdBuf[pos] = '\0';
        return _xelpEvalStatement(ths, cmdBuf, pos);
    }
    return XELP_S_OK;
}

/* _next: jump forward to label or execute a sub-command */
/* _next: forward jump to label, or execute sub-command.
   For :label form: scans forward from current mpScriptP for label.
   Modifies mpScriptP directly. :_end sets position to end-of-script. */
static XELPRESULT _xelpBuiltin_next(XELP *ths, int argc, const char **argv) {
    if (argc < 2) return XELP_E_ERR;

    /* _next command - execute sub-command */
    if (argv[1][0] != ':') {
        char cmdBuf[XELP_ARGVBUFSZ];
        int pos = 0, j;
        for (j = 1; j < argc && pos < XELP_ARGVBUFSZ - 2; j++) {
            const char *s = argv[j];
            if (j > 1 && pos < XELP_ARGVBUFSZ - 1) cmdBuf[pos++] = ' ';
            while (*s && pos < XELP_ARGVBUFSZ - 1) cmdBuf[pos++] = *s++;
        }
        cmdBuf[pos] = '\0';
        return _xelpEvalStatement(ths, cmdBuf, pos);
    }

    /* _next :label - forward jump */
    {
        const char *label = argv[1];
        int labelLen = XelpStrLen(label);
        XelpBuf searchBuf;

        /* No script context (standalone call) -- nothing to scan */
        if (!ths->mpScriptS) return XELP_S_OK;

        if (XelpStrEq(label, labelLen, ":_end") == XELP_S_OK) {
            ths->mpScriptP = ths->mpScriptE;
            return XELP_S_OK;
        }
        /* Forward search from current position */
        XELP_XB_INIT_PTRS(searchBuf,
            (char*)ths->mpScriptS, (char*)ths->mpScriptP, (char*)ths->mpScriptE);
        if (XelpFindTok(&searchBuf, label, label + labelLen, XELP_TOK_LINE) == XELP_S_OK) {
            ths->mpScriptP = searchBuf.p;
            return XELP_S_OK;
        }
        return XELP_E_NO_LABEL;
    }
}

/* _goto: jump to label by scanning from start of current script buffer.
   Modifies mpScriptP directly; eval loop picks up new position on next iteration.
   If label not found: XELP_E_NO_LABEL. :_end sets position to end-of-script. */
static XELPRESULT _xelpBuiltin_goto(XELP *ths, int argc, const char **argv) {
    const char *label;
    int labelLen;
    XelpBuf searchBuf;
    if (argc < 2) return XELP_E_NO_LABEL;
    label = argv[1];
    if (label[0] != ':') return XELP_E_NO_LABEL;
    labelLen = XelpStrLen(label);

    /* No script context (standalone call) -- nothing to scan */
    if (!ths->mpScriptS) return XELP_S_OK;

    /* :_end means exit current script */
    if (XelpStrEq(label, labelLen, ":_end") == XELP_S_OK) {
        ths->mpScriptP = ths->mpScriptE;
        return XELP_S_OK;
    }
    /* Scan from beginning of current script */
    XELP_XB_INIT_PTRS(searchBuf,
        (char*)ths->mpScriptS, (char*)ths->mpScriptS, (char*)ths->mpScriptE);
    if (XelpFindTok(&searchBuf, label, label + labelLen, XELP_TOK_LINE) == XELP_S_OK) {
        ths->mpScriptP = searchBuf.p;
        return XELP_S_OK;
    }
    return XELP_E_NO_LABEL;
}

/* _return: return from current frame.
   Pushes return value onto result stack and signals XELP_S_RETURN
   so the eval loop pops the frame. */
static XELPRESULT _xelpBuiltin_return(XELP *ths, int argc, const char **argv) {
    if (ths->mFrameDepth <= 0) return XELP_E_NO_FRAME;

    if (argc >= 2) {
        /* Push return value */
        int intVal;
        const char *s = argv[1];
        int slen = XelpStrLen(s);
        if (XelpParseNum(s, slen, &intVal) == XELP_S_OK) {
            _xelpResultPushInt(ths, intVal);
            ths->mR[1] = intVal; /* mirror to mR[1] per spec */
        } else {
            _xelpResultPushStr(ths, s, slen);
        }
    } else {
        _xelpResultPushNil(ths);
    }
    return XELP_S_RETURN;
}

/* _func: define a script procedure */
static XELPRESULT _xelpBuiltin_func(XELP *ths, int argc, const char **argv) {
    const char *name, *body;
    int nlen, bodyLen;
    unsigned short hash;
    int entrySize, i;

    if (argc < 3) return XELP_E_ERR;
    name = argv[1]; nlen = XelpStrLen(name);
    body = argv[2]; bodyLen = XelpStrLen(body);

    /* Store PROC entry in heap: [kind:1][hash:2][nameLen:1][name:nlen][bodyLen:2][body:bodyLen] */
    hash = _xelpNameHash(name, nlen);
    entrySize = 4 + nlen + 2 + bodyLen;

    if ((ths->mHP - entrySize) < ths->mSP)
        return XELP_E_ARENA_FULL;

    ths->mHP -= entrySize;
    {
        char *p = ths->mHP;
        p[0] = (char)XELP_VAL_PROC;
        p[1] = (char)(hash & 0xFF);
        p[2] = (char)((hash >> 8) & 0xFF);
        p[3] = (char)nlen;
        for (i = 0; i < nlen; i++) p[4 + i] = name[i];
        /* Store body length and body inline */
        p[4 + nlen] = (char)(bodyLen & 0xFF);
        p[4 + nlen + 1] = (char)((bodyLen >> 8) & 0xFF);
        for (i = 0; i < bodyLen; i++) p[4 + nlen + 2 + i] = body[i];
    }
    return XELP_S_OK;
}

/* Find a PROC entry by name. Returns pointer to body or NULL. */
static const char *_xelpFindProc(XELP *ths, const char *name, int nlen, int *bodyLen) {
    unsigned short hash = _xelpNameHash(name, nlen);
    char *p = ths->mHP;
    char *end = ths->mArena + XELP_SCRIPT_ARENA_SZ;

    while (p < end) {
        unsigned char kind = (unsigned char)*p;
        unsigned short eh;
        unsigned char enl;
        int entrySize;

        if (kind == XELP_VAL_INT) {
            enl = (unsigned char)p[3];
            entrySize = 4 + (int)enl + 4;
        } else if (kind == XELP_VAL_STR) {
            enl = (unsigned char)p[3];
            {
                int slen = (int)(((unsigned char)p[4 + enl]) | ((unsigned char)p[5 + enl] << 8));
                entrySize = 4 + (int)enl + 2 + slen;
            }
        } else if (kind == XELP_VAL_PROC) {
            int blen;
            enl = (unsigned char)p[3];
            eh = (unsigned short)(((unsigned char)p[1]) | ((unsigned char)p[2] << 8));
            blen = (int)(((unsigned char)p[4 + enl]) | ((unsigned char)p[5 + enl] << 8));
            entrySize = 4 + (int)enl + 2 + blen;

            if (eh == hash && enl == (unsigned char)nlen) {
                const char *en = p + 4;
                int i, match = 1;
                for (i = 0; i < nlen; i++) {
                    if (en[i] != name[i]) { match = 0; break; }
                }
                if (match) {
                    *bodyLen = blen;
                    return (const char *)(p + 4 + nlen + 2);
                }
            }
        } else {
            break; /* unknown kind, end of entries */
        }
        p += entrySize;
    }

    /* Check C-registered script funcs */
    if (ths->mpScriptFuncs) {
        XELPScriptFuncEntry *sf = ths->mpScriptFuncs;
        while (sf->mpCmd) {
            if (XelpStrEq(name, nlen, sf->mpCmd) == XELP_S_OK) {
                *bodyLen = XelpStrLen(sf->mpBody);
                return sf->mpBody;
            }
            sf++;
        }
    }
    return 0;
}

/*****************************************
 _xelpEvalStatement() - evaluate one statement with $ expansion and dispatch.
 lineS points to the statement text, lineLen is its length.
 */
static XELPRESULT _xelpEvalStatement(XELP *ths, const char *lineS, int lineLen) {
    char scratch[XELP_ARGVBUFSZ];
    char expandBuf[XELP_ARGVBUFSZ];
    int argc = 0;
    const char *pos, *end, *tokS, *tokE;
    int isQuoted, ppLen;
    char *wp;
    XELPRESULT r;

    if (lineLen <= 0) return XELP_S_OK;
    /* Reject lines that exceed argument buffer size (matches legacy _xelpBuf2Argv behavior) */
    if (lineLen >= XELP_ARGVBUFSZ) return XELP_E_ERR;

    /* Label: first char ':' is a no-op (callers guarantee non-ws start) */
    if (*lineS == ':') return XELP_S_OK;

    /* Paren pre-pass */
    ppLen = _xelpParenPrepass(lineS, lineLen, scratch, XELP_ARGVBUFSZ);
    if (ppLen < 0) return XELP_E_ERR;

    /* Check for parenthesized subexpressions - evaluate them */
    {
        int hasParens = 0, si;
        for (si = 0; si < ppLen; si++) {
            if (scratch[si] == '(' && (si == 0 || scratch[si-1] != XELP_CLI_ESC)) {
                hasParens = 1; break;
            }
        }

        if (hasParens) {
            /* Evaluate parenthesized expressions by iterative substitution */
            char workBuf[XELP_ARGVBUFSZ];
            int workLen = ppLen, changed;

            /* Copy scratch to workBuf */
            for (si = 0; si < ppLen; si++) workBuf[si] = scratch[si];
            workBuf[ppLen] = '\0';

            do {
                changed = 0;
                /* Find innermost () pair */
                {
                    int openIdx = -1, closeIdx = -1;
                    for (si = 0; si < workLen; si++) {
                        if (workBuf[si] == '(') openIdx = si;
                        else if (workBuf[si] == ')') { closeIdx = si; break; }
                    }
                    if (openIdx >= 0 && closeIdx > openIdx) {
                        /* Evaluate the content between ( and ) */
                        char innerBuf[XELP_ARGVBUFSZ];
                        int innerLen = closeIdx - openIdx - 1;
                        char resultBuf[32] = {0};
                        int resultLen = 0;
                        XelpResult res;

                        for (si = 0; si < innerLen; si++)
                            innerBuf[si] = workBuf[openIdx + 1 + si];
                        innerBuf[innerLen] = '\0';

                        /* Evaluate the inner expression */
                        {
                            int savedDepth = ths->mFrameDepth;
                            r = _xelpEvalStatement(ths, innerBuf, innerLen);
                            /* If inner expr called a script function, run it */
                            if (r == XELP_S_CALL)
                                r = _xelpEvalLoop(ths, savedDepth);
                        }

                        /* Get result from stack */
                        if (_xelpResultPop(ths, &res) == XELP_S_OK) {
                            if (res.kind == XELP_VAL_INT) {
                                resultLen = _xelpIntToStr(res.intVal, resultBuf, 32);
                            } else if (res.kind == XELP_VAL_STR) {
                                resultLen = (res.strLen < 31) ? res.strLen : 31;
                                for (si = 0; si < resultLen; si++)
                                    resultBuf[si] = res.strVal[si];
                            }
                        }
                        (void)r;

                        /* Substitute: replace (expr) with result in workBuf */
                        {
                            char newWork[XELP_ARGVBUFSZ];
                            int ni = 0;
                            for (si = 0; si < openIdx && ni < XELP_ARGVBUFSZ - 1; si++)
                                newWork[ni++] = workBuf[si];
                            for (si = 0; si < resultLen && ni < XELP_ARGVBUFSZ - 1; si++)
                                newWork[ni++] = resultBuf[si];
                            for (si = closeIdx + 1; si < workLen && ni < XELP_ARGVBUFSZ - 1; si++)
                                newWork[ni++] = workBuf[si];
                            newWork[ni] = '\0';
                            workLen = ni;
                            for (si = 0; si <= workLen; si++) workBuf[si] = newWork[si];
                        }
                        changed = 1;
                    }
                }
            } while (changed);

            /* Use workBuf for final tokenization */
            for (si = 0; si <= workLen; si++) scratch[si] = workBuf[si];
            ppLen = workLen;
        }
    }

    /* Tokenize with $ expansion */
    pos = scratch;
    end = scratch + ppLen;
    wp = expandBuf;

    while (argc < (int)XELP_ARGV_CAP && _xelpNextTokSpan(&pos, end, &tokS, &tokE, &isQuoted)) {
        int tokLen = (int)(tokE - tokS);

        if (isQuoted) {
            /* Quoted: strip quotes, process escapes, store as-is */
            const char *r2 = tokS + 1; /* skip open quote */
            const char *e2 = tokE - 1; /* before close quote */
            ths->mArgv[argc++] = wp;
            while (r2 < e2 && wp < expandBuf + XELP_ARGVBUFSZ - 1) {
                char c = *r2++;
                if (c == XELP_QUO_ESC && r2 < e2) {
                    const char *m = XELP_ESC_MAP;
                    c = *r2++;
                    while (*m) { if (c == m[0]) { c = m[1]; break; } m += 2; }
                }
                *wp++ = c;
            }
            *wp++ = '\0';
        } else if (tokLen > 0 && tokS[0] == '$') {
            /* Variable expansion */
            int expLen;
            ths->mArgv[argc++] = wp;
            expLen = _xelpExpandToken(ths, tokS, tokLen, wp, (int)(expandBuf + XELP_ARGVBUFSZ - wp - 1));
            if (expLen < 0) return XELP_E_UNDEF_VAR;
            wp += expLen;
            *wp++ = '\0';
        } else if (tokLen > 1 && tokS[0] == '@') {
            /* Positional parameter expansion: @1, @2, etc. */
            int idx;
            ths->mArgv[argc++] = wp;
            if (XelpParseNum(tokS + 1, tokLen - 1, &idx) == XELP_S_OK &&
                idx >= 0 && idx < (int)ths->mFrameArgc) {
                const char *arg = _xelpFrameArg(ths, idx);
                while (*arg && wp < expandBuf + XELP_ARGVBUFSZ - 1) {
                    *wp++ = *arg++;
                }
            }
            *wp++ = '\0';
        } else {
            /* Literal token: process CLI_ESC */
            int i;
            ths->mArgv[argc++] = wp;
            for (i = 0; i < tokLen && wp < expandBuf + XELP_ARGVBUFSZ - 1; i++) {
                if (tokS[i] == XELP_CLI_ESC && i + 1 < tokLen) { i++; }
                *wp++ = tokS[i];
            }
            *wp++ = '\0';
        }
    }

    /* Check for argv overflow: if we filled ARGV_CAP, check for remaining tokens */
    if (argc >= (int)XELP_ARGV_CAP && _xelpNextTokSpan(&pos, end, &tokS, &tokE, &isQuoted))
        return XELP_E_ERR;

    if (argc == 0) return XELP_S_OK;

    /* Dispatch: builtins -> script funcs -> C commands -> default handler */
    {
        const char *cmd = ths->mArgv[0];
        int cmdLen = XelpStrLen(cmd);

        /* Check builtins (start with '_') */
        if (cmd[0] == '_') {
            if (XelpStrEq(cmd, cmdLen, "_set") == XELP_S_OK) return _xelpBuiltin_set(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_print") == XELP_S_OK) return _xelpBuiltin_print(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_mr") == XELP_S_OK) return _xelpBuiltin_mr(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_add") == XELP_S_OK) return _xelpBuiltin_add(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_sub") == XELP_S_OK) return _xelpBuiltin_sub(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_mul") == XELP_S_OK) return _xelpBuiltin_mul(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_div") == XELP_S_OK) return _xelpBuiltin_div(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_mod") == XELP_S_OK) return _xelpBuiltin_mod(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_inc") == XELP_S_OK) return _xelpBuiltin_inc(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_dec") == XELP_S_OK) return _xelpBuiltin_dec(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_eq") == XELP_S_OK) return _xelpBuiltin_eq(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_neq") == XELP_S_OK) return _xelpBuiltin_neq(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_gt") == XELP_S_OK) return _xelpBuiltin_gt(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_lt") == XELP_S_OK) return _xelpBuiltin_lt(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_ge") == XELP_S_OK) return _xelpBuiltin_ge(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_le") == XELP_S_OK) return _xelpBuiltin_le(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_and") == XELP_S_OK) return _xelpBuiltin_and(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_or") == XELP_S_OK) return _xelpBuiltin_or(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_not") == XELP_S_OK) return _xelpBuiltin_not(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_if") == XELP_S_OK) return _xelpBuiltin_if(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_next") == XELP_S_OK) return _xelpBuiltin_next(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_goto") == XELP_S_OK) return _xelpBuiltin_goto(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_return") == XELP_S_OK) return _xelpBuiltin_return(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_func") == XELP_S_OK) return _xelpBuiltin_func(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_lpad") == XELP_S_OK) return _xelpBuiltin_lpad(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_band") == XELP_S_OK) return _xelpBuiltin_band(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_bor") == XELP_S_OK) return _xelpBuiltin_bor(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_bxor") == XELP_S_OK) return _xelpBuiltin_bxor(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_bnot") == XELP_S_OK) return _xelpBuiltin_bnot(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_shl") == XELP_S_OK) return _xelpBuiltin_shl(ths, argc, ths->mArgv);
            if (XelpStrEq(cmd, cmdLen, "_shr") == XELP_S_OK) return _xelpBuiltin_shr(ths, argc, ths->mArgv);
            return XELP_E_CMDNOTFOUND;
        }

        /* Check script funcs (PROC entries) */
        {
            int bodyLen;
            const char *body = _xelpFindProc(ths, cmd, cmdLen, &bodyLen);
            if (body) {
                /* Push arena frame (saves caller context, copies argv) */
                r = _xelpFramePush(ths, ths->mArgv, argc);
                if (r != XELP_S_OK) return r;
                /* Set up new script context for function body */
                ths->mpScriptS = body;
                ths->mpScriptP = body;
                ths->mpScriptE = body + bodyLen;
                return XELP_S_CALL; /* signal eval loop to continue */
            }
        }

        /* Check user C commands */
        {
            XELPCLIFuncMapEntry *f = ths->mpCLIModeFuncs;
            if (f) {
                while (f->mpCmd) {
                    if (XelpStrEq(cmd, cmdLen, f->mpCmd) == XELP_S_OK) {
                        ths->mR[0] = f->mFunPtr(ths, argc, ths->mArgv);
                        return ths->mR[0];
                    }
                    f++;
                }
            }
        }

        /* Default handler */
        if (ths->mpfDefCLI) {
            ths->mR[0] = ths->mpfDefCLI(ths, argc, ths->mArgv);
            return ths->mR[0];
        }

        ths->mR[0] = XELP_E_CMDNOTFOUND;
        return XELP_E_CMDNOTFOUND;
    }
}

/*****************************************
 _xelpEvalLoop() - iterative script evaluator.
 Processes script from ths->mpScriptS/P/E via XelpTokLineXB.
 Runs until frame depth drops to targetDepth (script ends or all frames returned).
 Function calls push arena frames and continue the loop (no C recursion).
 _goto/_next modify mpScriptP directly; the loop picks up the new position.
 */
static XELPRESULT _xelpEvalLoop(XELP *ths, int targetDepth) {
    XelpBuf script, line;
    XELPRESULT r;

    for (;;) {
        int lineLen;
        const char *lineS;

        /* Build script buf from current context */
        XELP_XB_INIT_PTRS(script,
            (char*)ths->mpScriptS, (char*)ths->mpScriptP, (char*)ths->mpScriptE);

        if (XelpTokLineXB(&script, &line, XELP_TOK_LINE) != XELP_S_OK) {
            /* Current script ended */
            if (ths->mFrameDepth <= targetDepth)
                return XELP_S_OK;
            /* Pop frame: restore caller context and continue */
            _xelpFramePop(ths);
            continue;
        }

        /* Save advanced position back to struct */
        ths->mpScriptP = script.p;

        lineLen = (int)(line.e - line.s);
        lineS = line.s;

        if (lineLen <= 0) continue;
        if (*lineS == ':') continue; /* skip labels */

        /* Evaluate statement (handles _goto/_next/_return via signals) */
        r = _xelpEvalStatement(ths, lineS, lineLen);

        /* XELP_S_CALL: function call pushed a frame, new context is set up */
        if (r == XELP_S_CALL) continue;

        /* XELP_S_RETURN: _return executed, pop frame and check if done */
        if (r == XELP_S_RETURN) {
            _xelpFramePop(ths);
            if (ths->mFrameDepth <= targetDepth)
                return XELP_S_OK;
            continue;
        }

        /* Error propagation */
        if (r < 0 && r != XELP_E_CMDNOTFOUND) {
            if (r == XELP_E_BREAK || r == XELP_E_ARENA_FULL ||
                r == XELP_E_NO_FRAME || r == XELP_E_NO_LABEL)
                return r;
        }

        /* Breakpoint callback: fires after each statement to check budget */
        if (ths->mpfBreakpoint) {
            r = ths->mpfBreakpoint(ths);
            if (r != XELP_S_OK) return XELP_E_BREAK;
        }
    }
}

/*****************************************
 XelpCallProc() - call a script function from C code.
 cmdline: "funcname arg1 arg2 ..."
 If the command is a script function, runs its body via the eval loop.
 */
XELPRESULT XelpCallProc(XELP *ths, const char *cmdline) {
    int cmdLen = XelpStrLen(cmdline);
    int savedDepth = ths->mFrameDepth;
    XELPRESULT r = _xelpEvalStatement(ths, cmdline, cmdLen);
    if (r == XELP_S_CALL) {
        r = _xelpEvalLoop(ths, savedDepth);
    }
    return r;
}

#endif /* XELP_ENABLE_SCRIPT */

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

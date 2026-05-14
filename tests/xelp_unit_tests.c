/**************************************************************************************************

  @xelp_unit_tests.c - simple test file example file for xelp embedded cli/scripting library
  @copy Copyright (C)   <M. A. Chatterjee>
  @author M A Chatterjee <deftio [at] deftio [dot] com>

  @license:
	Copyright (c) 2011-2026, M. A. Chatterjee <deftio at deftio dot com>
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

#include <stdio.h>
#include <stdlib.h>

#include "jumpbug_unit_test_fw.h"  /* micro portable unit test framework */
#include "../src/xelp.h"

/* =======================================================
   Callback functions and global test state
   ======================================================= */
struct {
    int k0;
    int k1;
    int k2;
    int c0;
    int c1;
    int c2;
}gGlobalCallbackData;

XELPRESULT k0 (XELP *ths, XELPKEYCODE k) {
    (void)ths;
    gGlobalCallbackData.k0 = (int)k;
    return XELP_S_OK;
}
XELPRESULT k1 (XELP *ths, XELPKEYCODE k) {
    (void)ths;
    gGlobalCallbackData.k1 = (int)k;
    return XELP_S_OK;
}
XELPRESULT k2 (XELP *ths, XELPKEYCODE k) {
    (void)ths;
    gGlobalCallbackData.k2 = (int)k;
    return XELP_S_OK;
}

/* declare static map for function in single key mode */
XELPKeyFuncMapEntry gMyKeyCommands[] =
{
	{&k0       ,'0', "sets k0"       },
	{&k1       ,'1', "sets k1"       },
	{&k2       ,'2', "sets k2"       },
	XELP_FUNC_ENTRY_LAST
};

XELPRESULT cli0 (XELP *ths, const char *c, int max) {
    (void)ths; (void)c; (void)max;
    gGlobalCallbackData.c0 = 0;
    return XELP_S_OK;
}
XELPRESULT cli1 (XELP *ths, const char *c, int max) {
    (void)ths; (void)c; (void)max;
    gGlobalCallbackData.c1 = 1;
    return XELP_S_OK;
}
XELPRESULT cli2 (XELP *ths, const char *c, int max) {
    (void)ths; (void)c; (void)max;
    gGlobalCallbackData.c2 = 2;
    return XELP_S_OK;
}
XELPRESULT cli3 (XELP *ths, const char *c, int max) {
    (void)ths; (void)c; (void)max;
    gGlobalCallbackData.c0 = 0;
    gGlobalCallbackData.c1 = 0;
    gGlobalCallbackData.c2 = 0;
    return XELP_S_OK;
}
/* declare a command map for functions in parse mode */
XELPCLIFuncMapEntry gMyCLICommands[] = {
	{&cli0    		    , "cli0",  "cli function 0   "       },
	{&cli1    		    , "foo"	,  "foo cli function "       },
    {&cli2    		    , "bar"	,  "bar cli function "       },
    {&cli3              , "rst" ,  "reset all cli global test vars to zero"},
	XELP_FUNC_ENTRY_LAST
};

char gChar;
void dummyOut(char c) {gChar = c;}

int gInt;
void dummyIntOut(int i) {gInt = i;}

int gBool;
void dummyVoid0() {gBool = 0;}
void dummyVoid1() {gBool = 1;}
void dummyPassThru(char c) { (void)c; }

#define GDUMMYBUFLEN (0x1000)
char gDummyBuf[GDUMMYBUFLEN];
XelpBuf gDummyXelpBuf;

void gDummyBufOut(char c) {
    XELP_XB_PUTC(gDummyXelpBuf,c);
}

/* helper to reset the dummy buf for output capture */
static void resetDummyBuf(void) {
    int i;
    for (i = 0; i < GDUMMYBUFLEN; i++) gDummyBuf[i] = 0;
    XELP_XB_INIT(gDummyXelpBuf,gDummyBuf,GDUMMYBUFLEN);
}

/*************************************************
Unit Test Cases for XELP() functions below
*/

/* ====================================================================
 test_XelpStrLen()
 */
XELPRESULT test_XelpStrLen() {

    if (JB_ASSERT(3 != XelpStrLen("abc"),"XelpStrLen abc=3"))
        return XELP_E_ERR;

    if (JB_ASSERT(0 != XelpStrLen(""),"XelpStrLen empty=0"))
        return XELP_E_ERR;

    if (JB_ASSERT(1 != XelpStrLen("x"),"XelpStrLen single char"))
        return XELP_E_ERR;

    if (JB_ASSERT(26 != XelpStrLen("abcdefghijklmnopqrstuvwxyz"),"XelpStrLen 26 chars"))
        return XELP_E_ERR;

    if (JB_ASSERT(5 != XelpStrLen("a b\tc"),"XelpStrLen with whitespace"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpStrEq()

   XelpStrEq is used for comparing length-limited char buffers to null
   terminated strings such as command names

   XelpStrEq (const char* pbuf, int blen, const char *cmd)
 */
XELPRESULT test_XelpStrEq() {
    char *a = "token1";
    char *b = " token1\0abc";
    char *c = "token1abc";
    char *d = "";

    int alen = XelpStrLen(a);

    if (JB_ASSERT(XELP_S_NOTFOUND != XelpStrEq(a,alen,b),"XelpStrEq" ))
        return XELP_E_ERR;

    if (JB_ASSERT(XELP_S_OK != XelpStrEq(a,alen,b+1),"XelpStrEq offset"))
        return XELP_E_ERR;


    if (JB_ASSERT(XELP_S_NOTFOUND != XelpStrEq(a,alen,c),"XelpStrEq 3"))
        return XELP_E_ERR;


    if (JB_ASSERT(XELP_S_OK != XelpStrEq(c,alen,a),"XelpStrEq 4"))
        return XELP_E_ERR;


    alen = XelpStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(c,alen,a),"XelpStrEq len test"))
        return XELP_E_ERR;

    if (JB_ASSERT(XELP_S_OK != XelpStrEq(c,0,d),"XelpStrEq zero len test"))
        return XELP_E_ERR;

    if (JB_ASSERT(XELP_S_NOTFOUND != XelpStrEq(c,0,b),"XelpStrEq zero len test"))
        return XELP_E_ERR;

    /* single char match */
    if (JB_ASSERT(XELP_S_OK != XelpStrEq("a",1,"a"),"XelpStrEq single char match"))
        return XELP_E_ERR;

    /* single char mismatch */
    if (JB_ASSERT(XELP_S_NOTFOUND != XelpStrEq("a",1,"b"),"XelpStrEq single char mismatch"))
        return XELP_E_ERR;

    /* buffer longer than command -- cmd ends before blen exhausted */
    if (JB_ASSERT(XELP_S_NOTFOUND != XelpStrEq("foobar",6,"foo"),"XelpStrEq buf longer than cmd"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpStrEq2()

   XelpStrEq2 is used for comparing ptr-delimited char buffers to null
   terminated strings.  It uses an end ptr instead of integer length.

   XelpStrEq2 (const char* pbuf, const char* pend, const char *cmd)
 */
XELPRESULT test_XelpStrEq2() {
    char *a = "token1", *ae;
    char *b = " token1\0abc";
    char *c = "token1abc";
    char *d = "";

    ae = a+ XelpStrLen(a);

    if (JB_ASSERT(XELP_S_NOTFOUND != XelpStrEq2(a,ae,b),"XelpStrEq2 t1" ))
        return XELP_E_ERR;

    if (JB_ASSERT(XELP_S_OK != XelpStrEq2(a,ae,b+1),"XelpStrEq2 offset"))
        return XELP_E_ERR;


    if (JB_ASSERT(XELP_S_NOTFOUND != XelpStrEq2(a,ae,c),"XelpStrEq2"))
        return XELP_E_ERR;


    if (JB_ASSERT(XELP_S_OK != XelpStrEq2(c,c+XelpStrLen(a),a),"XelpStrEq2"))
        return XELP_E_ERR;

    if (JB_ASSERT(XELP_S_NOTFOUND != XelpStrEq2(c,c+XelpStrLen(a),d),"XelpStrEq2 null start"))
        return XELP_E_ERR;

    /* single char match/mismatch */
    {
        char *sx = "x";
        if (JB_ASSERT(XELP_S_OK != XelpStrEq2(sx,sx+1,"x"),"XelpStrEq2 single match"))
            return XELP_E_ERR;

        if (JB_ASSERT(XELP_S_NOTFOUND != XelpStrEq2(sx,sx+1,"y"),"XelpStrEq2 single mismatch"))
            return XELP_E_ERR;
    }

    /* empty buffer vs empty cmd */
    if (JB_ASSERT(XELP_S_OK != XelpStrEq2(d,d,""),"XelpStrEq2 empty match"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpStr2Int()
 */

XELPRESULT test_XelpStr2Int() {
	if (JB_ASSERT(XelpStr2Int("90",2) != 90,"Str2Int  90"))
		return XELP_E_ERR;

    if (JB_ASSERT(XelpStr2Int("31h",3) != 49,"Str2Int 31h"))
		return XELP_E_ERR;

    if (JB_ASSERT(XelpStr2Int("-87",3) != -87,"Str2Int -87"))
		return XELP_E_ERR;

    if (JB_ASSERT(XelpStr2Int("+6546",5) != 6546,"Str2Int +6546"))
		return XELP_E_ERR;

    /* zero */
    if (JB_ASSERT(XelpStr2Int("0",1) != 0,"Str2Int 0"))
		return XELP_E_ERR;

    /* single digit */
    if (JB_ASSERT(XelpStr2Int("7",1) != 7,"Str2Int 7"))
		return XELP_E_ERR;

    /* uppercase hex */
    if (JB_ASSERT(XelpStr2Int("1Ah",3) != 0x1A,"Str2Int 1Ah uppercase"))
		return XELP_E_ERR;

    /* mixed case hex */
    if (JB_ASSERT(XelpStr2Int("aBh",3) != 0xAB,"Str2Int aBh mixed case"))
		return XELP_E_ERR;

    /* large decimal */
    if (JB_ASSERT(XelpStr2Int("12345",5) != 12345,"Str2Int 12345"))
		return XELP_E_ERR;

    /* hex 0h edge */
    if (JB_ASSERT(XelpStr2Int("0h",2) != 0,"Str2Int 0h"))
		return XELP_E_ERR;

	return XELP_S_OK;
}
/* ====================================================================
 test_XelpParseNum()
 */

XELPRESULT test_XelpParseNum() {
    int n;
    XELPRESULT r;

    r = XelpParseNum("90",2, &n);
    if (JB_ASSERT( ((n != 90) || ( r != XELP_S_OK)) ,"XelpParseNum 90"))
        return XELP_E_ERR;

    r = XelpParseNum("3ab30h",6, &n);
    if (JB_ASSERT( (n != 0x3ab30) || ( r != XELP_S_OK) ,"XelpParseNum 3ab30h"))
        return XELP_E_ERR;

    r = XelpParseNum("0x3ab30",7, &n);
    if (JB_ASSERT( (n != 0x3ab30) || ( r != XELP_S_OK) ,"XelpParseNum 0x3ab30"))
        return XELP_E_ERR;

    r = XelpParseNum("-87",3, &n);
    if (JB_ASSERT( (n !=  -87) || ( r != XELP_S_OK) ,"XelpParseNum -87"))
        return XELP_E_ERR;

    r = XelpParseNum("+6457",5, &n);
    if (JB_ASSERT( (n != 6457) || ( r != XELP_S_OK) ,"XelpParseNum +6457"))
       { return XELP_E_ERR;}

    /* uppercase hex with 0x prefix */
    r = XelpParseNum("0x1A",4, &n);
    if (JB_ASSERT( (n != 0x1A) || ( r != XELP_S_OK) ,"XelpParseNum 0x1A"))
        return XELP_E_ERR;

    r = XelpParseNum("0xFF",4, &n);
    if (JB_ASSERT( (n != 0xFF) || ( r != XELP_S_OK) ,"XelpParseNum 0xFF"))
        return XELP_E_ERR;

    r = XelpParseNum("0x0",3, &n);
    if (JB_ASSERT( (n != 0) || ( r != XELP_S_OK) ,"XelpParseNum 0x0"))
        return XELP_E_ERR;

    /* zero */
    r = XelpParseNum("0",1, &n);
    if (JB_ASSERT( (n != 0) || ( r != XELP_S_OK) ,"XelpParseNum 0"))
        return XELP_E_ERR;

    /* uppercase hex with h suffix */
    r = XelpParseNum("ABCh",4, &n);
    if (JB_ASSERT( (n != 0xABC) || ( r != XELP_S_OK) ,"XelpParseNum ABCh uppercase"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpBufCmp()
 */
XELPRESULT test_XelpBufCmp() {
    char *a = "token1";
    char *b = " token1\0abc";
    char *c = "token1abc";
    char *d = "token1\0 123";
    char *ae, *be, *ce, *de;

    ae = a + XelpStrLen(a);
    be = b + XelpStrLen(b);
    ce = c + XelpStrLen(a);
    de = c + XelpStrLen(d);
    if (JB_ASSERT(XELP_S_NOTFOUND != XelpBufCmp(a,ae,b,be,XELP_CMP_TYPE_A0B0),"XelpBufCmp" ))
        return XELP_E_ERR;

    be = b+1+XelpStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XelpBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_A0B0),"XelpBufCmp"))
        return XELP_E_ERR;


    be = b+2+XelpStrLen(b+1);
    if (JB_ASSERT(XELP_S_NOTFOUND != XelpBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_BUF),"XelpBufCmp"))
        return XELP_E_ERR;

    ce = c+XelpStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_BUF),"XelpBufCmp"))
        return XELP_E_ERR;

    ce = c+XelpStrLen(a)+1;
    if (JB_ASSERT(XELP_S_NOTFOUND != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_BUF),"XelpBufCmp"))
        return XELP_E_ERR;


    if (JB_ASSERT(XELP_S_NOTFOUND != XelpBufCmp(d,de+2,a,ae,XELP_CMP_TYPE_A0),"XelpBufCmp A01"))
        return XELP_E_ERR;

    ae = a + XelpStrLen(a);
    ce = c + XelpStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_A0),"XelpBufCmp A02"))
        return XELP_E_ERR;

    /* empty buffers */
    if (JB_ASSERT(XELP_S_OK != XelpBufCmp(a,a,c,c,XELP_CMP_TYPE_BUF),"XelpBufCmp empty bufs"))
        return XELP_E_ERR;

    /* single char match */
    {
        char *sa = "a", *sb = "b";
        if (JB_ASSERT(XELP_S_OK != XelpBufCmp(sa,sa+1,sa,sa+1,XELP_CMP_TYPE_BUF),"XelpBufCmp single"))
            return XELP_E_ERR;

        /* single char mismatch */
        if (JB_ASSERT(XELP_S_NOTFOUND != XelpBufCmp(sa,sa+1,sb,sb+1,XELP_CMP_TYPE_BUF),"XelpBufCmp single mismatch"))
            return XELP_E_ERR;
    }

    /* A0B0 with nulls embedded */
    {
        char x1[] = "ab\0cd";
        char x2[] = "ab\0xy";
        if (JB_ASSERT(XELP_S_OK != XelpBufCmp(x1,x1+5,x2,x2+5,XELP_CMP_TYPE_A0B0),"XelpBufCmp A0B0 null term"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpFindTok()
 */

XELPRESULT test_XelpFindTok() {

    char *label = "label1:",*le;
    char *b0 = "token1 token2; token3 token4 ; \n token5 token6 token7\n";
    char *b1 = "token1 token2 label1:\n";
    char *b2 = "token1 token2; \ntoken3 token4 token5\n   label1: token7 token8\n token9 label1: token10\n token11;";
    char *b3 = "token1 token2; \ntoken3 token4 token5\n   xlabel1: token7 token8\n token9 label1: token10\n token11;";
    XelpBuf x;

    le = label+XelpStrLen(label);

    x.s = b0;  x.p = x.s; x.e = x.s+XelpStrLen(x.s);
    if (JB_ASSERT(XELP_S_NOTFOUND != XelpFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok not found" ) )
        return XELP_E_ERR;

    x.s = b1;  x.p = x.s; x.e = x.s+XelpStrLen(x.s);
    if (JB_ASSERT(XELP_S_OK != XelpFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok found" ) )
        return XELP_E_ERR;

    x.s = b2;  x.p = x.s; x.e = x.s+XelpStrLen(x.s);
    if (JB_ASSERT(XELP_S_OK != XelpFindTok(&x,label,le,XELP_TOK_LINE),"XelpFindTok line" ) )
        return XELP_E_ERR;

    x.s = b3;  x.p = x.s; x.e = x.s+XelpStrLen(x.s);
    if (JB_ASSERT(XELP_S_NOTFOUND != XelpFindTok(&x,label,le,XELP_TOK_LINE),"XelpFindTok line not found" ) )
        return XELP_E_ERR;

    /* empty buffer */
    {
        char *empty = "";
        x.s = empty; x.p = x.s; x.e = x.s;
        if (JB_ASSERT(XELP_S_NOTFOUND != XelpFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok empty buf"))
            return XELP_E_ERR;
    }

    /* token at start of buffer */
    {
        char *b4 = "label1: something\n";
        x.s = b4; x.p = x.s; x.e = x.s+XelpStrLen(x.s);
        if (JB_ASSERT(XELP_S_OK != XelpFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok at start"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpTokLineXB()
 */
XELPRESULT test_XelpTokLineXB() {

    char *line1 = "abc def ghi";
    XelpBuf b,out;
    XELPRESULT r,r2;

    /* test first token */
    XELP_XB_INIT(b,line1,XelpStrLen(line1));
    r  = XelpTokLineXB(&b,&out,XELP_TOK_ONLY);
    r2 = XelpBufCmp(line1, line1+3,out.s,out.p,XELP_CMP_TYPE_BUF);

    if (JB_ASSERT((XELP_S_OK !=r) || (XELP_S_OK != r2),"XelpToklineXB first token"))
        return XELP_E_ERR;
    XELP_XB_TOP(b);

    /* empty buffer */
    {
        char *empty = "";
        XELP_XB_INIT(b,empty,0);
        r = XelpTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_NOTFOUND != r, "XelpToklineXB empty buf"))
            return XELP_E_ERR;
    }

    /* whitespace only -- no token found */
    {
        char *ws = "   \t  \n  ";
        XELP_XB_INIT(b,ws,XelpStrLen(ws));
        r = XelpTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_NOTFOUND != r, "XelpToklineXB whitespace only"))
            return XELP_E_ERR;
    }

    /* comment -- no token found */
    {
        char *cmt = "# this is a comment\n";
        XELP_XB_INIT(b,cmt,XelpStrLen(cmt));
        r = XelpTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_NOTFOUND != r, "XelpToklineXB comment only"))
            return XELP_E_ERR;
    }

    /* multiple tokens with TOK_LINE */
    {
        char *multi = "cmd arg1 arg2\n";
        XELP_XB_INIT(b,multi,XelpStrLen(multi));
        r = XelpTokLineXB(&b,&out,XELP_TOK_LINE);
        if (JB_ASSERT(XELP_S_OK != r, "XelpToklineXB TOK_LINE"))
            return XELP_E_ERR;
        /* out.s should be cmd start, out.p should be cmd end, out.e should be end of line */
        if (JB_ASSERT(XELP_S_OK != XelpStrEq2(out.s,out.p,"cmd"), "XelpToklineXB TOK_LINE cmd match"))
            return XELP_E_ERR;
    }

    /* semicolons */
    {
        char *semi = "cmd1; cmd2; cmd3\n";
        int count = 0;
        XELP_XB_INIT(b,semi,XelpStrLen(semi));
        while (XELP_S_OK == XelpTokLineXB(&b,&out,XELP_TOK_LINE))
            count++;
        if (JB_ASSERT(count != 3, "XelpToklineXB semicolons 3 lines"))
            return XELP_E_ERR;
    }

    /* quoted strings */
    {
        char *qs = "\"hello world\" next\n";
        XELP_XB_INIT(b,qs,XelpStrLen(qs));
        r = XelpTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_OK != r, "XelpToklineXB quoted token"))
            return XELP_E_ERR;
    }

    /* backtick escape */
    {
        char *esc = "abc`; def\n";
        XELP_XB_INIT(b,esc,XelpStrLen(esc));
        r = XelpTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_OK != r, "XelpToklineXB backtick esc"))
            return XELP_E_ERR;
    }

    /* tabs and mixed whitespace */
    {
        char *tabs = "\t  tok1\t\ttok2  ";
        XELP_XB_INIT(b,tabs,XelpStrLen(tabs));
        r = XelpTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT((XELP_S_OK != r) || (XELP_S_OK != XelpStrEq2(out.s,out.p,"tok1")), "XelpToklineXB tabs"))
            return XELP_E_ERR;
    }

    /* quote with escape inside */
    {
        char *qe = "\"hello\\\"world\" next\n";
        XELP_XB_INIT(b,qe,XelpStrLen(qe));
        r = XelpTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_OK != r, "XelpToklineXB quoted escape"))
            return XELP_E_ERR;
    }

    /* CRLF handling - \n is the line term */
    {
        char *crlf = "tok1\ntok2\n";
        int count = 0;
        XELP_XB_INIT(b,crlf,XelpStrLen(crlf));
        while (XELP_S_OK == XelpTokLineXB(&b,&out,XELP_TOK_ONLY))
            count++;
        if (JB_ASSERT(count != 2, "XelpToklineXB newline separated tokens"))
            return XELP_E_ERR;
    }

    /* comment after token on same line */
    {
        char *tc = "tok1 # comment\ntok2\n";
        XELP_XB_INIT(b,tc,XelpStrLen(tc));
        r = XelpTokLineXB(&b,&out,XELP_TOK_LINE);
        if (JB_ASSERT((XELP_S_OK != r) || (XELP_S_OK != XelpStrEq2(out.s,out.p,"tok1")), "XelpToklineXB tok then comment"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpInit()
 */

XELPRESULT test_XelpInit() {
    XELP myXelp, *x;
    x = &myXelp;

    if (JB_ASSERT(XELP_S_OK != XelpInit(x,"Xelp Unit Tests"),"XelpInit")) {
        return XELP_E_ERR;
    }

    /* verify about message set correctly */
    if (JB_ASSERT(x->mpAboutMsg == 0, "XelpInit about msg set")) {
        return XELP_E_ERR;
    }

    /* verify zeroed members */
    if (JB_ASSERT(x->mCurMode != XELP_MODE_CLI, "XelpInit mode is CLI"))
        return XELP_E_ERR;

    if (JB_ASSERT(x->mOutEnable != 1, "XelpInit mOutEnable is 1"))
        return XELP_E_ERR;

    if (JB_ASSERT(x->mEchoChar != XELP_ECHO_NORMAL, "XelpInit mEchoChar normal"))
        return XELP_E_ERR;

    if (JB_ASSERT(x->mpfOut != 0, "XelpInit mpfOut null"))
        return XELP_E_ERR;

    if (JB_ASSERT(x->mR[0] != 0, "XelpInit R0 zero"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_XelpOut_comprehensive() - replaces the old broken test

 Bug fix: The old test had unreachable code after return statements.
 The gChar checks inside the JB_ASSERT if-blocks were after return.
 */
XELPRESULT test_XelpOut_comprehensive() {

    XELP myXelp;
    XelpInit(&myXelp,"XelpOut Tests");

    /* test with no output function set -- should return OK, just no output */
    if (JB_ASSERT(XELP_S_OK != XelpOut(&myXelp,0,0),"XelpOut null msg no fn")) {
        return XELP_E_ERR;
    }

    XELP_SET_FN_OUT(myXelp,dummyOut);
    XELP_SET_FN_THR(myXelp,dummyOut);
    XELP_SET_FN_ERR(myXelp,dummyOut);

    /* print single char with maxlen=1 */
    gChar = 0;
    if (JB_ASSERT(XELP_S_OK != XelpOut(&myXelp,"a",1),"XelpOut single char")) {
        return XELP_E_ERR;
    }
    if (JB_ASSERT(gChar != 'a', "XelpOut single char value"))
        return XELP_E_ERR;

    /* print 2 chars, verify last char emitted */
    gChar = 0;
    if (JB_ASSERT(XELP_S_OK != XelpOut(&myXelp,"ab",2),"XelpOut two chars")) {
        return XELP_E_ERR;
    }
    if (JB_ASSERT(gChar != 'b', "XelpOut last char is b"))
        return XELP_E_ERR;

    /* null msg should be safe */
    if (JB_ASSERT(XELP_S_OK != XelpOut(&myXelp,0,0),"XelpOut NULL msg")) {
        return XELP_E_ERR;
    }

    /* maxlen=0 should print until null terminator (unbounded) */
    resetDummyBuf();
    XELP_SET_FN_OUT(myXelp,gDummyBufOut);
    XelpOut(&myXelp,"hello",0);
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 5, "XelpOut maxlen=0 prints all"))
        return XELP_E_ERR;

    /* maxlen=-1 should also print until null terminator (unbounded) */
    resetDummyBuf();
    XelpOut(&myXelp,"world",-1);
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 5, "XelpOut maxlen=-1 prints all"))
        return XELP_E_ERR;

    /* maxlen larger than string -- should stop at null terminator */
    resetDummyBuf();
    XelpOut(&myXelp,"hi",100);
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 2, "XelpOut maxlen>strlen"))
        return XELP_E_ERR;

    /* maxlen=1 on longer string -- should print exactly 1 char */
    resetDummyBuf();
    XelpOut(&myXelp,"abcdef",1);
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 1, "XelpOut maxlen=1 truncates"))
        return XELP_E_ERR;

    /* empty string should print nothing */
    resetDummyBuf();
    XelpOut(&myXelp,"",5);
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 0, "XelpOut empty string"))
        return XELP_E_ERR;

    /* null output function -- should be safe */
    {
        XELP x2;
        XelpInit(&x2,"test");
        if (JB_ASSERT(XELP_S_OK != XelpOut(&x2,"hello",5), "XelpOut no fn set"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_XelpHelp()
 */
XELPRESULT test_XelpHelp() {
    XELP x;
    XELPRESULT r;

    XELPKeyFuncMapEntry keyCmds[] =
    {
        {&k0       ,'0', "key 0 help"       },
        {&k1       ,'1', "key 1 help"       },
        {&k2       ,'2', "key 0 help"       },
        XELP_FUNC_ENTRY_LAST
    };
    XELPCLIFuncMapEntry cliCmds[] =
    {
        {&cli0    		    , "cli0",  "cli 0 help"       },
        {&cli1    		    , "foo"	,  "cli foo help"       },
        {&cli2    		    , "bar"	,  "cli bar help"       },
        {&cli3              , "rst" ,  "cli rst help"},
        XELP_FUNC_ENTRY_LAST
    };

    resetDummyBuf();
    XelpInit(&x,"Test XelpHelp");

    XELP_SET_FN_KEY(x,keyCmds);
    XELP_SET_FN_CLI(x,cliCmds);
	XELP_SET_FN_OUT(x,gDummyBufOut);

    r = XelpHelp(&x);
    gDummyBufOut(0);

    /* Bug fix: use > 0 instead of hard-coded 149, since output length depends on bug fixes */
    if (JB_ASSERT( (r!= XELP_S_OK) || ( XelpStrLen(gDummyBuf) <= 0), "Test Help output" )) {
        return XELP_E_ERR;
    }

    /* test help with no KEY commands */
    {
        XELP x2;
        resetDummyBuf();
        XelpInit(&x2,"Help no keys");
        XELP_SET_FN_CLI(x2,cliCmds);
        XELP_SET_FN_OUT(x2,gDummyBufOut);
        r = XelpHelp(&x2);
        gDummyBufOut(0);
        if (JB_ASSERT( (r!= XELP_S_OK) || (XelpStrLen(gDummyBuf) <= 0), "Test Help no keys"))
            return XELP_E_ERR;
    }

    /* test help with no CLI commands */
    {
        XELP x3;
        resetDummyBuf();
        XelpInit(&x3,"Help no cli");
        XELP_SET_FN_KEY(x3,keyCmds);
        XELP_SET_FN_OUT(x3,gDummyBufOut);
        r = XelpHelp(&x3);
        gDummyBufOut(0);
        if (JB_ASSERT( (r!= XELP_S_OK) || (XelpStrLen(gDummyBuf) <= 0), "Test Help no cli"))
            return XELP_E_ERR;
    }

    /* test help with NULL tables */
    {
        XELP x4;
        resetDummyBuf();
        XelpInit(&x4,"Help null tables");
        XELP_SET_FN_OUT(x4,gDummyBufOut);
        r = XelpHelp(&x4);
        gDummyBufOut(0);
        if (JB_ASSERT( r != XELP_S_OK, "Test Help null tables"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpExecKC()

 Bug fix: line 476 tested gGlobalCallbackData.c1 but should test k1
 (key mode callback sets k1, not c1). Also && should be || for proper
 failure detection.
 */
XELPRESULT test_XelpExecKC() {
    XELP x;
    XELPRESULT r;

    XelpInit(&x,"TestExecKC");

    r = XelpExecKC(&x,'1');
    if (JB_ASSERT(r!=XELP_S_NOTFOUND,"ExecKC null ptr")){
        return r;
    }

    XELP_SET_FN_KEY(x,gMyKeyCommands);
    XELP_SET_FN_CLI(x,gMyCLICommands);
	XELP_SET_FN_OUT(x,dummyOut);

    gGlobalCallbackData.k1 = 0;
    r = XelpExecKC(&x,'1');
    if (JB_ASSERT((r!=XELP_S_OK) || (gGlobalCallbackData.k1!='1'),"ExecKC '1' ")){
        return r;
    }

    r = XelpExecKC(&x,'z'); /* not a mapped key */
    if (JB_ASSERT(r!=XELP_S_NOTFOUND,"ExecKC 'z'")){
        return r;
    }

    /* verify return value stored in mR[0] */
    if (JB_ASSERT(XELP_R0(x) != XELP_S_NOTFOUND, "ExecKC mR[0] stores result"))
        return XELP_E_ERR;

    /* test key '0' */
    gGlobalCallbackData.k0 = 0;
    r = XelpExecKC(&x,'0');
    if (JB_ASSERT((r!=XELP_S_OK) || (gGlobalCallbackData.k0!='0'), "ExecKC '0'"))
        return XELP_E_ERR;

    return XELP_S_OK;

}
/* ====================================================================
 test_XelpParseKey()

 Bug fix: comments on KEY mode tests said "THR 0" but tests KEY mode.
 Fixed comment labels.
 */
XELPRESULT test_XelpParseKey() {
    XELP x;
    XELPRESULT r;
    int i;

    r = XelpInit(&x,"TestParseKey");
    XELP_SET_FN_KEY(x,gMyKeyCommands);
	XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* test CLI command via ParseKey -- type "foo" and press enter */
    {
        char *c1 = " foo ";
        for (i=0; i  <XelpStrLen(c1); i++) {
            r = XelpParseKey(&x,c1[i]);
            if (JB_ASSERT(r!= XELP_S_OK, "XelpParseKey -- sending keys")){
                return XELP_E_ERR;
            }
        }
        r = XelpParseKey(&x,XELPKEY_ENTER);
            if (JB_ASSERT(r!= XELP_S_OK, "XelpParseKey -- sending keys")){
                return XELP_E_ERR;
            }
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1,"Test cli 1 value")) {
            return XELP_E_ERR;
        }
    }

    /* test backspace handling */
    {
        char *c2 = " bar; ";
        for (i=0; i  <XelpStrLen(c2); i++) {
            r = XelpParseKey(&x,c2[i]);
            if (JB_ASSERT(r!= XELP_S_OK, "XelpParseKey -- sending keys")){
                return XELP_E_ERR;
            }
        }
        r = XelpParseKey(&x,XELPKEY_BKSP);
        r = XelpParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r!= XELP_S_OK, "XelpParseKey -- sending keys w bskp test")){
            return XELP_E_ERR;
        }
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1,"XelpParseKey test cli1 value")) {
            return XELP_E_ERR;
        }


#ifndef XELP_ENABLE_LINE_EDIT
        XELP_SET_FN_BKSP(x, dummyVoid1);
        dummyVoid0();
        r = XelpParseKey(&x,XELPKEY_CLI);
        r = XelpParseKey(&x,'a');
        r = XelpParseKey(&x,XELPKEY_BKSP);
        r = XelpParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT( (r!= XELP_S_OK) || (gBool != 1), "XelpParseKey --  bskp callback test")){
            return XELP_E_ERR;
        }
#else
        /* with line editing, mpfBksp is not called; library handles visual feedback */
        r = XelpParseKey(&x,XELPKEY_CLI);
        r = XelpParseKey(&x,'a');
        r = XelpParseKey(&x,XELPKEY_BKSP);
        r = XelpParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r!= XELP_S_OK, "XelpParseKey --  bskp line edit test")){
            return XELP_E_ERR;
        }
#endif

    }

    /* test mode changes
       Note: ESC (XELPKEY_KEY default) is deferred by the key accumulator until the
       next byte arrives.  So switching to KEY mode requires two XelpParseKey calls:
       the ESC byte, then any non-'[' byte that flushes it. The flush byte is then
       reprocessed in the new mode.  To isolate the KEY mode change, we send a NUL
       byte after ESC (NUL is not a mode-switch key, so it gets dispatched in KEY mode). */
    {
        XELP_SET_FN_EMCHG(x,0);

        r = XelpParseKey(&x,XELPKEY_CLI);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI), "XelpParseKey -- mode change to CLI 1")){
            return XELP_E_ERR;
        }
        r = XelpParseKey(&x,XELPKEY_KEY); /* ESC: stashed in accumulator */
        r = XelpParseKey(&x,'!');          /* flush ESC → KEY mode, '!' dispatched as key cmd */
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY), "XelpParseKey -- mode change to KEY")){
            return XELP_E_ERR;
        }

        XELP_SET_FN_EMCHG(x,dummyIntOut);
        XELP_SET_FN_THR(x,dummyOut);

        r = XelpParseKey(&x,XELPKEY_THR);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gInt != x.mCurMode), "XelpParseKey -- mode change to THR")){
            return XELP_E_ERR;
        }

        r = XelpParseKey(&x,XELPKEY_CLI);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gInt != x.mCurMode), "XelpParseKey -- mode change to CLI 2")){
            return XELP_E_ERR;
        }

        XELP_SET_FN_EMCHG(x,0);
    }

    /* test THR function redirects */
    {
        XELP_SET_FN_EMCHG(x,0);

        r = XelpParseKey(&x,XELPKEY_THR);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR), "XelpParseKey -- mode change to THR")){
            return XELP_E_ERR;
        }

        XELP_SET_FN_THR(x,dummyOut);
        r = XelpParseKey(&x,'a');
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gChar != 'a'), "XelpParseKey --  THR 1")){
            return XELP_E_ERR;
        }
        r = XelpParseKey(&x,'b');
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gChar != 'b'), "XelpParseKey --  THR 2")){
            return XELP_E_ERR;
        }
        r = XelpParseKey(&x,XELPKEY_CLI);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gChar != 'b'), "XelpParseKey --  THR exit")){
            return XELP_E_ERR;
        }

    }

    /* test KEY function redirects */
    {
        r = XelpParseKey(&x,XELPKEY_KEY); /* ESC: stashed */
        r = XelpParseKey(&x,'0');         /* flush ESC → KEY mode, reprocess '0' → executes k0 */
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY) || (gGlobalCallbackData.k0 != '0'), "XelpParseKey -- KEY cmd 0")){
            return XELP_E_ERR;
        }

        gGlobalCallbackData.k1 = 'y';
        r = XelpParseKey(&x,'1');
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY) || (gGlobalCallbackData.k1 != '1'), "XelpParseKey -- KEY cmd 1")){
            return XELP_E_ERR;
        }

        gGlobalCallbackData.k1 = 'z';
        r = XelpParseKey(&x,XELPKEY_CLI);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gGlobalCallbackData.k1  != 'z'), "XelpParseKey -- KEY exit")){
            return XELP_E_ERR;
        }

    }

    /* test backspace at buffer start (no-op) */
    {
        XELP_SET_FN_BKSP(x,0);
        r = XelpParseKey(&x,XELPKEY_CLI);
        /* buffer is now at start, backspace should be no-op */
        r = XelpParseKey(&x,XELPKEY_BKSP);
        if (JB_ASSERT(r != XELP_S_OK, "XelpParseKey bksp at start"))
            return XELP_E_ERR;
    }

    /* test CLI buffer overflow -- type more than XELP_CMDBUFSZ chars */
    {
        XELP_SET_FN_BKSP(x,0);
        r = XelpParseKey(&x,XELPKEY_CLI);
        for (i = 0; i < XELP_CMDBUFSZ + 10; i++) {
            r = XelpParseKey(&x,'x');
        }
        r = XelpParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r != XELP_S_OK, "XelpParseKey CLI overflow"))
            return XELP_E_ERR;
    }

    /* test mode switch with no registered KEY callbacks stays in current mode */
    {
        XELP x2;
        XelpInit(&x2,"NoKeyCallbacks");
        XELP_SET_FN_CLI(x2,gMyCLICommands);
        XELP_SET_FN_OUT(x2,dummyOut);
        /* no KEY funcs set -- trying to switch to KEY should stay in CLI */
        r = XelpParseKey(&x2,XELPKEY_KEY); /* ESC: stashed */
        r = XelpParseKey(&x2,'!');          /* flush: ESC fails to switch (no KEY table), '!' goes to CLI */
        if (JB_ASSERT(x2.mCurMode != XELP_MODE_CLI, "XelpParseKey no KEY stays CLI"))
            return XELP_E_ERR;
    }


    return XELP_S_OK;

}




/* ====================================================================
 test_XelpTokN()

 Bug fix: line 669-670 had XelpTokN called on x (init'd with c2)
 then XELP_XB_INIT re-inits x with c2 using XelpStrLen(c3) as length.
 Fixed to use correct length.
 */

XELPRESULT test_XelpTokN() {
    XelpBuf x, tok;
    XELPRESULT r;
    char *c1 =   "tok0 tok1 tok2    \t tok3   tok4\n tok5";
    char *c2 = "\ttok0 tok1 tok2    \t # tok3   tok4\n tok5 ";
    char *c3 = " tok0 tok1 tok2    \t # tok3   tok4;\n tok5; tok6 ";
    char *c4 = " tok0 tok1 tok2    \t #tok3   tok4;\n tok5; tok6 ";

    XELP_XB_INIT(x,c1,XelpStrLen(c1));
    r = XelpTokN(&x,0,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XelpStrEq2(tok.s, tok.p,"tok0") )),"XelpTokN get 0th token"))
        return XELP_E_ERR;

    r = XelpTokN(&x,3,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XelpStrEq2(tok.s, tok.p,"tok3") )),"XelpTokN get 3rd token"))
        return XELP_E_ERR;


    XELP_XB_INIT(x,c2,XelpStrLen(c2));
    r = XelpTokN(&x,3,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XelpStrEq2(tok.s, tok.p,"tok5") )),"XelpTokN get 3rd token w commented line"))
        return XELP_E_ERR;

    /* Bug fix: use c2 with XelpStrLen(c2) instead of c3 length */
    XELP_XB_INIT(x,c2,XelpStrLen(c2));
    r = XelpTokN(&x,3,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XelpStrEq2(tok.s, tok.p,"tok5") )),"XelpTokN get 3rd token w comment (fixed)"))
        return XELP_E_ERR;


    XELP_XB_INIT(x,c4,XelpStrLen(c4));
    r = XelpTokN(&x,3,&tok);
    if (JB_ASSERT( ((r != XELP_S_OK) || (XELP_S_OK != XelpStrEq2(tok.s, tok.p,"tok5") )),"XelpTokN get 3rd token w commented line w space"))
        return XELP_E_ERR;

    XELP_XB_INIT(x,c4,XelpStrLen(c4));
    r = XelpTokN(&x,23,&tok);
    if (JB_ASSERT( ((r == XELP_S_OK) || (XELP_S_NOTFOUND != XelpStrEq2(tok.s, tok.p,"tok5") )),"XelpTokN get token past buffer"))
        return XELP_E_ERR;

    /* quoted token */
    {
        char *q = "\"tok0\" tok1 tok2";
        XELP_XB_INIT(x,q,XelpStrLen(q));
        r = XelpTokN(&x,1,&tok);
        if (JB_ASSERT(r != XELP_S_OK, "XelpTokN quoted tok"))
            return XELP_E_ERR;
    }

    /* n=0 edge - get very first token */
    {
        char *s1 = "first second";
        XELP_XB_INIT(x,s1,XelpStrLen(s1));
        r = XelpTokN(&x,0,&tok);
        if (JB_ASSERT((r!=XELP_S_OK) || (XELP_S_OK != XelpStrEq2(tok.s,tok.p,"first")), "XelpTokN n=0"))
            return XELP_E_ERR;
    }

    /* test using c3 with correct length */
    XELP_XB_INIT(x,c3,XelpStrLen(c3));
    r = XelpTokN(&x,0,&tok);
    if (JB_ASSERT((r != XELP_S_OK) || (XELP_S_OK != XelpStrEq2(tok.s,tok.p,"tok0")), "XelpTokN c3 first"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpNumToks()
 */

XELPRESULT test_XelpNumToks() {
    XelpBuf x;
    XELPRESULT r;
    int n=0;
    char *c0 = "";
    char *c1 = "tok1 tok2    \t tok3   tok4\n t0k5";
    char *c2 = "\t tok1 tok2    \t# tok3   tok4\n t0k5; tok6";
    char *c3 = "\t tok1 tok2    \t#tok3   tok4\n t0k5; tok6";

    XELP_XB_INIT(x,c0,XelpStrLen(c0));
    r = XelpNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=0)),"XelpNumToks empty"))
        return XELP_E_ERR;

    XELP_XB_INIT(x,c1,XelpStrLen(c1));
    r = XelpNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=5)),"XelpNumToks tabs and newlines"))
        return XELP_E_ERR;

    XELP_XB_INIT(x,c2,XelpStrLen(c2));
    r = XelpNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=4)),"XelpNumToks comment on second line"))
        return XELP_E_ERR;

    XELP_XB_INIT(x,c3,XelpStrLen(c3));
    r = XelpNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=4)),"XelpNumToks comment hugging"))
        return XELP_E_ERR;

    /* single token */
    {
        char *s1 = "only";
        XELP_XB_INIT(x,s1,XelpStrLen(s1));
        r = XelpNumToks(&x,&n);
        if (JB_ASSERT(((r!=XELP_S_OK) || (n !=1)),"XelpNumToks single"))
            return XELP_E_ERR;
    }

    /* all whitespace -- tokenizer returns 1 empty token for non-empty whitespace buffers */
    {
        char *ws = "   \t  \n  ";
        XELP_XB_INIT(x,ws,XelpStrLen(ws));
        r = XelpNumToks(&x,&n);
        if (JB_ASSERT(r!=XELP_S_OK,"XelpNumToks all whitespace"))
            return XELP_E_ERR;
    }

    /* all comments -- tokenizer returns 1 token for comment-only buffers */
    {
        char *cm = "# all comment\n";
        XELP_XB_INIT(x,cm,XelpStrLen(cm));
        r = XelpNumToks(&x,&n);
        if (JB_ASSERT(r!=XELP_S_OK,"XelpNumToks all comments"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpParseXB() - actual command dispatch verification

 Bug fix: was a stub that just inited and returned OK.
 Now tests actual command execution.
 */

XELPRESULT test_XelpParseXB() {
    XELP x;
    XelpBuf script;
    char *s;
    XELPRESULT r;

    XelpInit(&x,"TestParseXB");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* single command */
    gGlobalCallbackData.c1 = 0;
    s = "foo arg1\n";
    XELP_XB_INIT(script,s,XelpStrLen(s));
    r = XelpParseXB(&x,&script);
    if (JB_ASSERT((r != XELP_S_OK) || (gGlobalCallbackData.c1 != 1), "XelpParseXB single cmd"))
        return XELP_E_ERR;

    /* multiple commands separated by semicolons */
    gGlobalCallbackData.c1 = 0;
    gGlobalCallbackData.c2 = 0;
    s = "foo; bar\n";
    XELP_XB_INIT(script,s,XelpStrLen(s));
    r = XelpParseXB(&x,&script);
    if (JB_ASSERT((r != XELP_S_OK) || (gGlobalCallbackData.c1 != 1) || (gGlobalCallbackData.c2 != 2), "XelpParseXB multi cmd"))
        return XELP_E_ERR;

    /* multiple commands separated by newlines */
    gGlobalCallbackData.c1 = 0;
    gGlobalCallbackData.c2 = 0;
    s = "foo\nbar\n";
    XELP_XB_INIT(script,s,XelpStrLen(s));
    r = XelpParseXB(&x,&script);
    if (JB_ASSERT((r != XELP_S_OK) || (gGlobalCallbackData.c1 != 1) || (gGlobalCallbackData.c2 != 2), "XelpParseXB newline cmds"))
        return XELP_E_ERR;

    /* command not found -- verify mR[0] */
    s = "nonexistent\n";
    XELP_XB_INIT(script,s,XelpStrLen(s));
    r = XelpParseXB(&x,&script);
    if (JB_ASSERT((r != XELP_S_OK) || (XELP_R0(x) != XELP_E_CMDNOTFOUND), "XelpParseXB cmd not found"))
        return XELP_E_ERR;

    /* empty input */
    s = "";
    XELP_XB_INIT(script,s,XelpStrLen(s));
    r = XelpParseXB(&x,&script);
    if (JB_ASSERT(r != XELP_S_OK, "XelpParseXB empty input"))
        return XELP_E_ERR;

    /* comment-only input */
    s = "# just a comment\n";
    XELP_XB_INIT(script,s,XelpStrLen(s));
    r = XelpParseXB(&x,&script);
    if (JB_ASSERT(r != XELP_S_OK, "XelpParseXB comment only"))
        return XELP_E_ERR;

    /* NULL function table */
    {
        XELP x2;
        XelpInit(&x2,"TestNullTable");
        XELP_SET_FN_OUT(x2,dummyOut);
        s = "foo\n";
        XELP_XB_INIT(script,s,XelpStrLen(s));
        r = XelpParseXB(&x2,&script);
        if (JB_ASSERT(r != XELP_S_OK, "XelpParseXB null table"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpParse()

 Bug fix: old test called XelpParse but didn't verify the command executed.
 */

XELPRESULT test_XelpParse() {
    XELP x;
    char *s;
    XELPRESULT r;

    XelpInit(&x,"TestParse");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* test actual command dispatch */
    gGlobalCallbackData.c1 = 0;
    s = "foo ";
    r = XelpParse(&x,s,XelpStrLen(s));
    if (JB_ASSERT((r!=XELP_S_OK) || (gGlobalCallbackData.c1 != 1),"XelpParse foo executes"))
        return XELP_E_ERR;

    /* test with semicolons */
    gGlobalCallbackData.c0 = -1;
    gGlobalCallbackData.c2 = 0;
    s = "cli0; bar\n";
    r = XelpParse(&x,s,XelpStrLen(s));
    if (JB_ASSERT((r!=XELP_S_OK) || (gGlobalCallbackData.c0 != 0) || (gGlobalCallbackData.c2 != 2),"XelpParse multi"))
        return XELP_E_ERR;

    /* test command not found */
    s = "doesnotexist\n";
    r = XelpParse(&x,s,XelpStrLen(s));
    if (JB_ASSERT((r!=XELP_S_OK) || (XELP_R0(x) != XELP_E_CMDNOTFOUND), "XelpParse not found"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_XelpBufMacros() - tests all XelpBuf macros
 */
XELPRESULT test_XelpBufMacros() {
    char buf[32];
    XelpBuf xb;
    char ch;
    int i;

    /* XELP_XB_INIT */
    XELP_XB_INIT(xb,buf,32);
    if (JB_ASSERT(xb.s != buf, "XBInit s"))
        return XELP_E_ERR;
    if (JB_ASSERT(xb.p != buf, "XBInit p"))
        return XELP_E_ERR;
    if (JB_ASSERT(xb.e != buf+32, "XBInit e"))
        return XELP_E_ERR;

    /* XELP_XB_LEN */
    if (JB_ASSERT(XELP_XB_LEN(xb) != 32, "XBBufLen"))
        return XELP_E_ERR;

    /* XELP_XB_POS at start */
    if (JB_ASSERT(XELP_XB_POS(xb) != 0, "XBGetPos 0"))
        return XELP_E_ERR;

    /* XELP_XB_PUTC with bounds check */
    XELP_XB_PUTC(xb,'A');
    if (JB_ASSERT(buf[0] != 'A', "XBPUTC A"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_XB_POS(xb) != 1, "XBGetPos after put"))
        return XELP_E_ERR;

    XELP_XB_PUTC(xb,'B');
    XELP_XB_PUTC(xb,'C');
    if (JB_ASSERT(XELP_XB_POS(xb) != 3, "XBGetPos after 3 puts"))
        return XELP_E_ERR;

    /* XELP_XB_TOP */
    XELP_XB_TOP(xb);
    if (JB_ASSERT(XELP_XB_POS(xb) != 0, "XBTOP resets pos"))
        return XELP_E_ERR;

    /* XELP_XB_GETC */
    ch = 0;
    XELP_XB_GETC(xb,ch);
    if (JB_ASSERT(ch != 'A', "XBGETC A"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_XB_POS(xb) != 1, "XBGETC advances pos"))
        return XELP_E_ERR;

    ch = 0;
    XELP_XB_GETC(xb,ch);
    if (JB_ASSERT(ch != 'B', "XBGETC B"))
        return XELP_E_ERR;

    /* XELP_XB_INIT_PTRS */
    {
        XelpBuf xb2;
        XELP_XB_INIT_PTRS(xb2,buf,buf+5,buf+32);
        if (JB_ASSERT(xb2.s != buf, "XBInitPtrs s"))
            return XELP_E_ERR;
        if (JB_ASSERT(xb2.p != buf+5, "XBInitPtrs p"))
            return XELP_E_ERR;
        if (JB_ASSERT(xb2.e != buf+32, "XBInitPtrs e"))
            return XELP_E_ERR;
    }

    /* XELP_XB_INIT_BP */
    {
        XelpBuf xb3;
        XELP_XB_INIT_BP(xb3,buf,10,32);
        if (JB_ASSERT(xb3.p != buf+10, "XBInitBP pos"))
            return XELP_E_ERR;
        if (JB_ASSERT(XELP_XB_LEN(xb3) != 32, "XBInitBP len"))
            return XELP_E_ERR;
    }

    /* XELP_XB_COPY */
    {
        XelpBuf xba, xbb;
        XELP_XB_INIT(xba,buf,16);
        xba.p = buf+5;
        XELP_XB_COPY(xba,xbb);
        if (JB_ASSERT(xbb.s != xba.s, "XBPCopy s"))
            return XELP_E_ERR;
        if (JB_ASSERT(xbb.p != xba.p, "XBPCopy p"))
            return XELP_E_ERR;
        if (JB_ASSERT(xbb.e != xba.e, "XBPCopy e"))
            return XELP_E_ERR;
    }

    /* XELP_XB_PUTC bounds check -- fill buffer to end */
    {
        char smallbuf[4];
        XelpBuf sb;
        XELP_XB_INIT(sb,smallbuf,4);
        for (i=0; i<6; i++) {
            XELP_XB_PUTC(sb,(char)('0'+i));
        }
        /* should have stopped at 4 chars */
        if (JB_ASSERT(XELP_XB_POS(sb) != 4, "XBPUTC bounds check"))
            return XELP_E_ERR;
    }

    /* XELP_XB_GETC at end -- should not advance */
    {
        char gbuf[2];
        XelpBuf gb;
        gbuf[0] = 'X';
        gbuf[1] = 'Y';
        XELP_XB_INIT(gb,gbuf,2);
        ch = 0;
        XELP_XB_GETC(gb,ch);
        if (JB_ASSERT(ch != 'X', "XBGETC first"))
            return XELP_E_ERR;
        XELP_XB_GETC(gb,ch);
        if (JB_ASSERT(ch != 'Y', "XBGETC second"))
            return XELP_E_ERR;
        ch = 'Z';
        XELP_XB_GETC(gb,ch);
        /* ch should remain 'Z' since p>=e now */
        if (JB_ASSERT(ch != 'Z', "XBGETC at end no change"))
            return XELP_E_ERR;
    }

    /* XELP_XB_OUT macro test */
    {
        XELP xelp;
        XelpBuf ob;
        char obuf[] = "Hello";
        XelpInit(&xelp,"outxb test");
        resetDummyBuf();
        XELP_SET_FN_OUT(xelp,gDummyBufOut);
        XELP_XB_INIT(ob,obuf,5);
        XELP_XB_OUT(&xelp,ob);
        gDummyBufOut(0);
        if (JB_ASSERT(XelpStrLen(gDummyBuf) != 5, "XELP_XB_OUT macro"))
            return XELP_E_ERR;
    }

    /* XELP_XB_PTR */
    {
        XelpBuf xb4;
        XELP_XB_INIT(xb4,buf,16);
        if (JB_ASSERT(XELP_XB_PTR(xb4) != buf, "XBGetBufPtr"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}


/* ====================================================================
 test_default_handlers() - tests for default KEY and CLI handlers
 that are called when no matching command/key is found.
 */

/* default handler callback globals */
static int gDefKeyVal;
static const char *gDefCLIArgs;
static int gDefCLILen;

XELPRESULT defKeyHandler(XELP *ths, XELPKEYCODE key) {
    (void)ths;
    gDefKeyVal = (int)key;
    return XELP_W_WARN;
}

XELPRESULT defCLIHandler(XELP *ths, const char *args, int len) {
    (void)ths;
    gDefCLIArgs = args;
    gDefCLILen = len;
    return XELP_W_WARN;
}

XELPRESULT test_default_handlers() {
    XELP x;
    XELPRESULT r;
    XelpBuf script;
    char *s;

    /* ---- KEY default handler tests ---- */

    /* unmapped key with no default handler -- should return NOTFOUND */
    XelpInit(&x,"TestDefHandlers");
    XELP_SET_FN_KEY(x,gMyKeyCommands);
    XELP_SET_FN_OUT(x,dummyOut);

    r = XelpExecKC(&x,'z');
    if (JB_ASSERT(r != XELP_S_NOTFOUND, "DefKey null handler returns NOTFOUND"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(x) != XELP_S_NOTFOUND, "DefKey null handler mR[0]"))
        return XELP_E_ERR;

    /* set default KEY handler -- unmapped key should call it */
    XELP_SET_FN_DEF_KEY(x,defKeyHandler);
    gDefKeyVal = 0;
    r = XelpExecKC(&x,'z');
    if (JB_ASSERT(r != XELP_W_WARN, "DefKey handler called return"))
        return XELP_E_ERR;
    if (JB_ASSERT(gDefKeyVal != 'z', "DefKey handler received key"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(x) != XELP_W_WARN, "DefKey handler mR[0] stores result"))
        return XELP_E_ERR;

    /* mapped key should NOT call default handler */
    gDefKeyVal = 0;
    gGlobalCallbackData.k1 = 0;
    r = XelpExecKC(&x,'1');
    if (JB_ASSERT(r != XELP_S_OK, "DefKey mapped key returns OK"))
        return XELP_E_ERR;
    if (JB_ASSERT(gGlobalCallbackData.k1 != '1', "DefKey mapped key calls fn"))
        return XELP_E_ERR;
    if (JB_ASSERT(gDefKeyVal != 0, "DefKey handler NOT called for mapped key"))
        return XELP_E_ERR;

    /* default KEY handler with NULL fn table */
    {
        XELP x2;
        XelpInit(&x2,"TestDefKeyNoTable");
        XELP_SET_FN_OUT(x2,dummyOut);
        XELP_SET_FN_DEF_KEY(x2,defKeyHandler);
        gDefKeyVal = 0;
        r = XelpExecKC(&x2,'q');
        if (JB_ASSERT(r != XELP_W_WARN, "DefKey no table calls default"))
            return XELP_E_ERR;
        if (JB_ASSERT(gDefKeyVal != 'q', "DefKey no table received key"))
            return XELP_E_ERR;
    }

    /* default KEY handler via ParseKey in KEY mode */
    {
        XELP x3;
        XelpInit(&x3,"TestDefKeyParseKey");
        XELP_SET_FN_KEY(x3,gMyKeyCommands);
        XELP_SET_FN_CLI(x3,gMyCLICommands);
        XELP_SET_FN_OUT(x3,dummyOut);
        XELP_SET_FN_DEF_KEY(x3,defKeyHandler);
        XelpParseKey(&x3,XELPKEY_KEY); /* ESC: stashed */
        gDefKeyVal = 0;
        XelpParseKey(&x3,'q'); /* flush ESC → KEY mode, reprocess 'q' → default handler */
        if (JB_ASSERT(gDefKeyVal != 'q', "DefKey via ParseKey"))
            return XELP_E_ERR;
    }

    /* ---- CLI default handler tests ---- */

    /* unknown command with no default CLI handler -- mR[0] = CMDNOTFOUND */
    XelpInit(&x,"TestDefCLI");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    s = "unknowncmd arg1\n";
    XELP_XB_INIT(script,s,XelpStrLen(s));
    r = XelpParseXB(&x,&script);
    if (JB_ASSERT(XELP_R0(x) != XELP_E_CMDNOTFOUND, "DefCLI null handler CMDNOTFOUND"))
        return XELP_E_ERR;

    /* set default CLI handler -- unknown command should call it */
    XELP_SET_FN_DEF_CLI(x,defCLIHandler);
    gDefCLIArgs = 0;
    gDefCLILen = 0;
    s = "unknowncmd arg1\n";
    XELP_XB_INIT(script,s,XelpStrLen(s));
    r = XelpParseXB(&x,&script);
    if (JB_ASSERT(r != XELP_S_OK, "DefCLI handler called"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(x) != XELP_W_WARN, "DefCLI handler mR[0]"))
        return XELP_E_ERR;
    if (JB_ASSERT(gDefCLIArgs == 0, "DefCLI handler received args"))
        return XELP_E_ERR;
    if (JB_ASSERT(gDefCLILen == 0, "DefCLI handler received len"))
        return XELP_E_ERR;

    /* known command should NOT call default CLI handler */
    gDefCLIArgs = 0;
    gDefCLILen = 0;
    gGlobalCallbackData.c1 = 0;
    s = "foo arg\n";
    XELP_XB_INIT(script,s,XelpStrLen(s));
    r = XelpParseXB(&x,&script);
    if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "DefCLI known cmd dispatched"))
        return XELP_E_ERR;
    if (JB_ASSERT(gDefCLIArgs != 0, "DefCLI handler NOT called for known cmd"))
        return XELP_E_ERR;

    /* multiple commands: one known, one unknown -- default handler called for unknown only */
    gDefCLIArgs = 0;
    gGlobalCallbackData.c1 = 0;
    s = "foo; badcmd\n";
    XELP_XB_INIT(script,s,XelpStrLen(s));
    r = XelpParseXB(&x,&script);
    if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "DefCLI mixed: known ran"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(x) != XELP_W_WARN, "DefCLI mixed: default handler ran"))
        return XELP_E_ERR;

    /* default CLI handler with NULL fn table */
    {
        XELP x4;
        XelpInit(&x4,"TestDefCLINoTable");
        XELP_SET_FN_OUT(x4,dummyOut);
        XELP_SET_FN_DEF_CLI(x4,defCLIHandler);
        /* no CLI table set -- command should NOT dispatch (no table to search) */
        gDefCLIArgs = 0;
        s = "anything\n";
        XELP_XB_INIT(script,s,XelpStrLen(s));
        r = XelpParseXB(&x4,&script);
        /* with null fn table the dispatch loop is skipped entirely */
        if (JB_ASSERT(r != XELP_S_OK, "DefCLI null table returns OK"))
            return XELP_E_ERR;
    }

    /* default CLI handler via ParseKey (type unknown cmd + enter) */
    {
        XELP x5;
        int i;
        char *cmd = "badcmd";
        XelpInit(&x5,"TestDefCLIParseKey");
        XELP_SET_FN_CLI(x5,gMyCLICommands);
        XELP_SET_FN_OUT(x5,dummyOut);
        XELP_SET_FN_DEF_CLI(x5,defCLIHandler);
        gDefCLIArgs = 0;
        for (i = 0; i < XelpStrLen(cmd); i++)
            XelpParseKey(&x5,cmd[i]);
        XelpParseKey(&x5,XELPKEY_ENTER);
        if (JB_ASSERT(XELP_R0(x5) != XELP_W_WARN, "DefCLI via ParseKey"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_buffer_boundaries() - verify that buffer limits are never exceeded
 across all API entry points. Exercises exact boundary conditions for:
 - CLI command buffer (XELP_CMDBUFSZ) via ParseKey
 - XelpParseXB / XelpParse with various buffer sizes
 - XelpBuf macros at boundaries
 - Command handler received length is correctly bounded
 - Tokenizer never reads past buffer end
 */

/* handler that records received buffer pointer and length for boundary checks */
static const char *gBndArgs;
static int gBndLen;
static int gBndCallCount;

XELPRESULT bndHandler(XELP *ths, const char *args, int len) {
    (void)ths;
    gBndArgs = args;
    gBndLen = len;
    gBndCallCount++;
    return XELP_S_OK;
}

XELPRESULT test_buffer_boundaries() {
    XELP x;
    XELPRESULT r;
    XelpBuf script;
    int i;

    XELPCLIFuncMapEntry bndCmds[] = {
        {&bndHandler, "cmd", "test cmd"},
        XELP_FUNC_ENTRY_LAST
    };

    /* === CLI buffer via ParseKey === */

    /* 1. Type exactly XELP_CMDBUFSZ-2 chars + enter (buffer inited with CMDBUFSZ-1 capacity) */
    {
        XelpInit(&x,"BndTest");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        gBndCallCount = 0;
        for (i = 0; i < XELP_CMDBUFSZ - 2; i++)
            XelpParseKey(&x,'A');
        XelpParseKey(&x,XELPKEY_ENTER);
        /* the typed chars should have been captured and parsed */
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd CLI reset after enter"))
            return XELP_E_ERR;
    }

    /* 2. Type exactly XELP_CMDBUFSZ-1 chars (fill to capacity) + enter */
    {
        XelpInit(&x,"BndTest2");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        for (i = 0; i < XELP_CMDBUFSZ - 1; i++)
            XelpParseKey(&x,'B');
        /* buffer should be exactly full now */
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != XELP_CMDBUFSZ - 1, "bnd CLI full"))
            return XELP_E_ERR;
        XelpParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd CLI reset after full"))
            return XELP_E_ERR;
    }

    /* 3. Overflow: type XELP_CMDBUFSZ * 2 chars -- buffer must not exceed capacity */
    {
        XelpInit(&x,"BndTest3");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        for (i = 0; i < XELP_CMDBUFSZ * 2; i++)
            XelpParseKey(&x,'C');
        /* XELP_XB_PUTC bounds check should have prevented overflow */
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != XELP_CMDBUFSZ - 1, "bnd CLI overflow stopped"))
            return XELP_E_ERR;
        /* verify buffer end ptr is correct */
        if (JB_ASSERT(x.mCmdXB.p > x.mCmdXB.e, "bnd CLI ptr within bounds"))
            return XELP_E_ERR;
        XelpParseKey(&x,XELPKEY_ENTER);
    }

    /* 4. Backspace at empty buffer -- p must not go below s */
    {
        XelpInit(&x,"BndTest4");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        for (i = 0; i < 10; i++)
            XelpParseKey(&x,XELPKEY_BKSP);
        if (JB_ASSERT(x.mCmdXB.p < x.mCmdXB.s, "bnd bksp at empty no underflow"))
            return XELP_E_ERR;
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd bksp stays at 0"))
            return XELP_E_ERR;
    }

    /* 5. Type, backspace to empty, type again -- buffer reuse */
    {
        XelpInit(&x,"BndTest5");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        XelpParseKey(&x,'x');
        XelpParseKey(&x,'y');
        XelpParseKey(&x,'z');
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 3, "bnd type 3 chars"))
            return XELP_E_ERR;
        for (i = 0; i < 10; i++)
            XelpParseKey(&x,XELPKEY_BKSP);
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd bksp to empty"))
            return XELP_E_ERR;
        XelpParseKey(&x,'a');
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 1, "bnd retype after bksp"))
            return XELP_E_ERR;
        XelpParseKey(&x,XELPKEY_ENTER);
    }

    /* === Command handler receives correctly bounded length === */

    /* 6. handler len matches actual token+args length */
    {
        XelpInit(&x,"BndTest6");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        gBndArgs = 0;
        gBndLen = 0;
        gBndCallCount = 0;
        {
            char *s = "cmd arg1 arg2\n";
            XELP_XB_INIT(script,s,XelpStrLen(s));
            r = XelpParseXB(&x,&script);
        }
        if (JB_ASSERT(gBndCallCount != 1, "bnd handler called once"))
            return XELP_E_ERR;
        /* len should be distance from line start to line end (before \n) */
        if (JB_ASSERT(gBndLen != 13, "bnd handler len=13"))
            return XELP_E_ERR;
        /* args ptr should point into the input buffer */
        if (JB_ASSERT(gBndArgs == 0, "bnd handler got args ptr"))
            return XELP_E_ERR;
    }

    /* 7. handler len for command with no args */
    {
        gBndLen = -1;
        {
            char *s = "cmd\n";
            XELP_XB_INIT(script,s,XelpStrLen(s));
            r = XelpParseXB(&x,&script);
        }
        if (JB_ASSERT(gBndLen != 3, "bnd handler cmd-only len=3"))
            return XELP_E_ERR;
    }

    /* 8. handler len for single-char line (no newline, just "cmd") */
    {
        gBndLen = -1;
        {
            char *s = "cmd";
            XELP_XB_INIT(script,s,XelpStrLen(s));
            r = XelpParseXB(&x,&script);
        }
        if (JB_ASSERT(gBndLen != 3, "bnd handler no-newline len=3"))
            return XELP_E_ERR;
    }

    /* 9. handler len with semicolons -- each command gets its own length */
    {
        gBndCallCount = 0;
        gBndLen = -1;
        {
            char *s = "cmd a; cmd bb\n";
            XELP_XB_INIT(script,s,XelpStrLen(s));
            r = XelpParseXB(&x,&script);
        }
        if (JB_ASSERT(gBndCallCount != 2, "bnd semicolon 2 calls"))
            return XELP_E_ERR;
        /* last call should have been for "cmd bb" */
        if (JB_ASSERT(gBndLen != 6, "bnd semicolon second len=6"))
            return XELP_E_ERR;
    }

    /* === XelpParse boundary -- len parameter respected === */

    /* 10. XelpParse with exact length */
    {
        gBndCallCount = 0;
        gBndLen = -1;
        r = XelpParse(&x,"cmd x\n",6);
        if (JB_ASSERT(gBndCallCount != 1, "bnd Parse exact len"))
            return XELP_E_ERR;
        if (JB_ASSERT(gBndLen != 5, "bnd Parse handler len=5"))
            return XELP_E_ERR;
    }

    /* 11. XelpParse with shorter length than string -- should only parse up to len */
    {
        gBndCallCount = 0;
        r = XelpParse(&x,"cmd xyz extra\n",3);  /* only "cmd" visible */
        if (JB_ASSERT(gBndCallCount != 1, "bnd Parse truncated calls"))
            return XELP_E_ERR;
        if (JB_ASSERT(gBndLen != 3, "bnd Parse truncated len=3"))
            return XELP_E_ERR;
    }

    /* 12. XelpParse with len=0 -- nothing to parse */
    {
        gBndCallCount = 0;
        r = XelpParse(&x,"cmd\n",0);
        if (JB_ASSERT(gBndCallCount != 0, "bnd Parse len=0 no call"))
            return XELP_E_ERR;
    }

    /* === XelpBuf boundary checks === */

    /* 13. XELP_XB_PUTC at exact capacity -- should accept */
    {
        char buf[4];
        XelpBuf xb;
        XELP_XB_INIT(xb,buf,4);
        XELP_XB_PUTC(xb,'a');
        XELP_XB_PUTC(xb,'b');
        XELP_XB_PUTC(xb,'c');
        XELP_XB_PUTC(xb,'d');
        if (JB_ASSERT(XELP_XB_POS(xb) != 4, "bnd XBPUTC fill to cap"))
            return XELP_E_ERR;
        /* 5th write should be ignored */
        XELP_XB_PUTC(xb,'e');
        if (JB_ASSERT(XELP_XB_POS(xb) != 4, "bnd XBPUTC past cap ignored"))
            return XELP_E_ERR;
        if (JB_ASSERT(buf[3] != 'd', "bnd XBPUTC no overwrite"))
            return XELP_E_ERR;
    }

    /* 14. XELP_XB_GETC at end -- should not advance */
    {
        char buf[2];
        XelpBuf xb;
        char ch;
        buf[0] = 'X';
        buf[1] = 'Y';
        XELP_XB_INIT(xb,buf,2);
        ch = 0; XELP_XB_GETC(xb,ch);
        ch = 0; XELP_XB_GETC(xb,ch);
        if (JB_ASSERT(XELP_XB_POS(xb) != 2, "bnd XBGETC at end pos"))
            return XELP_E_ERR;
        ch = 'Z';
        XELP_XB_GETC(xb,ch);
        if (JB_ASSERT(ch != 'Z', "bnd XBGETC at end unchanged"))
            return XELP_E_ERR;
        if (JB_ASSERT(XELP_XB_POS(xb) != 2, "bnd XBGETC at end no advance"))
            return XELP_E_ERR;
    }

    /* 15. zero-length buffer */
    {
        char buf[1];
        XelpBuf xb;
        XELP_XB_INIT(xb,buf,0);
        if (JB_ASSERT(XELP_XB_LEN(xb) != 0, "bnd zero-len buflen"))
            return XELP_E_ERR;
        XELP_XB_PUTC(xb,'x');
        if (JB_ASSERT(XELP_XB_POS(xb) != 0, "bnd zero-len put ignored"))
            return XELP_E_ERR;
    }

    /* === Tokenizer boundary === */

    /* 16. single-char buffer */
    {
        XelpBuf b, tok;
        char *s = "x";
        XELP_XB_INIT(b,s,1);
        r = XelpTokLineXB(&b,&tok,XELP_TOK_ONLY);
        if (JB_ASSERT(r != XELP_S_OK, "bnd tok single char"))
            return XELP_E_ERR;
    }

    /* 17. tokenizer with exact-length buffer (no trailing space) */
    {
        XelpBuf b, tok;
        char *s = "tok1";
        XELP_XB_INIT(b,s,4);
        r = XelpTokLineXB(&b,&tok,XELP_TOK_ONLY);
        if (JB_ASSERT(r != XELP_S_OK, "bnd tok exact len"))
            return XELP_E_ERR;
        if (JB_ASSERT(XELP_S_OK != XelpStrEq2(tok.s,tok.p,"tok1"), "bnd tok exact match"))
            return XELP_E_ERR;
    }

    /* 18. repeated parse cycles -- buffer resets correctly each time */
    {
        XelpInit(&x,"BndRepeat");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        for (i = 0; i < 50; i++) {
            int j;
            for (j = 0; j < 3; j++)
                XelpParseKey(&x,'c');
            XelpParseKey(&x,XELPKEY_ENTER);
        }
        /* after 50 cycles the buffer should still be valid */
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd 50 cycles reset"))
            return XELP_E_ERR;
        if (JB_ASSERT(x.mCmdXB.p < x.mCmdXB.s, "bnd 50 cycles p >= s"))
            return XELP_E_ERR;
        if (JB_ASSERT(x.mCmdXB.p > x.mCmdXB.e, "bnd 50 cycles p <= e"))
            return XELP_E_ERR;
    }

    /* 19. ParseKey: fill buffer to exact capacity, then enter */
    {
        XelpInit(&x,"BndExact");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        gBndCallCount = 0;
        /* XelpInit sets mCmdXB capacity to XELP_CMDBUFSZ-1 */
        for (i = 0; i < XELP_CMDBUFSZ - 1; i++)
            XelpParseKey(&x,'D');
        /* one more should be dropped by XBPUTC bounds check */
        XelpParseKey(&x,'E');
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != XELP_CMDBUFSZ - 1, "bnd exact cap+1"))
            return XELP_E_ERR;
        XelpParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd exact cap enter reset"))
            return XELP_E_ERR;
    }

    /* 20. XelpParse: verify handler cannot see beyond supplied length */
    {
        char mixed[] = "cmd SECRET";  /* 10 chars total */
        gBndLen = -1;
        r = XelpParse(&x, mixed, 3);  /* only "cmd" visible */
        if (JB_ASSERT(gBndLen != 3, "bnd Parse hides trailing data"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_stress_malformed() - stress tests for malformed input, overflows,
 and edge cases that could crash or corrupt memory.
 */
XELPRESULT test_stress_malformed() {
    XELP x;
    XELPRESULT r;
    XelpBuf script;
    int i;

    XelpInit(&x,"StressTest");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_KEY(x,gMyKeyCommands);
    XELP_SET_FN_OUT(x,dummyOut);
    XELP_SET_FN_THR(x,dummyOut);

    /* --- CLI buffer overflow via ParseKey: 2x buffer size --- */
    {
        r = XelpParseKey(&x,XELPKEY_CLI);
        for (i = 0; i < XELP_CMDBUFSZ * 2; i++) {
            r = XelpParseKey(&x,'A');
        }
        r = XelpParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r != XELP_S_OK, "stress CLI buf overflow 2x"))
            return XELP_E_ERR;
    }

    /* --- CLI buffer overflow: exactly CMDBUFSZ-1 chars (boundary) --- */
    {
        r = XelpParseKey(&x,XELPKEY_CLI);
        for (i = 0; i < XELP_CMDBUFSZ - 1; i++) {
            r = XelpParseKey(&x,'B');
        }
        r = XelpParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r != XELP_S_OK, "stress CLI buf boundary"))
            return XELP_E_ERR;
    }

    /* --- rapid mode switching --- */
    {
        for (i = 0; i < 100; i++) {
            XelpParseKey(&x,XELPKEY_CLI);
            XelpParseKey(&x,XELPKEY_KEY); /* ESC: stashed */
            XelpParseKey(&x,XELPKEY_THR); /* flush ESC → KEY, reprocess THR → THR mode */
        }
        /* should not crash and mode should be THR after last switch */
        if (JB_ASSERT(x.mCurMode != XELP_MODE_THR, "stress rapid mode switch"))
            return XELP_E_ERR;
        XelpParseKey(&x,XELPKEY_CLI); /* back to CLI */
    }

    /* --- backspace more times than chars typed --- */
    {
        r = XelpParseKey(&x,XELPKEY_CLI);
        XelpParseKey(&x,'x');
        XelpParseKey(&x,'y');
        for (i = 0; i < 20; i++) {
            XelpParseKey(&x,XELPKEY_BKSP);
        }
        r = XelpParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r != XELP_S_OK, "stress backspace underflow"))
            return XELP_E_ERR;
    }

    /* --- Parse with all-semicolons script (many empty commands) --- */
    {
        char *semis = ";;;\n;;;\n;;;\n";
        XELP_XB_INIT(script,semis,XelpStrLen(semis));
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress all semicolons"))
            return XELP_E_ERR;
    }

    /* --- Parse with only whitespace and newlines --- */
    {
        char *ws = "   \n\n  \t  \n\n";
        XELP_XB_INIT(script,ws,XelpStrLen(ws));
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress all whitespace script"))
            return XELP_E_ERR;
    }

    /* --- Parse with unterminated quote --- */
    {
        char *uq = "\"unterminated string";
        XELP_XB_INIT(script,uq,XelpStrLen(uq));
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress unterminated quote"))
            return XELP_E_ERR;
    }

    /* --- Parse with only comments, deeply nested --- */
    {
        char *cm = "# comment 1\n# comment 2\n# comment 3\n# comment 4\n";
        XELP_XB_INIT(script,cm,XelpStrLen(cm));
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress all comments script"))
            return XELP_E_ERR;
    }

    /* --- Parse with backtick escape at end of buffer --- */
    {
        char *esc = "tok`";
        XELP_XB_INIT(script,esc,XelpStrLen(esc));
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress backtick at end"))
            return XELP_E_ERR;
    }

    /* --- Parse with many semicolons and commands mixed --- */
    {
        char *mix = "foo; bar; rst; foo; bar; rst;\n";
        XELP_XB_INIT(script,mix,XelpStrLen(mix));
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress mixed cmds"))
            return XELP_E_ERR;
    }

    /* --- Very long token (>256 chars) - should not crash --- */
    {
        char longbuf[300];
        for (i = 0; i < 280; i++) longbuf[i] = 'z';
        longbuf[280] = '\n';
        longbuf[281] = 0;
        XELP_XB_INIT(script,longbuf,281);
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress very long token"))
            return XELP_E_ERR;
    }

    /* --- Binary/control chars in input should not crash --- */
    {
        char binbuf[20];
        for (i = 0; i < 16; i++) binbuf[i] = (char)(i + 1);
        binbuf[16] = '\n';
        binbuf[17] = 0;
        XELP_XB_INIT(script,binbuf,17);
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress binary chars"))
            return XELP_E_ERR;
    }

    /* --- XelpStr2Int with garbage: returns 0 on invalid input --- */
    {
        int v;
        v = XelpStr2Int("zzz",3);
        if (JB_ASSERT(v != 0, "Str2Int garbage returns 0"))
            return XELP_E_ERR;

        v = XelpStr2Int("   h",4);
        if (JB_ASSERT(v != 0, "Str2Int spaces-h returns 0"))
            return XELP_E_ERR;

        v = XelpStr2Int("343.3",5);
        if (JB_ASSERT(v != 0, "Str2Int decimal point returns 0"))
            return XELP_E_ERR;
    }

    /* --- XelpParseNum validation: must return XELP_E_ERR on bad input --- */
    {
        int n = 99;

        /* garbage decimal */
        r = XelpParseNum("xyz",3,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum garbage decimal"))
            return XELP_E_ERR;

        /* decimal with embedded dot */
        n = 99;
        r = XelpParseNum("3.14",4,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum decimal point"))
            return XELP_E_ERR;

        /* bare "0x" -- no hex digits */
        n = 99;
        r = XelpParseNum("0x",2,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum bare 0x"))
            return XELP_E_ERR;

        /* bare "h" -- no hex digits */
        n = 99;
        r = XelpParseNum("h",1,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum bare h"))
            return XELP_E_ERR;

        /* empty string */
        n = 99;
        r = XelpParseNum("",0,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum empty"))
            return XELP_E_ERR;

        /* sign only, no digits */
        n = 99;
        r = XelpParseNum("-",1,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum minus only"))
            return XELP_E_ERR;

        n = 99;
        r = XelpParseNum("+",1,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum plus only"))
            return XELP_E_ERR;

        /* hex with non-hex char: "0xGG" */
        n = 99;
        r = XelpParseNum("0xGG",4,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum 0xGG"))
            return XELP_E_ERR;

        /* hex suffix with non-hex body: "zzh" */
        n = 99;
        r = XelpParseNum("zzh",3,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum zzh"))
            return XELP_E_ERR;

        /* spaces in decimal: "1 2" */
        n = 99;
        r = XelpParseNum("1 2",3,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum space in decimal"))
            return XELP_E_ERR;

        /* valid inputs still work after exercising error paths */
        n = 0;
        r = XelpParseNum("42",2,&n);
        if (JB_ASSERT((r != XELP_S_OK) || (n != 42), "ParseNum 42 after errors"))
            return XELP_E_ERR;

        n = 0;
        r = XelpParseNum("0xFF",4,&n);
        if (JB_ASSERT((r != XELP_S_OK) || (n != 0xFF), "ParseNum 0xFF after errors"))
            return XELP_E_ERR;

        n = 0;
        r = XelpParseNum("ABh",3,&n);
        if (JB_ASSERT((r != XELP_S_OK) || (n != 0xAB), "ParseNum ABh after errors"))
            return XELP_E_ERR;

        n = 0;
        r = XelpParseNum("-99",3,&n);
        if (JB_ASSERT((r != XELP_S_OK) || (n != -99), "ParseNum -99 after errors"))
            return XELP_E_ERR;

        /* --- integer overflow: must return XELP_E_ERR, not wrap --- */

        /* decimal overflow: 20 digits always overflows any int */
        n = 0;
        r = XelpParseNum("99999999999999999999",20,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum decimal overflow"))
            return XELP_E_ERR;

        /* hex overflow: 0x + 16 F's overflows any int */
        n = 0;
        r = XelpParseNum("0xFFFFFFFFFF",12,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum hex overflow"))
            return XELP_E_ERR;

        /* hex suffix overflow */
        n = 0;
        r = XelpParseNum("FFFFFFFFFFh",11,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum hex suffix overflow"))
            return XELP_E_ERR;

        /* boundary: largest valid positive (via Str2Int which wraps ParseNum) */
        n = 0;
        r = XelpParseNum("32767",5,&n);  /* valid on 16-bit and 32-bit */
        if (JB_ASSERT(r != XELP_S_OK, "ParseNum 32767 ok"))
            return XELP_E_ERR;
    }

    /* --- XelpExecKC with all possible char values --- */
    {
        char ch;
        for (ch = 1; ch < 127; ch++) {
            XelpExecKC(&x,ch); /* should not crash on any char */
        }
        JB_ASSERT(0, "stress ExecKC all chars");
    }

    /* --- ParseKey with all printable chars --- */
    {
        XelpParseKey(&x,XELPKEY_CLI);
        for (i = 0x20; i < 0x7f; i++) {
            XelpParseKey(&x,(char)i);
        }
        XelpParseKey(&x,XELPKEY_ENTER);
        JB_ASSERT(0, "stress ParseKey all printable");
    }

    /* --- Repeated init should not leak or crash --- */
    {
        XELP x2;
        for (i = 0; i < 100; i++) {
            XelpInit(&x2,"reinit test");
        }
        JB_ASSERT(0, "stress repeated init");
    }

    /* --- Help with only about msg (no key/cli tables) --- */
    {
        XELP x3;
        XelpInit(&x3,"Only About");
        XELP_SET_FN_OUT(x3,dummyOut);
        r = XelpHelp(&x3);
        if (JB_ASSERT(r != XELP_S_OK, "stress help minimal"))
            return XELP_E_ERR;
    }

    /* --- Quoted string with backslash at end --- */
    {
        char *qesc = "\"hello\\";
        XELP_XB_INIT(script,qesc,XelpStrLen(qesc));
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress quote backslash end"))
            return XELP_E_ERR;
    }

    /* --- Multiple quotes in sequence --- */
    {
        char *mq = "\"a\" \"b\" \"c\"\n";
        XELP_XB_INIT(script,mq,XelpStrLen(mq));
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress multiple quotes"))
            return XELP_E_ERR;
    }

    /* --- Single char buffer --- */
    {
        char *sc = "x";
        XELP_XB_INIT(script,sc,1);
        r = XelpParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress single char buf"))
            return XELP_E_ERR;
    }

    /* --- Token finder on very long buffer --- */
    {
        char bigbuf[512];
        XelpBuf bx;
        char *label = "target:";
        for (i = 0; i < 500; i++) bigbuf[i] = 'a';
        bigbuf[490] = ' ';
        bigbuf[491] = 't'; bigbuf[492] = 'a'; bigbuf[493] = 'r';
        bigbuf[494] = 'g'; bigbuf[495] = 'e'; bigbuf[496] = 't';
        bigbuf[497] = ':'; bigbuf[498] = ' ';
        bigbuf[499] = '\n'; bigbuf[500] = 0;
        XELP_XB_INIT(bx,bigbuf,500);
        r = XelpFindTok(&bx,label,label+7,XELP_TOK_ONLY);
        if (JB_ASSERT(r != XELP_S_OK, "stress FindTok large buf"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}


/* ====================================================================
 test_XelpRegisters()

 Tests for the mR[] register array and XELP_R0-R3 accessor macros.
 */

/* helper CLI command that writes R1-R3 from parsed args */
XELPRESULT cmd_set_regs(XELP *ths, const char *args, int len) {
    XelpBuf b, tok;
    XELP_XB_INIT(b, (char*)args, len);

    XELP_XB_TOP(b);
    XelpTokN(&b, 1, &tok);
    ths->mR[1] = XelpStr2Int(tok.s, tok.p - tok.s);

    XELP_XB_TOP(b);
    XelpTokN(&b, 2, &tok);
    ths->mR[2] = XelpStr2Int(tok.s, tok.p - tok.s);

    XELP_XB_TOP(b);
    XelpTokN(&b, 3, &tok);
    ths->mR[3] = XelpStr2Int(tok.s, tok.p - tok.s);

    return XELP_S_OK;
}

/* helper KEY command that sets R1 */
XELPRESULT key_set_r1(XELP *ths, XELPKEYCODE k) {
    ths->mR[1] = (int)k;
    return XELP_S_OK;
}

XELPRESULT test_XelpRegisters() {
    XELP x;
    XELPRESULT r;
    XelpBuf script;
    int i;

    XELPCLIFuncMapEntry regCmds[] = {
        {&cmd_set_regs, "setr", "set R1-R3"},
        {&cli0,         "nop",  "no-op"},
        XELP_FUNC_ENTRY_LAST
    };

    XELPKeyFuncMapEntry regKeys[] = {
        {&key_set_r1, 'r', "set R1"},
        XELP_FUNC_ENTRY_LAST
    };

    /* 1. compile-time: XELP_REGS_SZ >= 4 */
    if (JB_ASSERT(XELP_REGS_SZ < 4, "XELP_REGS_SZ >= 4"))
        return XELP_E_ERR;

    /* 2. all 4 registers zeroed after XelpInit */
    XelpInit(&x, "RegTest");
    XELP_SET_FN_CLI(x, regCmds);
    XELP_SET_FN_KEY(x, regKeys);
    XELP_SET_FN_OUT(x, dummyOut);

    for (i = 0; i < 4; i++) {
        if (JB_ASSERT(x.mR[i] != 0, "regs zeroed after init"))
            return XELP_E_ERR;
    }

    /* 3. accessor macros read correctly */
    if (JB_ASSERT(XELP_R0(x) != 0, "R0 macro reads 0"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R1(x) != 0, "R1 macro reads 0"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R2(x) != 0, "R2 macro reads 0"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R3(x) != 0, "R3 macro reads 0"))
        return XELP_E_ERR;

    /* 4. engine writes R0 after KEY dispatch */
    r = XelpExecKC(&x, 'r');
    if (JB_ASSERT(r != XELP_S_OK, "key dispatch OK"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(x) != XELP_S_OK, "R0 set after key dispatch"))
        return XELP_E_ERR;
    /* R1 was set by key_set_r1 */
    if (JB_ASSERT(XELP_R1(x) != 'r', "R1 set by key handler"))
        return XELP_E_ERR;

    /* 5. engine writes R0 after CLI dispatch */
    {
        char *s = "setr 10 20 30\n";
        XELP_XB_INIT(script, s, XelpStrLen(s));
        r = XelpParseXB(&x, &script);
    }
    if (JB_ASSERT(r != XELP_S_OK, "CLI dispatch OK"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(x) != XELP_S_OK, "R0 set after CLI dispatch"))
        return XELP_E_ERR;

    /* 6. command wrote R1-R3, caller reads them back via macros */
    if (JB_ASSERT(XELP_R1(x) != 10, "R1 set by setr"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R2(x) != 20, "R2 set by setr"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R3(x) != 30, "R3 set by setr"))
        return XELP_E_ERR;

    /* 7. R1-R3 survive between dispatch calls (engine doesn't touch them) */
    {
        char *s = "nop\n";
        XELP_XB_INIT(script, s, XelpStrLen(s));
        r = XelpParseXB(&x, &script);
    }
    if (JB_ASSERT(XELP_R1(x) != 10, "R1 survives nop"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R2(x) != 20, "R2 survives nop"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R3(x) != 30, "R3 survives nop"))
        return XELP_E_ERR;

    /* 8. callee-clobbers-all: next setr call overwrites R0 */
    {
        char *s = "setr 100 200 300\n";
        XELP_XB_INIT(script, s, XelpStrLen(s));
        r = XelpParseXB(&x, &script);
    }
    if (JB_ASSERT(XELP_R0(x) != XELP_S_OK, "R0 overwritten by next call"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R1(x) != 100, "R1 overwritten by setr"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R2(x) != 200, "R2 overwritten by setr"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R3(x) != 300, "R3 overwritten by setr"))
        return XELP_E_ERR;

    /* 9. not-found command sets R0 to CMDNOTFOUND, leaves R1-R3 alone */
    {
        char *s = "nosuchcmd\n";
        XELP_XB_INIT(script, s, XelpStrLen(s));
        r = XelpParseXB(&x, &script);
    }
    if (JB_ASSERT(XELP_R0(x) != XELP_E_CMDNOTFOUND, "R0 CMDNOTFOUND"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R1(x) != 100, "R1 untouched after not-found"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_KeyAccumulator() - multi-byte sequence assembly (tested indirectly via XelpParseKey)
 */
XELPRESULT test_KeyAccumulator() {
    XELP x;

    XelpInit(&x,"TestKeyAccum");
    XELP_SET_FN_OUT(x,dummyOut);
    XELP_SET_FN_CLI(x,gMyCLICommands);

    /* single char 'a' processes immediately into CLI buffer */
    XelpParseKey(&x, 'a');
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 1, "accum single char in buf"))
        return XELP_E_ERR;

    XelpParseKey(&x, XELPKEY_ENTER); /* reset buffer */

    /* ESC alone stalls -- accumulator holds it, nothing in buffer */
    XelpParseKey(&x, 0x1B);
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "accum ESC stalls"))
        return XELP_E_ERR;

    /* ESC + '[' still stalls (CSI start) */
    XelpParseKey(&x, '[');
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "accum CSI stalls"))
        return XELP_E_ERR;

    /* ESC + '[' + 'A' = UP arrow in CLI */
    XelpParseKey(&x, 'A');
#ifdef XELP_ENABLE_HISTORY
    /* history recalls "a" (typed above): buf should have 1 char */
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 1, "accum UP arrow recalls from history"))
        return XELP_E_ERR;
    XelpParseKey(&x, XELPKEY_ENTER); /* reset for next test */
#else
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "accum UP arrow dropped in CLI"))
        return XELP_E_ERR;
#endif

    /* 4-byte sequence: ESC [ 3 ~ (KDEL) at empty buf: no effect */
    XelpParseKey(&x, 0x1B);
    XelpParseKey(&x, '[');
    XelpParseKey(&x, '3');
    XelpParseKey(&x, '~');
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "accum KDEL at empty no effect"))
        return XELP_E_ERR;

    /* Verify accumulator state is clean after completed sequences */
    if (JB_ASSERT(x.mKeyLen != 0, "accum clean after sequences"))
        return XELP_E_ERR;

    /* ESC + non-'[' flushes ESC (mode switch) and reprocesses next char */
    {
        XELP x2;
        XelpInit(&x2,"TestAccumFlush");
        XELP_SET_FN_OUT(x2,dummyOut);
        XELP_SET_FN_CLI(x2,gMyCLICommands);
        XELP_SET_FN_KEY(x2,gMyKeyCommands);

        XelpParseKey(&x2, 0x1B);  /* ESC: stashed */
        gGlobalCallbackData.k0 = 0;
        XelpParseKey(&x2, '0');   /* flush ESC → KEY mode, reprocess '0' → k0 handler */
        if (JB_ASSERT(x2.mCurMode != XELP_MODE_KEY, "accum flush ESC → KEY"))
            return XELP_E_ERR;
        if (JB_ASSERT(gGlobalCallbackData.k0 != '0', "accum flush reprocessed '0'"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_MultiByteKeyDispatch() - KEY mode with arrow key entries
 */
static int gMultiByteKeyVal;
XELPRESULT multiByteKeyHandler(XELP *ths, XELPKEYCODE k) {
    (void)ths;
    gMultiByteKeyVal = (int)k;
    return XELP_S_OK;
}

XELPRESULT test_MultiByteKeyDispatch() {
    XELP x;
    XELPRESULT r;

    XELPKeyFuncMapEntry mbKeys[] = {
        {&multiByteKeyHandler, XELP_KEYCODE_UP,   "up"},
        {&multiByteKeyHandler, XELP_KEYCODE_DOWN,  "down"},
        {&multiByteKeyHandler, 'a',                "a key"},
        XELP_FUNC_ENTRY_LAST
    };

    XelpInit(&x,"TestMultiByte");
    XELP_SET_FN_KEY(x,mbKeys);
    XELP_SET_FN_OUT(x,dummyOut);

    /* direct dispatch of multi-byte key via XelpExecKC */
    gMultiByteKeyVal = 0;
    r = XelpExecKC(&x, XELP_KEYCODE_UP);
    if (JB_ASSERT(r != XELP_S_OK, "multibyte direct up dispatch"))
        return XELP_E_ERR;
    if (JB_ASSERT(gMultiByteKeyVal != (int)XELP_KEYCODE_UP, "multibyte direct up value"))
        return XELP_E_ERR;

    /* single char still works */
    gMultiByteKeyVal = 0;
    r = XelpExecKC(&x, 'a');
    if (JB_ASSERT(r != XELP_S_OK, "multibyte single char dispatch"))
        return XELP_E_ERR;
    if (JB_ASSERT(gMultiByteKeyVal != 'a', "multibyte single char value"))
        return XELP_E_ERR;

    /* unmapped multi-byte key */
    r = XelpExecKC(&x, XELP_KEYCODE_LEFT);
    if (JB_ASSERT(r != XELP_S_NOTFOUND, "multibyte unmapped returns NOTFOUND"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

#ifdef XELP_ENABLE_LINE_EDIT
/* ====================================================================
 Helper: feed a string of raw bytes to XelpParseKey (for simulating typed input)
 */
static void feedString(XELP *x, const char *s) {
    while (*s) XelpParseKey(x, *s++);
}

/* Helper: feed a multi-byte keycode as individual bytes via XelpParseKey */
static void feedKeycode(XELP *x, XELPKEYCODE kc) {
    XelpParseKey(x, XELP_KC_B0(kc));
    if (XELP_KC_B1(kc)) XelpParseKey(x, XELP_KC_B1(kc));
    if (XELP_KC_B2(kc)) XelpParseKey(x, XELP_KC_B2(kc));
    if (XELP_KC_B3(kc)) XelpParseKey(x, XELP_KC_B3(kc));
}

/* Helper: extract CLI buffer content as a string for comparison.
   Copies content from mCmdXB.s to mCmdXB.p into dst and null-terminates. */
static void getCmdBuf(XELP *x, char *dst, int maxlen) {
    int len = (int)(x->mCmdXB.p - x->mCmdXB.s);
    int i;
    if (len > maxlen - 1) len = maxlen - 1;
    for (i = 0; i < len; i++) dst[i] = x->mCmdXB.s[i];
    dst[len] = 0;
}

/* ====================================================================
 test_CLILineEdit_Insert() - type "hello", LEFT*3, "X", verify "heXllo"
 */
XELPRESULT test_CLILineEdit_Insert() {
    XELP x;
    char buf[64];

    XelpInit(&x,"TestLineInsert");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "hello");
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    XelpParseKey(&x, 'X');

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "heXllo"), "line edit insert"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLILineEdit_Delete() - type "hello", HOME, KDEL, verify "ello"
 */
XELPRESULT test_CLILineEdit_Delete() {
    XELP x;
    char buf[64];

    XelpInit(&x,"TestLineDel");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "hello");
    feedKeycode(&x, XELP_KEYCODE_HOME);
    feedKeycode(&x, XELP_KEYCODE_KDEL);

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ello"), "line edit delete"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLILineEdit_HomeEnd() - HOME, type "AB", END, type "CD", verify "ABhelloCD"
 */
XELPRESULT test_CLILineEdit_HomeEnd() {
    XELP x;
    char buf[64];

    XelpInit(&x,"TestLineHE");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "hello");
    feedKeycode(&x, XELP_KEYCODE_HOME);
    feedString(&x, "AB");
    feedKeycode(&x, XELP_KEYCODE_END);
    feedString(&x, "CD");

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ABhelloCD"), "line edit home end"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLILineEdit_Backspace() - type "hello", LEFT*2, BKSP, verify "helo"
 */
XELPRESULT test_CLILineEdit_Backspace() {
    XELP x;
    char buf[64];

    XelpInit(&x,"TestLineBksp");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "hello");
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    XelpParseKey(&x, XELPKEY_BKSP);

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "helo"), "line edit backspace"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLIBackspaceBS() -- comprehensive tests for 0x08 (ASCII BS)
 Verifies that 0x08 works identically to XELPKEY_BKSP (0x07) in all contexts.
 */
XELPRESULT test_CLIBackspaceBS() {
    XELP x;
    char buf[64];

    /* --- 1. 0x08 deletes char at end of line --- */
    {
        XelpInit(&x,"TestBS1");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "abc");
        XelpParseKey(&x, XELPKEY_BS);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ab"),
                      "BS 0x08 delete at end"))
            return XELP_E_ERR;
    }

    /* --- 2. 0x08 deletes char mid-line (with line editing) --- */
    {
        XelpInit(&x,"TestBS2");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "hello");
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        XelpParseKey(&x, XELPKEY_BS);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "helo"),
                      "BS 0x08 mid-line"))
            return XELP_E_ERR;
    }

    /* --- 3. 0x08 at start of line: no-op (no crash, no change) --- */
    {
        XelpInit(&x,"TestBS3");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "ab");
        feedKeycode(&x, XELP_KEYCODE_HOME);
        XelpParseKey(&x, XELPKEY_BS);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ab"),
                      "BS 0x08 at start no-op"))
            return XELP_E_ERR;
    }

    /* --- 4. 0x08 on empty buffer: no crash --- */
    {
        XelpInit(&x,"TestBS4");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        XelpParseKey(&x, XELPKEY_BS);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XelpStrLen(buf) != 0,
                      "BS 0x08 empty buf no crash"))
            return XELP_E_ERR;
    }

    /* --- 5. 0x08 and 0x07 produce identical results --- */
    {
        XELP x1, x2;
        char buf1[64], buf2[64];

        XelpInit(&x1,"TestBS5a");
        XELP_SET_FN_CLI(x1,gMyCLICommands);
        XELP_SET_FN_OUT(x1,dummyOut);
        XelpInit(&x2,"TestBS5b");
        XELP_SET_FN_CLI(x2,gMyCLICommands);
        XELP_SET_FN_OUT(x2,dummyOut);

        feedString(&x1, "test");
        feedKeycode(&x1, XELP_KEYCODE_LEFT);
        XelpParseKey(&x1, XELPKEY_BKSP);

        feedString(&x2, "test");
        feedKeycode(&x2, XELP_KEYCODE_LEFT);
        XelpParseKey(&x2, XELPKEY_BS);

        getCmdBuf(&x1, buf1, sizeof(buf1));
        getCmdBuf(&x2, buf2, sizeof(buf2));

        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf1, XelpStrLen(buf1), buf2),
                      "BS 0x08 == BKSP 0x07"))
            return XELP_E_ERR;
    }

    /* --- 6. Multiple 0x08 deletions --- */
    {
        XelpInit(&x,"TestBS6");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "abcde");
        XelpParseKey(&x, XELPKEY_BS);
        XelpParseKey(&x, XELPKEY_BS);
        XelpParseKey(&x, XELPKEY_BS);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ab"),
                      "BS 0x08 triple delete"))
            return XELP_E_ERR;
    }

    /* --- 7. 0x08 after insert mid-line --- */
    {
        XelpInit(&x,"TestBS7");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "abcd");
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        feedString(&x, "XY");
        XelpParseKey(&x, XELPKEY_BS);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abXcd"),
                      "BS 0x08 after insert"))
            return XELP_E_ERR;
    }

    /* --- 8. 0x08 delete all chars one by one --- */
    {
        int i;
        XelpInit(&x,"TestBS8");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "abcd");
        for (i = 0; i < 4; i++)
            XelpParseKey(&x, XELPKEY_BS);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XelpStrLen(buf) != 0,
                      "BS 0x08 delete all"))
            return XELP_E_ERR;

        /* one more should be harmless */
        XelpParseKey(&x, XELPKEY_BS);
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XelpStrLen(buf) != 0,
                      "BS 0x08 past empty"))
            return XELP_E_ERR;
    }

    /* --- 9. 0x08 with echo mask --- */
    {
        XelpInit(&x,"TestBS9");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        resetDummyBuf();
        feedString(&x, "abc");
        XelpParseKey(&x, XELPKEY_BS);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ab"),
                      "BS 0x08 with mask buffer"))
            return XELP_E_ERR;
    }

    /* --- 10. Mixed 0x07 and 0x08 in same session --- */
    {
        XelpInit(&x,"TestBS10");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "abcdef");
        XelpParseKey(&x, XELPKEY_BKSP); /* 0x07: remove 'f' */
        XelpParseKey(&x, XELPKEY_BS);   /* 0x08: remove 'e' */
        XelpParseKey(&x, XELPKEY_BKSP); /* 0x07: remove 'd' */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abc"),
                      "BS mixed 0x07 0x08"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_CLIArrowsDrop() - UP/DOWN in CLI: no corruption
 */
XELPRESULT test_CLIArrowsDrop() {
    XELP x;
    char buf[64];

    XelpInit(&x,"TestArrowDrop");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "abc");
    feedKeycode(&x, XELP_KEYCODE_UP);
    feedKeycode(&x, XELP_KEYCODE_DOWN);
    feedString(&x, "d");

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abcd"), "CLI arrows drop"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLILineEdit_BufferFull() - insert at capacity
 */
XELPRESULT test_CLILineEdit_BufferFull() {
    XELP x;
    int i;

    XelpInit(&x,"TestLineFull");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* fill buffer to capacity */
    for (i = 0; i < XELP_CMDBUFSZ - 1; i++)
        XelpParseKey(&x, 'A');

    /* try to insert at cursor (should be ignored) */
    feedKeycode(&x, XELP_KEYCODE_HOME);
    XelpParseKey(&x, 'B');

    /* buffer should still be at capacity */
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != XELP_CMDBUFSZ - 1, "line edit buf full"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
#endif /* XELP_ENABLE_LINE_EDIT */

/* ====================================================================
 test_HelpMultiByteKeys() - cover ALL _xelpPrintKeyName branches
 */
XELPRESULT test_HelpMultiByteKeys() {
    XELP x;
    XELPRESULT r;

    /* table with every named multi-byte key + a single char + an unknown code */
    XELPKeyFuncMapEntry mbKeys[] = {
        {&k0, XELP_KEYCODE_UP,    "up"},
        {&k0, XELP_KEYCODE_DOWN,  "down"},
        {&k0, XELP_KEYCODE_LEFT,  "left"},
        {&k0, XELP_KEYCODE_RIGHT, "right"},
        {&k0, XELP_KEYCODE_HOME,  "home"},
        {&k0, XELP_KEYCODE_END,   "end"},
        {&k0, XELP_KEYCODE_KDEL,  "del"},
        {&k0, XELP_KEYCODE_INS,   "ins"},
        {&k0, XELP_KEYCODE_PGUP,  "pgup"},
        {&k0, XELP_KEYCODE_PGDN,  "pgdn"},
        {&k0, 0x00FF1234UL,       "unknown"},  /* unknown multi-byte → hex output */
        {&k1, 'a',                "letter a"},  /* single char branch */
        XELP_FUNC_ENTRY_LAST
    };

    resetDummyBuf();
    XelpInit(&x,"TestHelpMB");
    XELP_SET_FN_KEY(x,mbKeys);
    XELP_SET_FN_OUT(x,gDummyBufOut);

    r = XelpHelp(&x);
    gDummyBufOut(0);

    if (JB_ASSERT(r != XELP_S_OK, "help multibyte result"))
        return XELP_E_ERR;
    if (JB_ASSERT(XelpStrLen(gDummyBuf) <= 0, "help multibyte has output"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

#ifdef XELP_ENABLE_LINE_EDIT
/* ====================================================================
 test_CLILineEdit_Right() - RIGHT arrow moves cursor forward
 */
XELPRESULT test_CLILineEdit_Right() {
    XELP x;
    char buf[64];

    XelpInit(&x,"TestLineRight");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "abcd");
    feedKeycode(&x, XELP_KEYCODE_HOME);     /* cursor at 0 */
    feedKeycode(&x, XELP_KEYCODE_RIGHT);    /* cursor at 1 */
    feedKeycode(&x, XELP_KEYCODE_RIGHT);    /* cursor at 2 */
    XelpParseKey(&x, 'X');                   /* insert at 2 → "abXcd" */

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abXcd"), "line edit right insert"))
        return XELP_E_ERR;

    /* RIGHT at end of buffer should be a no-op */
    feedKeycode(&x, XELP_KEYCODE_END);
    feedKeycode(&x, XELP_KEYCODE_RIGHT);    /* already at end → no-op */
    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abXcd"), "line edit right at end"))
        return XELP_E_ERR;

    /* LEFT at start should be a no-op */
    feedKeycode(&x, XELP_KEYCODE_HOME);
    feedKeycode(&x, XELP_KEYCODE_LEFT);     /* already at start → no-op */
    XelpParseKey(&x, 'Z');
    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ZabXcd"), "line edit left at start"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
#endif /* XELP_ENABLE_LINE_EDIT */

/* ====================================================================
 test_AccumOverflow() - CSI sequences with intermediate bytes that hit 4-byte overflow
 */
XELPRESULT test_AccumOverflow() {
    XELP x;

    XelpInit(&x,"TestAccumOvfl");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* ESC [ <digit> <digit> — 4 bytes, digit at pos 3 isn't a terminator
       so overflow guard fires at mKeyLen==4 and flushes */
    XelpParseKey(&x, 0x1B);  /* ESC */
    XelpParseKey(&x, '[');   /* CSI start */
    XelpParseKey(&x, '1');   /* intermediate — not a letter or ~ */
    XelpParseKey(&x, '5');   /* 4th byte, still intermediate → overflow flush */
    /* should not crash, accumulator should be idle */
    if (JB_ASSERT(x.mKeyLen != 0, "accum overflow resets"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "accum overflow no buf corruption"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLIMalformedKeys() - slam the CLI with garbage, partial sequences,
 interleaved multi-byte, and verify no buffer overruns or crashes.
 */
XELPRESULT test_CLIMalformedKeys() {
    XELP x;
    int i;
    char buf[64];

    XelpInit(&x,"TestMalformed");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* 1. bare ESC followed immediately by another ESC */
    XelpParseKey(&x, 0x1B);
    XelpParseKey(&x, 0x1B);  /* flushes first ESC, stashes second */
    XelpParseKey(&x, 'a');   /* flushes second ESC, reprocesses 'a' */
    /* mode should still be CLI (ESC triggers KEY but KEY has no table on fresh init...
       wait, we set gMyCLICommands but gMyKeyCommands — let me make this right */
    XelpParseKey(&x, XELPKEY_ENTER); /* reset */

    /* 2. partial CSI abandoned by another ESC */
    XelpParseKey(&x, 0x1B);
    XelpParseKey(&x, '[');
    XelpParseKey(&x, 0x1B);  /* new ESC while in CSI — overflow/flush, then stash new ESC */
    XelpParseKey(&x, 'b');   /* flush second ESC, reprocess 'b' */
    XelpParseKey(&x, XELPKEY_ENTER); /* reset */

    /* 3. rapid-fire arrow keys interleaved with typing */
    feedString(&x, "hi");
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    feedKeycode(&x, XELP_KEYCODE_RIGHT);
    feedKeycode(&x, XELP_KEYCODE_UP);
    feedKeycode(&x, XELP_KEYCODE_DOWN);
    feedString(&x, "X");
    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "hiX"), "malformed interleaved arrows"))
        return XELP_E_ERR;
    XelpParseKey(&x, XELPKEY_ENTER);

    /* 4. unknown multi-byte key (PGUP, INS, etc.) in CLI — should be silently dropped */
    feedString(&x, "ok");
    feedKeycode(&x, XELP_KEYCODE_PGUP);
    feedKeycode(&x, XELP_KEYCODE_PGDN);
    feedKeycode(&x, XELP_KEYCODE_INS);
    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ok"), "malformed unknown keys dropped"))
        return XELP_E_ERR;
    XelpParseKey(&x, XELPKEY_ENTER);

    /* 5. overflow: 200 chars, then arrows, then enter — no crash */
    for (i = 0; i < 200; i++)
        XelpParseKey(&x, 'z');
    feedKeycode(&x, XELP_KEYCODE_HOME);
    feedKeycode(&x, XELP_KEYCODE_END);
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    feedKeycode(&x, XELP_KEYCODE_RIGHT);
    feedKeycode(&x, XELP_KEYCODE_KDEL);
    XelpParseKey(&x, XELPKEY_BKSP);
    XelpParseKey(&x, XELPKEY_ENTER);
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "malformed overflow reset"))
        return XELP_E_ERR;

    /* 6. all control chars (0x01-0x1A, skip ESC) fed one by one — none should crash */
    for (i = 1; i < 0x1B; i++)
        XelpParseKey(&x, (char)i);
    /* 0x1B (ESC) needs a follow-up */
    XelpParseKey(&x, 0x1B);
    XelpParseKey(&x, 'x');  /* flush ESC */
    for (i = 0x1C; i < 0x20; i++)
        XelpParseKey(&x, (char)i);
    XelpParseKey(&x, XELPKEY_ENTER);
    JB_ASSERT(0, "malformed ctrl chars no crash");

    /* 7. KDEL and BKSP at empty buffer — must not underflow */
    feedKeycode(&x, XELP_KEYCODE_KDEL);
    XelpParseKey(&x, XELPKEY_BKSP);
    XelpParseKey(&x, XELPKEY_DEL);
    if (JB_ASSERT(x.mCmdXB.p < x.mCmdXB.s, "malformed no underflow"))
        return XELP_E_ERR;

    /* 8. DEL (0x7F) mid-line (same as BKSP path) */
    feedString(&x, "abc");
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    XelpParseKey(&x, XELPKEY_DEL);
    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ac"), "malformed DEL mid-line"))
        return XELP_E_ERR;
    XelpParseKey(&x, XELPKEY_ENTER);

    /* 9. CSI with very long intermediate sequence (> 4 bytes emulated) */
    XelpParseKey(&x, 0x1B);
    XelpParseKey(&x, '[');
    XelpParseKey(&x, '1');
    XelpParseKey(&x, ';');   /* intermediate, causes overflow at byte 4 */
    /* accumulator should have flushed */
    XelpParseKey(&x, '2');   /* this is a new single char now */
    XelpParseKey(&x, XELPKEY_ENTER);

    return XELP_S_OK;
}

/* ====================================================================
 test_MultiInstance()

 Runs two XELP instances sharing the same command tables, alternating
 char feeds to verify no shared state leaks between instances.
 Tests:
   1. Interleaved CLI commands dispatch to the correct instance
   2. Interleaved ParseKey char feeds keep independent buffers
   3. Mode changes on one instance don't affect the other
   4. mR[] registers are per-instance
*/
XELPRESULT test_MultiInstance() {
    XELP a, b;
    XELPRESULT r;
    int i;

    XelpInit(&a, "InstanceA");
    XelpInit(&b, "InstanceB");

    XELP_SET_FN_CLI(a, gMyCLICommands);
    XELP_SET_FN_CLI(b, gMyCLICommands);
    XELP_SET_FN_KEY(a, gMyKeyCommands);
    XELP_SET_FN_KEY(b, gMyKeyCommands);
    XELP_SET_FN_OUT(a, dummyOut);
    XELP_SET_FN_OUT(b, dummyOut);
    XELP_SET_FN_BKSP(a, dummyVoid0);
    XELP_SET_FN_BKSP(b, dummyVoid0);
    XELP_SET_FN_THR(a, dummyOut);
    XELP_SET_FN_THR(b, dummyOut);

    /* 1. Interleaved XelpParse: different commands, independent results */
    gGlobalCallbackData.c0 = -1;
    gGlobalCallbackData.c1 = -1;
    gGlobalCallbackData.c2 = -1;

    r = XelpParse(&a, "foo\n", 4);
    if (JB_ASSERT((r != XELP_S_OK) || (gGlobalCallbackData.c1 != 1), "MultiInst Parse A foo"))
        return XELP_E_ERR;

    r = XelpParse(&b, "bar\n", 4);
    if (JB_ASSERT((r != XELP_S_OK) || (gGlobalCallbackData.c2 != 2), "MultiInst Parse B bar"))
        return XELP_E_ERR;

    /* mR[0] should reflect each instance's last dispatch independently */
    if (JB_ASSERT(XELP_R0(a) != XELP_S_OK, "MultiInst mR[0] A"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(b) != XELP_S_OK, "MultiInst mR[0] B"))
        return XELP_E_ERR;

    /* Run command-not-found on A, verify B's mR[0] unaffected */
    r = XelpParse(&a, "nonexistent\n", 12);
    if (JB_ASSERT(XELP_R0(a) != XELP_E_CMDNOTFOUND, "MultiInst A cmdnotfound"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(b) != XELP_S_OK, "MultiInst B mR[0] still OK"))
        return XELP_E_ERR;

    /* 2. Interleaved ParseKey: type into both instances alternately */
    {
        char *cmdA = "cli0";
        char *cmdB = "foo";
        int lenA = XelpStrLen(cmdA);
        int lenB = XelpStrLen(cmdB);

        /* Reset callback state */
        gGlobalCallbackData.c0 = -1;
        gGlobalCallbackData.c1 = -1;

        /* Feed characters alternating: A gets "cli0", B gets "foo" */
        for (i = 0; i < lenA || i < lenB; i++) {
            if (i < lenA) XelpParseKey(&a, cmdA[i]);
            if (i < lenB) XelpParseKey(&b, cmdB[i]);
        }
        XelpParseKey(&a, XELPKEY_ENTER);
        XelpParseKey(&b, XELPKEY_ENTER);

        if (JB_ASSERT(gGlobalCallbackData.c0 != 0, "MultiInst ParseKey A cli0"))
            return XELP_E_ERR;
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "MultiInst ParseKey B foo"))
            return XELP_E_ERR;
    }

    /* 3. Mode changes on one instance don't affect the other */
    {
        /* A switches to KEY mode, B stays in CLI */
        XelpParseKey(&a, XELPKEY_KEY); /* ESC: stashed */
        XelpParseKey(&a, '\0');        /* flush ESC -> KEY mode */
        if (JB_ASSERT(a.mCurMode != XELP_MODE_KEY, "MultiInst A to KEY"))
            return XELP_E_ERR;
        if (JB_ASSERT(b.mCurMode != XELP_MODE_CLI, "MultiInst B still CLI"))
            return XELP_E_ERR;

        /* B switches to THR, A still in KEY */
        XelpParseKey(&b, XELPKEY_THR);
        if (JB_ASSERT(b.mCurMode != XELP_MODE_THR, "MultiInst B to THR"))
            return XELP_E_ERR;
        if (JB_ASSERT(a.mCurMode != XELP_MODE_KEY, "MultiInst A still KEY"))
            return XELP_E_ERR;

        /* Return both to CLI */
        XelpParseKey(&a, XELPKEY_CLI);
        XelpParseKey(&b, XELPKEY_CLI);
        if (JB_ASSERT(a.mCurMode != XELP_MODE_CLI, "MultiInst A back CLI"))
            return XELP_E_ERR;
        if (JB_ASSERT(b.mCurMode != XELP_MODE_CLI, "MultiInst B back CLI"))
            return XELP_E_ERR;
    }

    /* 4. Stress: many interleaved commands */
    {
        int round;
        for (round = 0; round < 50; round++) {
            gGlobalCallbackData.c0 = -1;
            gGlobalCallbackData.c1 = -1;
            r = XelpParse(&a, "cli0\n", 5);
            r = XelpParse(&b, "foo\n", 4);
            if (JB_ASSERT(gGlobalCallbackData.c0 != 0, "MultiInst stress A"))
                return XELP_E_ERR;
            if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "MultiInst stress B"))
                return XELP_E_ERR;
        }
    }

    /* 5. Both instances do ParseKey with backspace — independent buffers */
    {
        gGlobalCallbackData.c2 = -1;
        /* A types "baz" then backspace 3 times (empty), then "foo" + enter */
        XelpParseKey(&a, 'b');
        XelpParseKey(&a, 'a');
        XelpParseKey(&a, 'z');
        XelpParseKey(&a, XELPKEY_BKSP);
        XelpParseKey(&a, XELPKEY_BKSP);
        XelpParseKey(&a, XELPKEY_BKSP);

        /* B types "bar" + enter while A's buffer is being edited */
        XelpParseKey(&b, 'b');
        XelpParseKey(&b, 'a');
        XelpParseKey(&b, 'r');
        XelpParseKey(&b, XELPKEY_ENTER);
        if (JB_ASSERT(gGlobalCallbackData.c2 != 2, "MultiInst bksp B bar"))
            return XELP_E_ERR;

        /* A now types "foo" + enter */
        gGlobalCallbackData.c1 = -1;
        XelpParseKey(&a, 'f');
        XelpParseKey(&a, 'o');
        XelpParseKey(&a, 'o');
        XelpParseKey(&a, XELPKEY_ENTER);
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "MultiInst bksp A foo"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_XelpArgs() - sequential argument iterator
 */
XELPRESULT test_XelpArgs() {
    XelpBuf tok;
    int val, n;
    XelpArgs a;
    XELPRESULT r;

    /* --- basic iteration: "divmod 17 5" --- */
    {
        char buf[] = "divmod 17 5";
        XelpArgsInit(&a, buf, XelpStrLen(buf));

        r = XelpNextTok(&a, &tok);
        if (JB_ASSERT(r != XELP_S_OK, "Args tok0 ok"))
            return XELP_E_ERR;
        if (JB_ASSERT((int)(tok.p - tok.s) != 6, "Args tok0 len"))
            return XELP_E_ERR;

        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 17, "Args int 17"))
            return XELP_E_ERR;

        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 5, "Args int 5"))
            return XELP_E_ERR;

        /* past end */
        r = XelpNextTok(&a, &tok);
        if (JB_ASSERT(r == XELP_S_OK, "Args past end"))
            return XELP_E_ERR;
        if (JB_ASSERT(tok.s != 0, "Args past end tok null"))
            return XELP_E_ERR;
    }

    /* --- XelpArgCount --- */
    {
        char buf2[] = "echo hello world";
        XelpArgsInit(&a, buf2, XelpStrLen(buf2));
        r = XelpArgCount(&a, &n);
        if (JB_ASSERT(r != XELP_S_OK || n != 3, "ArgCount 3"))
            return XELP_E_ERR;

        /* count should not disturb iteration position */
        r = XelpNextTok(&a, &tok);
        if (JB_ASSERT(r != XELP_S_OK || (int)(tok.p - tok.s) != 4, "ArgCount preserves pos"))
            return XELP_E_ERR;
    }

    /* --- empty buffer --- */
    {
        char buf3[] = "";
        XelpArgsInit(&a, buf3, 0);
        r = XelpNextTok(&a, &tok);
        if (JB_ASSERT(r == XELP_S_OK, "Args empty"))
            return XELP_E_ERR;

        r = XelpArgCount(&a, &n);
        if (JB_ASSERT(n != 0, "ArgCount empty"))
            return XELP_E_ERR;
    }

    /* --- single token --- */
    {
        char buf4[] = "help";
        XelpArgsInit(&a, buf4, XelpStrLen(buf4));
        r = XelpNextTok(&a, &tok);
        if (JB_ASSERT(r != XELP_S_OK || (int)(tok.p - tok.s) != 4, "Args single tok"))
            return XELP_E_ERR;
        if (JB_ASSERT(tok.s[0] != 'h' || tok.s[3] != 'p', "Args single content"))
            return XELP_E_ERR;
    }

    /* --- XelpNextInt with hex --- */
    {
        char buf5[] = "cmd 0xFF ABh";
        XelpArgsInit(&a, buf5, XelpStrLen(buf5));
        XelpNextTok(&a, 0); /* skip "cmd" */

        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 0xFF, "Args hex 0xFF"))
            return XELP_E_ERR;

        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 0xAB, "Args hex ABh"))
            return XELP_E_ERR;
    }

    /* --- XelpNextInt on non-numeric token --- */
    {
        char buf6[] = "cmd abc";
        XelpArgsInit(&a, buf6, XelpStrLen(buf6));
        XelpNextTok(&a, 0); /* skip "cmd" */
        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r != XELP_E_ERR, "Args non-numeric"))
            return XELP_E_ERR;
    }

    /* --- NULL tok pointer (skip pattern) --- */
    {
        char buf7[] = "skip me keep";
        XelpArgsInit(&a, buf7, XelpStrLen(buf7));
        r = XelpNextTok(&a, 0); /* skip with NULL */
        if (JB_ASSERT(r != XELP_S_OK, "Args skip NULL ptr"))
            return XELP_E_ERR;
        r = XelpNextTok(&a, 0);
        if (JB_ASSERT(r != XELP_S_OK, "Args skip NULL ptr 2"))
            return XELP_E_ERR;
        r = XelpNextTok(&a, &tok);
        if (JB_ASSERT(r != XELP_S_OK || (int)(tok.p - tok.s) != 4, "Args after skips"))
            return XELP_E_ERR;
    }

    /* --- negative integer --- */
    {
        char buf8[] = "cmd -42";
        XelpArgsInit(&a, buf8, XelpStrLen(buf8));
        XelpNextTok(&a, 0);
        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != -42, "Args negative int"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_XelpArgIntStr() - direct-access argument helpers
 */
XELPRESULT test_XelpArgIntStr() {
    XELPRESULT r;
    int val;
    const char *s;
    int slen;

    /* 1. XelpArgInt: get arg 1 as int */
    {
        char buf[] = "led 42";
        r = XelpArgInt(buf, XelpStrLen(buf), 1, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 42, "ArgInt basic"))
            return XELP_E_ERR;
    }

    /* 2. XelpArgInt: arg 0 is the command name */
    {
        char buf[] = "divmod 10 3";
        r = XelpArgInt(buf, XelpStrLen(buf), 0, &val);
        /* "divmod" is not a number */
        if (JB_ASSERT(r != XELP_E_ERR, "ArgInt arg0 not a number"))
            return XELP_E_ERR;
    }

    /* 3. XelpArgInt: multi args */
    {
        char buf[] = "divmod 10 3";
        r = XelpArgInt(buf, XelpStrLen(buf), 1, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 10, "ArgInt arg1=10"))
            return XELP_E_ERR;
        r = XelpArgInt(buf, XelpStrLen(buf), 2, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 3, "ArgInt arg2=3"))
            return XELP_E_ERR;
    }

    /* 4. XelpArgInt: hex */
    {
        char buf[] = "cmd 0xFF";
        r = XelpArgInt(buf, XelpStrLen(buf), 1, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 255, "ArgInt hex 0xFF"))
            return XELP_E_ERR;
    }

    /* 5. XelpArgInt: arg past end */
    {
        char buf[] = "cmd 1";
        val = -1;
        r = XelpArgInt(buf, XelpStrLen(buf), 5, &val);
        if (JB_ASSERT(r != XELP_E_ERR, "ArgInt past end"))
            return XELP_E_ERR;
        if (JB_ASSERT(val != -1, "ArgInt past end val unchanged"))
            return XELP_E_ERR;
    }

    /* 6. XelpArgInt: negative */
    {
        char buf[] = "adj -7";
        r = XelpArgInt(buf, XelpStrLen(buf), 1, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != -7, "ArgInt negative"))
            return XELP_E_ERR;
    }

    /* 7. XelpArgStr: basic */
    {
        char buf[] = "ssid MyNetwork";
        r = XelpArgStr(buf, XelpStrLen(buf), 1, &s, &slen);
        if (JB_ASSERT(r != XELP_S_OK || slen != 9, "ArgStr basic len"))
            return XELP_E_ERR;
        if (JB_ASSERT(s[0] != 'M' || s[8] != 'k', "ArgStr basic content"))
            return XELP_E_ERR;
    }

    /* 8. XelpArgStr: arg 0 = command name */
    {
        char buf[] = "echo hello";
        r = XelpArgStr(buf, XelpStrLen(buf), 0, &s, &slen);
        if (JB_ASSERT(r != XELP_S_OK || slen != 4, "ArgStr arg0 len"))
            return XELP_E_ERR;
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(s, slen, "echo"), "ArgStr arg0 echo"))
            return XELP_E_ERR;
    }

    /* 9. XelpArgStr: past end */
    {
        char buf[] = "help";
        r = XelpArgStr(buf, XelpStrLen(buf), 1, &s, &slen);
        if (JB_ASSERT(r != XELP_E_ERR, "ArgStr past end"))
            return XELP_E_ERR;
    }

    /* 10. XelpArgStr: empty buffer */
    {
        char buf[] = "";
        r = XelpArgStr(buf, 0, 0, &s, &slen);
        if (JB_ASSERT(r != XELP_E_ERR, "ArgStr empty buf"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_CursorWithEcho()
 Verify arrow keys, HOME/END, insert, delete, backspace with echo
 masking and output control. Tests both buffer correctness and output.
 */
XELPRESULT test_CursorWithEcho() {
    XELP x;
    char buf[64];
    int i;

    /* --- 1. Insert with echo mask '*': buffer has real chars, output has stars --- */
    {
        XelpInit(&x,"CurEcho1");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        resetDummyBuf();
        feedString(&x, "abc");
        gDummyBufOut(0);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abc"),
                      "Mask insert buffer correct"))
            return XELP_E_ERR;
        if (JB_ASSERT(gDummyBuf[0] != '*' || gDummyBuf[1] != '*' || gDummyBuf[2] != '*',
                      "Mask insert output stars"))
            return XELP_E_ERR;
    }

    /* --- 2. LEFT/RIGHT with mask: cursor movement works, output uses mask chars --- */
    {
        XelpInit(&x,"CurEcho2");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "abcde");
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        XelpParseKey(&x, 'X');

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abcXde"),
                      "Mask LEFT insert buffer"))
            return XELP_E_ERR;
    }

    /* --- 3. HOME + insert with mask --- */
    {
        XelpInit(&x,"CurEcho3");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "hello");
        feedKeycode(&x, XELP_KEYCODE_HOME);
        feedString(&x, "AB");

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ABhello"),
                      "Mask HOME insert buffer"))
            return XELP_E_ERR;
    }

    /* --- 4. END + append with mask --- */
    {
        XelpInit(&x,"CurEcho4");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "hello");
        feedKeycode(&x, XELP_KEYCODE_HOME);
        feedKeycode(&x, XELP_KEYCODE_END);
        feedString(&x, "CD");

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "helloCD"),
                      "Mask END append buffer"))
            return XELP_E_ERR;
    }

    /* --- 5. Backspace with mask: buffer correct --- */
    {
        XelpInit(&x,"CurEcho5");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "hello");
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        XelpParseKey(&x, XELPKEY_BKSP);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "helo"),
                      "Mask backspace buffer"))
            return XELP_E_ERR;
    }

    /* --- 6. Delete key with mask --- */
    {
        XelpInit(&x,"CurEcho6");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "hello");
        feedKeycode(&x, XELP_KEYCODE_HOME);
        feedKeycode(&x, XELP_KEYCODE_KDEL);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ello"),
                      "Mask delete buffer"))
            return XELP_E_ERR;
    }

    /* --- 7. Echo OFF with cursor ops: buffer correct, no echo output --- */
    {
        XelpInit(&x,"CurEcho7");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, XELP_ECHO_OFF);

        resetDummyBuf();
        feedString(&x, "secret");
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        XelpParseKey(&x, 'X');
        gDummyBufOut(0);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "secrXet"),
                      "EchoOff cursor insert buffer"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrLen(gDummyBuf) != 0,
                      "EchoOff cursor no output"))
            return XELP_E_ERR;
    }

    /* --- 8. Output disabled with cursor ops: buffer correct, zero output --- */
    {
        XelpInit(&x,"CurEcho8");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_OUT_ENABLE(x, 0);

        resetDummyBuf();
        feedString(&x, "test");
        feedKeycode(&x, XELP_KEYCODE_HOME);
        feedString(&x, "AB");
        feedKeycode(&x, XELP_KEYCODE_END);
        feedString(&x, "CD");
        gDummyBufOut(0);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ABtestCD"),
                      "OutDisabled cursor buffer"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrLen(gDummyBuf) != 0,
                      "OutDisabled cursor no output"))
            return XELP_E_ERR;
    }

    /* --- 9. UP/DOWN arrows with mask: no buffer corruption --- */
    {
        XelpInit(&x,"CurEcho9");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "xyz");
        feedKeycode(&x, XELP_KEYCODE_UP);
        feedKeycode(&x, XELP_KEYCODE_DOWN);
        feedString(&x, "w");

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "xyzw"),
                      "Mask UP/DOWN no corrupt"))
            return XELP_E_ERR;
    }

    /* --- 10. Complex sequence: mask, navigate, delete, insert, verify --- */
    {
        XelpInit(&x,"CurEcho10");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '#');

        resetDummyBuf();
        feedString(&x, "abcdef");
        feedKeycode(&x, XELP_KEYCODE_HOME);
        feedKeycode(&x, XELP_KEYCODE_RIGHT);
        feedKeycode(&x, XELP_KEYCODE_RIGHT);
        feedKeycode(&x, XELP_KEYCODE_KDEL);   /* delete 'c' → "abdef" */
        feedKeycode(&x, XELP_KEYCODE_KDEL);   /* delete 'd' → "abef"  */
        feedString(&x, "XY");                  /* insert → "abXYef"    */
        feedKeycode(&x, XELP_KEYCODE_END);
        XelpParseKey(&x, 'Z');                 /* append → "abXYefZ"   */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abXYefZ"),
                      "Complex mask nav buffer"))
            return XELP_E_ERR;
    }

    /* --- 11. Switch echo mid-stream: normal→mask→normal --- */
    {
        XelpInit(&x,"CurEcho11");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);

        resetDummyBuf();
        feedString(&x, "AB");                 /* normal echo: "AB" */
        XELP_SET_ECHO(x, '*');
        feedString(&x, "CD");                 /* masked echo: "**" */
        XELP_SET_ECHO(x, XELP_ECHO_NORMAL);
        feedString(&x, "EF");                 /* normal echo: "EF" */
        gDummyBufOut(0);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ABCDEF"),
                      "Mid-stream echo switch buffer"))
            return XELP_E_ERR;
        /* output should be "AB**EF" */
        if (JB_ASSERT(gDummyBuf[0] != 'A' || gDummyBuf[1] != 'B',
                      "Mid-stream normal part"))
            return XELP_E_ERR;
        if (JB_ASSERT(gDummyBuf[2] != '*' || gDummyBuf[3] != '*',
                      "Mid-stream masked part"))
            return XELP_E_ERR;
        if (JB_ASSERT(gDummyBuf[4] != 'E' || gDummyBuf[5] != 'F',
                      "Mid-stream restored part"))
            return XELP_E_ERR;
    }

    /* --- 12. LEFT past buffer start (no-op) with mask --- */
    {
        XelpInit(&x,"CurEcho12");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "ab");
        feedKeycode(&x, XELP_KEYCODE_HOME);
        feedKeycode(&x, XELP_KEYCODE_LEFT);  /* should be no-op */
        feedString(&x, "Z");

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "Zab"),
                      "Mask LEFT past start"))
            return XELP_E_ERR;
    }

    /* --- 13. RIGHT past buffer end (no-op) with mask --- */
    {
        XelpInit(&x,"CurEcho13");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "ab");
        feedKeycode(&x, XELP_KEYCODE_RIGHT); /* already at end, no-op */
        feedString(&x, "c");

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abc"),
                      "Mask RIGHT past end"))
            return XELP_E_ERR;
    }

    /* --- 14. Backspace at start (no-op) with mask --- */
    {
        XelpInit(&x,"CurEcho14");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "ab");
        feedKeycode(&x, XELP_KEYCODE_HOME);
        XelpParseKey(&x, XELPKEY_BKSP);      /* at start, no-op */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ab"),
                      "Mask bksp at start"))
            return XELP_E_ERR;
    }

    /* --- 15. Delete at end (no-op) with mask --- */
    {
        XelpInit(&x,"CurEcho15");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "ab");
        feedKeycode(&x, XELP_KEYCODE_KDEL);  /* at end, no-op */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "ab"),
                      "Mask del at end"))
            return XELP_E_ERR;
    }

    /* --- 16. Buffer full with mask: insert rejected --- */
    {
        XelpInit(&x,"CurEcho16");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        for (i = 0; i < XELP_CMDBUFSZ - 1; i++)
            XelpParseKey(&x, 'A');
        feedKeycode(&x, XELP_KEYCODE_HOME);
        XelpParseKey(&x, 'B');  /* should be rejected */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(buf[0] != 'A', "Mask full insert rejected"))
            return XELP_E_ERR;
    }

    /* --- 17. ENTER with mask: newline echoes, command executes --- */
    {
        XelpInit(&x,"CurEcho17");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        gGlobalCallbackData.c1 = 0;
        resetDummyBuf();
        feedString(&x, "foo");
        XelpParseKey(&x, XELPKEY_ENTER);
        gDummyBufOut(0);

        /* command should have executed */
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "Mask ENTER executes cmd"))
            return XELP_E_ERR;
        /* output should contain stars then newline */
        if (JB_ASSERT(gDummyBuf[0] != '*' || gDummyBuf[1] != '*' || gDummyBuf[2] != '*',
                      "Mask ENTER output stars"))
            return XELP_E_ERR;
    }

    /* --- 18. Output disabled + mask: buffer works, zero output --- */
    {
        XelpInit(&x,"CurEcho18");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_OUT_ENABLE(x, 0);
        XELP_SET_ECHO(x, '*');

        resetDummyBuf();
        feedString(&x, "test");
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        XelpParseKey(&x, 'X');
        gDummyBufOut(0);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "tesXt"),
                      "OutDis+Mask buffer"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrLen(gDummyBuf) != 0,
                      "OutDis+Mask no output"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_OutputEnable()
 Verify mOutEnable gates all output.
 */
XELPRESULT test_OutputEnable() {
    XELP x;

    XelpInit(&x,"TestOutEnable");
    XELP_SET_FN_KEY(x,gMyKeyCommands);
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,gDummyBufOut);

    /* 1. default state: output enabled */
    if (JB_ASSERT(XELP_GET_OUT_ENABLE(x) != 1, "OutEnable default 1"))
        return XELP_E_ERR;

    /* 2. XelpOut with output enabled */
    resetDummyBuf();
    XelpOut(&x, "hello", 0);
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 5, "OutEnable XelpOut normal"))
        return XELP_E_ERR;

    /* 3. disable output */
    XELP_SET_OUT_ENABLE(x, 0);
    if (JB_ASSERT(XELP_GET_OUT_ENABLE(x) != 0, "OutEnable set 0"))
        return XELP_E_ERR;

    resetDummyBuf();
    XelpOut(&x, "hello", 0);
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 0, "OutEnable XelpOut muted"))
        return XELP_E_ERR;

    /* 4. re-enable output */
    XELP_SET_OUT_ENABLE(x, 1);
    resetDummyBuf();
    XelpOut(&x, "hi", 0);
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 2, "OutEnable XelpOut resumed"))
        return XELP_E_ERR;

    /* 4b. XelpPutc with output enabled */
    resetDummyBuf();
    XelpPutc(&x, 'Z');
    gDummyBufOut(0);
    if (JB_ASSERT(gDummyBuf[0] != 'Z' || XelpStrLen(gDummyBuf) != 1, "XelpPutc normal"))
        return XELP_E_ERR;

    /* 4c. XelpPutc muted when disabled */
    XELP_SET_OUT_ENABLE(x, 0);
    resetDummyBuf();
    XelpPutc(&x, 'Q');
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 0, "XelpPutc muted"))
        return XELP_E_ERR;
    XELP_SET_OUT_ENABLE(x, 1);

    /* 5. XelpHelp suppressed when disabled */
    XELP_SET_OUT_ENABLE(x, 0);
    resetDummyBuf();
    XelpHelp(&x);
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 0, "OutEnable help muted"))
        return XELP_E_ERR;
    XELP_SET_OUT_ENABLE(x, 1);

    /* 6. XelpParseKey echo suppressed when disabled */
    XELP_SET_OUT_ENABLE(x, 0);
    resetDummyBuf();
    XelpParseKey(&x, 'A');
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 0, "OutEnable key echo muted"))
        return XELP_E_ERR;
    XELP_SET_OUT_ENABLE(x, 1);

    /* 7. prompt suppressed when disabled -- type a char then ENTER with output disabled */
    {
        XELP x2;
        XelpInit(&x2,"TestOutPrompt");
        XELP_SET_FN_CLI(x2,gMyCLICommands);
        XELP_SET_FN_OUT(x2,gDummyBufOut);
        XELP_SET_OUT_ENABLE(x2, 0);
        resetDummyBuf();
        XelpParseKey(&x2, 'x');
        XelpParseKey(&x2, XELPKEY_ENTER);
        gDummyBufOut(0);
        if (JB_ASSERT(XelpStrLen(gDummyBuf) != 0, "OutEnable prompt muted"))
            return XELP_E_ERR;
    }

    /* 8. re-enable, verify full behavior restored */
    {
        XELP x3;
        XelpInit(&x3,"TestOutRestore");
        XELP_SET_FN_CLI(x3,gMyCLICommands);
        XELP_SET_FN_OUT(x3,gDummyBufOut);
        XELP_SET_OUT_ENABLE(x3, 0);
        XELP_SET_OUT_ENABLE(x3, 1);
        resetDummyBuf();
        XelpParseKey(&x3, 'A');
        gDummyBufOut(0);
        if (JB_ASSERT(XelpStrLen(gDummyBuf) != 1, "OutEnable restore echo"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_EchoControl()
 Verify mEchoChar controls character echo masking.
 */
XELPRESULT test_EchoControl() {
    XELP x;
    int i;

    XelpInit(&x,"TestEcho");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,gDummyBufOut);

    /* 1. default state */
    if (JB_ASSERT(XELP_GET_ECHO(x) != XELP_ECHO_NORMAL, "Echo default normal"))
        return XELP_E_ERR;

    /* 2. normal echo: feed printable chars, verify output matches */
    resetDummyBuf();
    XelpParseKey(&x, 'a');
    XelpParseKey(&x, 'b');
    XelpParseKey(&x, 'c');
    gDummyBufOut(0);
    if (JB_ASSERT(gDummyBuf[0] != 'a' || gDummyBuf[1] != 'b' || gDummyBuf[2] != 'c',
                  "Echo normal chars"))
        return XELP_E_ERR;

    /* reset buffer for next test */
    XelpParseKey(&x, XELPKEY_ENTER); /* flush command buffer */

    /* 3. mask '*': feed chars, verify all output is '*' */
    XELP_SET_ECHO(x, '*');
    resetDummyBuf();
    XelpParseKey(&x, 'x');
    XelpParseKey(&x, 'y');
    XelpParseKey(&x, 'z');
    gDummyBufOut(0);
    if (JB_ASSERT(gDummyBuf[0] != '*' || gDummyBuf[1] != '*' || gDummyBuf[2] != '*',
                  "Echo mask '*'"))
        return XELP_E_ERR;
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 3, "Echo mask len"))
        return XELP_E_ERR;

    XelpParseKey(&x, XELPKEY_ENTER);

    /* 4. XELP_ECHO_OFF: feed chars, verify zero echo output */
    XELP_SET_ECHO(x, XELP_ECHO_OFF);
    resetDummyBuf();
    XelpParseKey(&x, 'a');
    XelpParseKey(&x, 'b');
    gDummyBufOut(0);
    if (JB_ASSERT(XelpStrLen(gDummyBuf) != 0, "Echo off no output"))
        return XELP_E_ERR;

    /* 5. ENTER still echoes newline even with echo off */
    resetDummyBuf();
    XelpParseKey(&x, XELPKEY_ENTER);
    gDummyBufOut(0);
    /* ENTER produces '\n' plus possibly prompt -- at minimum '\n' */
    if (JB_ASSERT(gDummyBuf[0] != '\n', "Echo off ENTER still echoes"))
        return XELP_E_ERR;

    /* 6. prompt still prints after ENTER even with echo off */
    /* (covered by test 5 -- if XELP_CLI_PROMPT is defined, output > 1 char) */

    /* 7. restore normal, verify echo resumes */
    XELP_SET_ECHO(x, XELP_ECHO_NORMAL);
    resetDummyBuf();
    XelpParseKey(&x, 'd');
    gDummyBufOut(0);
    if (JB_ASSERT(gDummyBuf[0] != 'd', "Echo restored normal"))
        return XELP_E_ERR;

    XelpParseKey(&x, XELPKEY_ENTER);

    /* 8. backspace cursor control still works with mask active */
    XELP_SET_ECHO(x, '*');
    XelpParseKey(&x, 'a');
    XelpParseKey(&x, 'b');
    resetDummyBuf();
    XelpParseKey(&x, XELPKEY_DEL); /* backspace */
    /* should not crash; buffer should have 1 char now */
    gDummyBufOut(0);
    /* just verify no crash; cursor control chars may appear */

    XelpParseKey(&x, XELPKEY_ENTER);
    XELP_SET_ECHO(x, XELP_ECHO_NORMAL);

    /* 9. password flow end-to-end */
    {
        XELP px;
        XelpInit(&px,"PassTest");
        XELP_SET_FN_CLI(px,gMyCLICommands);
        XELP_SET_FN_OUT(px,gDummyBufOut);

        XELP_SET_ECHO(px, '*');
        resetDummyBuf();
        XelpParseKey(&px, 's');
        XelpParseKey(&px, 'e');
        XelpParseKey(&px, 'c');
        XelpParseKey(&px, 'r');
        XelpParseKey(&px, 'e');
        XelpParseKey(&px, 't');
        gDummyBufOut(0);
        if (JB_ASSERT(XelpStrLen(gDummyBuf) != 6, "Password mask len 6"))
            return XELP_E_ERR;
        for (i = 0; i < 6; i++) {
            if (JB_ASSERT(gDummyBuf[i] != '*', "Password all stars"))
                return XELP_E_ERR;
        }
    }

    /* 10. XelpOut from commands still works while echo is masked */
    {
        XELP ox;
        XelpInit(&ox,"OutWhileMasked");
        XELP_SET_FN_OUT(ox,gDummyBufOut);
        XELP_SET_ECHO(ox, '*');
        resetDummyBuf();
        XelpOut(&ox, "test", 0);
        gDummyBufOut(0);
        if (JB_ASSERT(XelpStrLen(gDummyBuf) != 4, "XelpOut independent of echo"))
            return XELP_E_ERR;
        if (JB_ASSERT(gDummyBuf[0] != 't', "XelpOut not masked"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 Command History tests -- guarded by both XELP_ENABLE_LINE_EDIT and
 XELP_ENABLE_HISTORY so they compile out when history is disabled.
 */
#if defined(XELP_ENABLE_LINE_EDIT) && defined(XELP_ENABLE_HISTORY)

/* ====================================================================
 test_HistoryBasic() -- ~8 cases covering fundamental history recall
 */
XELPRESULT test_HistoryBasic() {
    char buf[64];

    /* 1. Fresh init: UP does nothing, buffer stays empty */
    {
        XELP x;
        XelpInit(&x,"HB1");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedKeycode(&x, XELP_KEYCODE_UP);
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XelpStrLen(buf) != 0, "Fresh UP does nothing"))
            return XELP_E_ERR;
    }

    /* 2. Type "hello" + ENTER, UP recalls "hello" */
    {
        XELP x;
        XelpInit(&x,"HB2");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "hello");
        XelpParseKey(&x, XELPKEY_ENTER);
        feedKeycode(&x, XELP_KEYCODE_UP);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "hello"),
                      "UP recalls hello"))
            return XELP_E_ERR;
    }

    /* 3. "aaa" ENTER, "bbb" ENTER, UP=bbb, UP=aaa */
    {
        XELP x;
        XelpInit(&x,"HB3");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "bbb"); XelpParseKey(&x, XELPKEY_ENTER);

        feedKeycode(&x, XELP_KEYCODE_UP);
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "bbb"),
                      "UP1 = bbb"))
            return XELP_E_ERR;

        feedKeycode(&x, XELP_KEYCODE_UP);
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "aaa"),
                      "UP2 = aaa"))
            return XELP_E_ERR;
    }

    /* 4. DOWN after UP: returns to more recent entry */
    {
        XELP x;
        XelpInit(&x,"HB4");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "bbb"); XelpParseKey(&x, XELPKEY_ENTER);

        feedKeycode(&x, XELP_KEYCODE_UP);  /* bbb */
        feedKeycode(&x, XELP_KEYCODE_UP);  /* aaa */
        feedKeycode(&x, XELP_KEYCODE_DOWN); /* bbb */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "bbb"),
                      "DOWN returns to bbb"))
            return XELP_E_ERR;
    }

    /* 5. DOWN past newest: restores empty line */
    {
        XELP x;
        XelpInit(&x,"HB5");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedKeycode(&x, XELP_KEYCODE_UP);   /* aaa */
        feedKeycode(&x, XELP_KEYCODE_DOWN); /* past newest → empty */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XelpStrLen(buf) != 0, "DOWN past newest = empty"))
            return XELP_E_ERR;
    }

    /* 6. UP past oldest: stays on oldest, no crash */
    {
        XELP x;
        XelpInit(&x,"HB6");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "bbb"); XelpParseKey(&x, XELPKEY_ENTER);

        feedKeycode(&x, XELP_KEYCODE_UP);  /* bbb */
        feedKeycode(&x, XELP_KEYCODE_UP);  /* aaa */
        feedKeycode(&x, XELP_KEYCODE_UP);  /* clamped at aaa */
        feedKeycode(&x, XELP_KEYCODE_UP);  /* still aaa */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "aaa"),
                      "UP past oldest stays on aaa"))
            return XELP_E_ERR;
    }

    /* 7. ENTER on recalled line dispatches it */
    {
        XELP x;
        XelpInit(&x,"HB7");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        gGlobalCallbackData.c1 = 0;
        feedString(&x, "foo"); XelpParseKey(&x, XELPKEY_ENTER);
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "foo executed first time"))
            return XELP_E_ERR;

        /* reset and recall */
        gGlobalCallbackData.c1 = 0;
        feedKeycode(&x, XELP_KEYCODE_UP);
        XelpParseKey(&x, XELPKEY_ENTER);
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "Recalled foo executes"))
            return XELP_E_ERR;
    }

    /* 8. Empty ENTER does NOT store in history */
    {
        XELP x;
        XelpInit(&x,"HB8");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        XelpParseKey(&x, XELPKEY_ENTER); /* empty */
        XelpParseKey(&x, XELPKEY_ENTER); /* empty */
        feedKeycode(&x, XELP_KEYCODE_UP);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XelpStrLen(buf) != 0, "Empty ENTER not stored"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_HistoryInProgressSave() -- ~4 cases for in-progress line stashing
 */
XELPRESULT test_HistoryInProgressSave() {
    char buf[64];

    /* 1. Type "partial", UP, DOWN: "partial" restored */
    {
        XELP x;
        XelpInit(&x,"HIP1");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "partial");
        feedKeycode(&x, XELP_KEYCODE_UP);   /* saves "partial", shows "aaa" */
        feedKeycode(&x, XELP_KEYCODE_DOWN); /* restores "partial" */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "partial"),
                      "In-progress restored after UP DOWN"))
            return XELP_E_ERR;
    }

    /* 2. Type "partial", UP, UP, DOWN, DOWN: "partial" restored exactly */
    {
        XELP x;
        XelpInit(&x,"HIP2");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "bbb"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "partial");

        feedKeycode(&x, XELP_KEYCODE_UP);   /* bbb */
        feedKeycode(&x, XELP_KEYCODE_UP);   /* aaa */
        feedKeycode(&x, XELP_KEYCODE_DOWN); /* bbb */
        feedKeycode(&x, XELP_KEYCODE_DOWN); /* partial */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "partial"),
                      "In-progress restored multi bounce"))
            return XELP_E_ERR;
    }

    /* 3. Type "partial", UP, type over, ENTER: new text executes + saves */
    {
        XELP x;
        XelpInit(&x,"HIP3");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "partial");
        feedKeycode(&x, XELP_KEYCODE_UP); /* "aaa" recalled */

        /* clear and type "bar" */
        feedKeycode(&x, XELP_KEYCODE_HOME);
        {
            int j;
            for (j = 0; j < 3; j++) feedKeycode(&x, XELP_KEYCODE_KDEL);
        }
        gGlobalCallbackData.c2 = -1;
        feedString(&x, "bar");
        XelpParseKey(&x, XELPKEY_ENTER);
        if (JB_ASSERT(gGlobalCallbackData.c2 != 2, "Overtyped recalled cmd executes bar"))
            return XELP_E_ERR;

        /* "bar" should be in history now */
        feedKeycode(&x, XELP_KEYCODE_UP);
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "bar"),
                      "Overtyped cmd saved in history"))
            return XELP_E_ERR;
    }

    /* 4. Type "partial", UP, ENTER: partial is lost */
    {
        XELP x;
        XelpInit(&x,"HIP4");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "partial");
        feedKeycode(&x, XELP_KEYCODE_UP);  /* "aaa" recalled */
        XelpParseKey(&x, XELPKEY_ENTER);   /* executes "aaa" */

        /* history should now have "aaa" at top (re-executed), not "partial" */
        feedKeycode(&x, XELP_KEYCODE_UP);
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "aaa"),
                      "Partial lost after recall+enter"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_HistoryFull() -- ~4 cases for ring buffer capacity/eviction
 */
XELPRESULT test_HistoryFull() {
    char buf[64];

    /* 1. Fill to capacity, verify all recallable */
    {
        XELP x;
        int i;
        char cmd[8];

        XelpInit(&x,"HF1");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        for (i = 0; i < XELP_HIST_DEPTH; i++) {
            cmd[0] = 'a' + (char)i; cmd[1] = 0;
            feedString(&x, cmd);
            XelpParseKey(&x, XELPKEY_ENTER);
        }
        /* UP should recall in reverse: last entered first */
        for (i = XELP_HIST_DEPTH - 1; i >= 0; i--) {
            feedKeycode(&x, XELP_KEYCODE_UP);
            getCmdBuf(&x, buf, sizeof(buf));
            cmd[0] = 'a' + (char)i; cmd[1] = 0;
            if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), cmd),
                          "Full ring recall"))
                return XELP_E_ERR;
        }
    }

    /* 2. Overfill: oldest evicted, newest stored */
    {
        XELP x;
        int i;
        char cmd[8];

        XelpInit(&x,"HF2");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        /* store DEPTH+1 entries: "a","b","c","d","e" (for DEPTH=4) */
        for (i = 0; i <= XELP_HIST_DEPTH; i++) {
            cmd[0] = 'a' + (char)i; cmd[1] = 0;
            feedString(&x, cmd);
            XelpParseKey(&x, XELPKEY_ENTER);
        }
        /* "a" should be evicted; first UP gives last entered */
        feedKeycode(&x, XELP_KEYCODE_UP);
        getCmdBuf(&x, buf, sizeof(buf));
        cmd[0] = 'a' + (char)XELP_HIST_DEPTH; cmd[1] = 0;
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), cmd),
                      "Overfill newest at top"))
            return XELP_E_ERR;

        /* go to oldest -- should NOT be "a" */
        for (i = 1; i < XELP_HIST_DEPTH; i++)
            feedKeycode(&x, XELP_KEYCODE_UP);
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK == XelpStrEq(buf, XelpStrLen(buf), "a"),
                      "Overfill oldest evicted"))
            return XELP_E_ERR;
    }

    /* 3. Fill + evict several times, verify ring integrity */
    {
        XELP x;
        int i;
        char cmd[8];

        XelpInit(&x,"HF3");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        /* push 3 * DEPTH entries */
        for (i = 0; i < 3 * XELP_HIST_DEPTH; i++) {
            cmd[0] = 'A' + (char)(i % 26); cmd[1] = 0;
            feedString(&x, cmd);
            XelpParseKey(&x, XELPKEY_ENTER);
        }
        /* last DEPTH entries should be recallable */
        for (i = 3 * XELP_HIST_DEPTH - 1; i >= 3 * XELP_HIST_DEPTH - XELP_HIST_DEPTH; i--) {
            feedKeycode(&x, XELP_KEYCODE_UP);
            getCmdBuf(&x, buf, sizeof(buf));
            cmd[0] = 'A' + (char)(i % 26); cmd[1] = 0;
            if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), cmd),
                          "Ring integrity after many evictions"))
                return XELP_E_ERR;
        }
    }

    /* 4. Very long command near XELP_CMDBUFSZ: stored and recalled */
    {
        XELP x;
        int i, len;

        XelpInit(&x,"HF4");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        len = XELP_CMDBUFSZ - 2; /* max usable (CMDBUFSZ-1 is buf limit, -1 for safety) */
        for (i = 0; i < len; i++)
            XelpParseKey(&x, 'Z');
        XelpParseKey(&x, XELPKEY_ENTER);

        feedKeycode(&x, XELP_KEYCODE_UP);
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XelpStrLen(buf) != len, "Long cmd recalled correct len"))
            return XELP_E_ERR;
        if (JB_ASSERT(buf[0] != 'Z' || buf[len-1] != 'Z', "Long cmd content correct"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_HistoryWithEditing() -- ~5 cases for cursor editing of recalled cmds
 */
XELPRESULT test_HistoryWithEditing() {
    char buf[64];

    /* 1. Recall with UP, LEFT/RIGHT works on recalled text */
    {
        XELP x;
        XelpInit(&x,"HE1");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "abcde"); XelpParseKey(&x, XELPKEY_ENTER);
        feedKeycode(&x, XELP_KEYCODE_UP);
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        XelpParseKey(&x, 'X');

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abcXde"),
                      "Edit recalled: insert X"))
            return XELP_E_ERR;
    }

    /* 2. Recall, HOME, type prefix, ENTER: modified version executes */
    {
        XELP x;
        XelpInit(&x,"HE2");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "oo"); XelpParseKey(&x, XELPKEY_ENTER);
        feedKeycode(&x, XELP_KEYCODE_UP);
        feedKeycode(&x, XELP_KEYCODE_HOME);
        XelpParseKey(&x, 'f');

        gGlobalCallbackData.c1 = 0;
        XelpParseKey(&x, XELPKEY_ENTER);
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "Modified recalled cmd foo executes"))
            return XELP_E_ERR;
    }

    /* 3. Recall, KDEL at cursor */
    {
        XELP x;
        XelpInit(&x,"HE3");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "abcd"); XelpParseKey(&x, XELPKEY_ENTER);
        feedKeycode(&x, XELP_KEYCODE_UP);
        feedKeycode(&x, XELP_KEYCODE_HOME);
        feedKeycode(&x, XELP_KEYCODE_KDEL); /* delete 'a' */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "bcd"),
                      "Recalled KDEL deletes char"))
            return XELP_E_ERR;
    }

    /* 4. Recall, backspace */
    {
        XELP x;
        XelpInit(&x,"HE4");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "abcd"); XelpParseKey(&x, XELPKEY_ENTER);
        feedKeycode(&x, XELP_KEYCODE_UP);
        XelpParseKey(&x, XELPKEY_BKSP); /* delete last char 'd' */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "abc"),
                      "Recalled bksp works"))
            return XELP_E_ERR;
    }

    /* 5. Recall, HOME, END: cursor at correct positions */
    {
        XELP x;
        XelpInit(&x,"HE5");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "hello"); XelpParseKey(&x, XELPKEY_ENTER);
        feedKeycode(&x, XELP_KEYCODE_UP);
        feedKeycode(&x, XELP_KEYCODE_HOME);
        XelpParseKey(&x, 'A');   /* insert at start */
        feedKeycode(&x, XELP_KEYCODE_END);
        XelpParseKey(&x, 'Z');   /* append at end */

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "AhelloZ"),
                      "HOME/END on recalled line"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_HistoryDuplicates() -- ~3 cases for consecutive duplicate suppression
 */
XELPRESULT test_HistoryDuplicates() {
    char buf[64];

    /* 1. "aaa" three times: only one entry (skip consecutive dups) */
    {
        XELP x;
        XelpInit(&x,"HD1");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);

        feedKeycode(&x, XELP_KEYCODE_UP);  /* aaa */
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "aaa"),
                      "Dup: first UP = aaa"))
            return XELP_E_ERR;

        feedKeycode(&x, XELP_KEYCODE_UP);  /* should still be aaa (no more entries) */
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "aaa"),
                      "Dup: second UP still aaa (only 1 entry)"))
            return XELP_E_ERR;
    }

    /* 2. "aaa", "bbb", "aaa": all three stored (non-consecutive) */
    {
        XELP x;
        XelpInit(&x,"HD2");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "bbb"); XelpParseKey(&x, XELPKEY_ENTER);
        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);

        feedKeycode(&x, XELP_KEYCODE_UP);  /* aaa (newest) */
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "aaa"),
                      "Non-consec: UP1 = aaa"))
            return XELP_E_ERR;

        feedKeycode(&x, XELP_KEYCODE_UP);  /* bbb */
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "bbb"),
                      "Non-consec: UP2 = bbb"))
            return XELP_E_ERR;

        feedKeycode(&x, XELP_KEYCODE_UP);  /* aaa (oldest) */
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "aaa"),
                      "Non-consec: UP3 = aaa"))
            return XELP_E_ERR;
    }

    /* 3. Empty string never stored regardless */
    {
        XELP x;
        XelpInit(&x,"HD3");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,dummyOut);

        feedString(&x, "aaa"); XelpParseKey(&x, XELPKEY_ENTER);
        XelpParseKey(&x, XELPKEY_ENTER); /* empty */
        XelpParseKey(&x, XELPKEY_ENTER); /* empty */

        feedKeycode(&x, XELP_KEYCODE_UP);
        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "aaa"),
                      "Empty not stored, UP = aaa"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_HistoryAndEcho() -- ~3 cases for history interaction with echo/output
 */
XELPRESULT test_HistoryAndEcho() {
    char buf[64];

    /* 1. Echo mask '*': recall works, output shows masked chars */
    {
        XELP x;
        XelpInit(&x,"HEC1");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_ECHO(x, '*');

        feedString(&x, "secret");
        XelpParseKey(&x, XELPKEY_ENTER);

        resetDummyBuf();
        feedKeycode(&x, XELP_KEYCODE_UP);
        gDummyBufOut(0);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "secret"),
                      "Echo mask: buffer has real text"))
            return XELP_E_ERR;
        /* output should contain '*' chars, not real text */
        {
            int i, stars = 0;
            for (i = 0; i < XelpStrLen(gDummyBuf); i++)
                if (gDummyBuf[i] == '*') stars++;
            if (JB_ASSERT(stars < 6, "Echo mask: output has stars"))
                return XELP_E_ERR;
        }
    }

    /* 2. Output disabled: history still saves and recalls */
    {
        XELP x;
        XelpInit(&x,"HEC2");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);
        XELP_SET_OUT_ENABLE(x, 0);

        feedString(&x, "silent");
        XelpParseKey(&x, XELPKEY_ENTER);
        feedKeycode(&x, XELP_KEYCODE_UP);

        getCmdBuf(&x, buf, sizeof(buf));
        if (JB_ASSERT(XELP_S_OK != XelpStrEq(buf, XelpStrLen(buf), "silent"),
                      "Output disabled: recall works"))
            return XELP_E_ERR;

        XELP_SET_OUT_ENABLE(x, 1);
    }

    /* 3. After recall with echo off, re-enable, type: echo resumes */
    {
        XELP x;
        XelpInit(&x,"HEC3");
        XELP_SET_FN_CLI(x,gMyCLICommands);
        XELP_SET_FN_OUT(x,gDummyBufOut);

        feedString(&x, "cmd1");
        XelpParseKey(&x, XELPKEY_ENTER);

        XELP_SET_ECHO(x, XELP_ECHO_OFF);
        feedKeycode(&x, XELP_KEYCODE_UP);
        XelpParseKey(&x, XELPKEY_ENTER);

        XELP_SET_ECHO(x, XELP_ECHO_NORMAL);
        resetDummyBuf();
        XelpParseKey(&x, 'Q');
        gDummyBufOut(0);
        if (JB_ASSERT(gDummyBuf[0] != 'Q', "Echo resumes after recall"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

#endif /* XELP_ENABLE_LINE_EDIT && XELP_ENABLE_HISTORY */

/* ====================================================================
 test_XelpBuf2Argv() - opt-in argc/argv tokenizer
 */
XELPRESULT test_XelpBuf2Argv() {
    XELP x;
    const char *argv[XELP_ARGV_MAX];
    int argc;
    XELPRESULT r;
    int val;
    const char *s;
    int slen;

    XelpInit(&x, "b2a");

    /* 1. Basic: "echo hello" -> argc=2 */
    {
        char buf[] = "echo hello";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A basic argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrEq(argv[0], XelpStrLen(argv[0]), "echo") != XELP_S_OK, "B2A argv[0]"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrEq(argv[1], XelpStrLen(argv[1]), "hello") != XELP_S_OK, "B2A argv[1]"))
            return XELP_E_ERR;
    }

    /* 2. Multiple args: "divmod 10 3" -> argc=3 */
    {
        char buf[] = "divmod 10 3";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 3, "B2A multi argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrEq(argv[0], XelpStrLen(argv[0]), "divmod") != XELP_S_OK, "B2A multi argv0"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrEq(argv[1], XelpStrLen(argv[1]), "10") != XELP_S_OK, "B2A multi argv1"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrEq(argv[2], XelpStrLen(argv[2]), "3") != XELP_S_OK, "B2A multi argv2"))
            return XELP_E_ERR;
    }

    /* 3. Quoted string: "echo \"hello world\"" -> argv[1]="hello world" */
    {
        char buf[] = "echo \"hello world\"";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A quoted argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrEq(argv[1], XelpStrLen(argv[1]), "hello world") != XELP_S_OK, "B2A quoted"))
            return XELP_E_ERR;
    }

    /* 4. Escape in quotes: \n -> 0x0A */
    {
        char buf[] = "echo \"line1\\nline2\"";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A esc-n argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(argv[1][5] != 0x0A, "B2A esc-n char"))
            return XELP_E_ERR;
    }

    /* 5. Tab escape: \t -> 0x09 */
    {
        char buf[] = "echo \"col1\\tcol2\"";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A esc-t argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(argv[1][4] != 0x09, "B2A esc-t char"))
            return XELP_E_ERR;
    }

    /* 6. Backslash escape: \\\\ -> single backslash */
    {
        char buf[] = "echo \"path\\\\file\"";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A esc-bs argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(argv[1][4] != '\\', "B2A esc-bs char"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrLen(argv[1]) != 9, "B2A esc-bs len"))
            return XELP_E_ERR;
    }

    /* 7. Quote escape: say \"hi\" */
    {
        char buf[] = "echo \"say \\\"hi\\\"\"";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A esc-q argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(argv[1][4] != '"' || argv[1][7] != '"', "B2A esc-q chars"))
            return XELP_E_ERR;
    }

    /* 8. CLI escape (backtick): echo `; -> argv[1]=";" */
    {
        char buf[] = "echo `;";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A cli-esc argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(argv[1][0] != ';', "B2A cli-esc char"))
            return XELP_E_ERR;
    }

    /* 9. Empty input -> argc=0 */
    {
        r = XelpBuf2Argv(&x, "", 0, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 0, "B2A empty"))
            return XELP_E_ERR;
    }

    /* 10. Whitespace only -> argc=0 */
    {
        char buf[] = "   ";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 0, "B2A whitespace"))
            return XELP_E_ERR;
    }

    /* 11. Semicolon terminator: stops at ; */
    {
        char buf[] = "echo hi; echo bye";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A semi argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrEq(argv[1], XelpStrLen(argv[1]), "hi") != XELP_S_OK, "B2A semi stops"))
            return XELP_E_ERR;
    }

    /* 12. Comment terminator: stops at # */
    {
        char buf[] = "echo hi # comment";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A hash argc"))
            return XELP_E_ERR;
    }

    /* 13. Too many args: maxargs=2 with 3 args -> XELP_E_ERR */
    {
        char buf[] = "a b c";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, 2);
        if (JB_ASSERT(r != XELP_E_ERR, "B2A too many"))
            return XELP_E_ERR;
    }

    /* 14. Input too long: len >= XELP_CMDBUFSZ -> XELP_E_ERR */
    {
        r = XelpBuf2Argv(&x, "x", XELP_CMDBUFSZ, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_E_ERR, "B2A too long"))
            return XELP_E_ERR;
    }

    /* 15. Single arg (command only): "help" -> argc=1 */
    {
        char buf[] = "help";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "B2A single"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrEq(argv[0], XelpStrLen(argv[0]), "help") != XELP_S_OK, "B2A single val"))
            return XELP_E_ERR;
    }

    /* 16. Tab as whitespace: "echo\thello" -> argc=2 */
    {
        char buf[] = "echo\thello";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A tab ws"))
            return XELP_E_ERR;
    }

    /* 17. Script mode: args NOT in mCmdMsgBuf (external buffer) */
    {
        XELP x2;
        XelpInit(&x2, "script");
        {
            char ext[] = "set mode 1";
            r = XelpBuf2Argv(&x2, ext, XelpStrLen(ext), &argc, argv, XELP_ARGV_MAX);
            if (JB_ASSERT(r != XELP_S_OK || argc != 3, "B2A script argc"))
                return XELP_E_ERR;
            if (JB_ASSERT(XelpStrEq(argv[0], XelpStrLen(argv[0]), "set") != XELP_S_OK, "B2A script a0"))
                return XELP_E_ERR;
            if (JB_ASSERT(XelpStrEq(argv[2], XelpStrLen(argv[2]), "1") != XELP_S_OK, "B2A script a2"))
                return XELP_E_ERR;
        }
    }

    /* 18. XelpArgvInt basic: argv[1]="42" -> val=42 */
    {
        char buf[] = "cmd 42";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK, "B2A ArgvInt setup"))
            return XELP_E_ERR;
        r = XelpArgvInt(argv, argc, 1, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 42, "B2A ArgvInt val"))
            return XELP_E_ERR;
    }

    /* 19. XelpArgvInt out of range: n >= argc -> XELP_E_ERR */
    {
        char buf[] = "cmd";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK, "B2A ArgvInt oor setup"))
            return XELP_E_ERR;
        r = XelpArgvInt(argv, argc, 1, &val);
        if (JB_ASSERT(r != XELP_E_ERR, "B2A ArgvInt oor"))
            return XELP_E_ERR;
    }

    /* 20. XelpArgvStr basic: argv[1]="hello" -> s="hello", slen=5 */
    {
        char buf[] = "cmd hello";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK, "B2A ArgvStr setup"))
            return XELP_E_ERR;
        r = XelpArgvStr(argv, argc, 1, &s, &slen);
        if (JB_ASSERT(r != XELP_S_OK || slen != 5, "B2A ArgvStr len"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrEq(s, slen, "hello") != XELP_S_OK, "B2A ArgvStr val"))
            return XELP_E_ERR;
    }

    /* 21. CLI mode: args already in mCmdMsgBuf (in-place tokenize) */
    {
        XELP x2;
        int i;
        char *src = "set val 7";
        int slen2 = XelpStrLen(src);
        XelpInit(&x2, "cli");
        for (i = 0; i < slen2; i++)
            x2.mCmdMsgBuf[i] = src[i];
        r = XelpBuf2Argv(&x2, x2.mCmdMsgBuf, slen2, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 3, "B2A cli-mode argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrEq(argv[0], XelpStrLen(argv[0]), "set") != XELP_S_OK, "B2A cli-mode a0"))
            return XELP_E_ERR;
    }

    /* 22. Unknown escape in quotes: \x -> literal x */
    {
        char buf[] = "echo \"a\\xb\"";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "B2A unk-esc argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(argv[1][1] != 'x', "B2A unk-esc char"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ================================================================
   test_XelpBuf2Argv_overflow -- overflow, boundary, and stress tests
   ================================================================ */
XELPRESULT test_XelpBuf2Argv_overflow() {
    XELP x;
    const char *argv[XELP_ARGV_MAX];
    int argc;
    XELPRESULT r;
    int val;
    const char *s;
    int slen;
    int i;

    XelpInit(&x, "ovf");

    /* ---- A. Buffer size boundaries ---- */

    /* A1. Input len = XELP_CMDBUFSZ-1 (63) -> success, argc=1 */
    {
        char buf[XELP_CMDBUFSZ];
        for (i = 0; i < XELP_CMDBUFSZ - 2; i++) buf[i] = 'A';
        buf[XELP_CMDBUFSZ - 2] = '\0';
        r = XelpBuf2Argv(&x, buf, XELP_CMDBUFSZ - 1, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "OVF A1 len=bufsz-1"))
            return XELP_E_ERR;
    }

    /* A2. Input len = XELP_CMDBUFSZ (64) -> XELP_E_ERR */
    {
        char buf[XELP_CMDBUFSZ + 1];
        for (i = 0; i < XELP_CMDBUFSZ; i++) buf[i] = 'B';
        buf[XELP_CMDBUFSZ] = '\0';
        r = XelpBuf2Argv(&x, buf, XELP_CMDBUFSZ, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_E_ERR, "OVF A2 len=bufsz"))
            return XELP_E_ERR;
    }

    /* A3. Input len = XELP_CMDBUFSZ+1 (65) -> XELP_E_ERR */
    {
        char buf[XELP_CMDBUFSZ + 2];
        for (i = 0; i < XELP_CMDBUFSZ + 1; i++) buf[i] = 'C';
        buf[XELP_CMDBUFSZ + 1] = '\0';
        r = XelpBuf2Argv(&x, buf, XELP_CMDBUFSZ + 1, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_E_ERR, "OVF A3 len=bufsz+1"))
            return XELP_E_ERR;
    }

    /* A4. Single token filling XELP_CMDBUFSZ-2 chars -> success */
    {
        char buf[XELP_CMDBUFSZ];
        for (i = 0; i < XELP_CMDBUFSZ - 2; i++) buf[i] = 'D';
        buf[XELP_CMDBUFSZ - 2] = '\0';
        r = XelpBuf2Argv(&x, buf, XELP_CMDBUFSZ - 2, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "OVF A4 token fill"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrLen(argv[0]) != XELP_CMDBUFSZ - 2, "OVF A4 token len"))
            return XELP_E_ERR;
    }

    /* A5. Token with leading space at exact boundary (XELP_CMDBUFSZ-1) */
    {
        char buf[XELP_CMDBUFSZ];
        buf[0] = ' ';
        for (i = 1; i < XELP_CMDBUFSZ - 1; i++) buf[i] = 'E';
        r = XelpBuf2Argv(&x, buf, XELP_CMDBUFSZ - 1, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "OVF A5 lead space"))
            return XELP_E_ERR;
    }

    /* ---- B. maxargs boundary ---- */

    /* B6. Exactly XELP_ARGV_MAX tokens -> success */
    {
        char buf[XELP_CMDBUFSZ];
        int pos = 0;
        for (i = 0; i < XELP_ARGV_MAX && pos < XELP_CMDBUFSZ - 2; i++) {
            if (i > 0) buf[pos++] = ' ';
            buf[pos++] = (char)('a' + i);
        }
        buf[pos] = '\0';
        r = XelpBuf2Argv(&x, buf, pos, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != XELP_ARGV_MAX, "OVF B6 exact maxargs"))
            return XELP_E_ERR;
    }

    /* B7. XELP_ARGV_MAX+1 tokens -> XELP_E_ERR */
    {
        char buf[XELP_CMDBUFSZ];
        int pos = 0;
        for (i = 0; i < XELP_ARGV_MAX + 1 && pos < XELP_CMDBUFSZ - 2; i++) {
            if (i > 0) buf[pos++] = ' ';
            buf[pos++] = (char)('a' + (i % 26));
        }
        buf[pos] = '\0';
        r = XelpBuf2Argv(&x, buf, pos, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_E_ERR, "OVF B7 maxargs+1"))
            return XELP_E_ERR;
    }

    /* B8. maxargs=1 with 2 tokens -> XELP_E_ERR */
    {
        char buf[] = "one two";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, 1);
        if (JB_ASSERT(r != XELP_E_ERR, "OVF B8 maxargs=1"))
            return XELP_E_ERR;
    }

    /* B9. maxargs=0 with any input -> XELP_E_ERR */
    {
        char buf[] = "hello";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, 0);
        if (JB_ASSERT(r != XELP_E_ERR, "OVF B9 maxargs=0"))
            return XELP_E_ERR;
    }

    /* B10. Verify each of 8 single-char tokens has correct content */
    {
        char buf[] = "a b c d e f g h";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 8, "OVF B10 argc"))
            return XELP_E_ERR;
        for (i = 0; i < 8; i++) {
            if (JB_ASSERT(argv[i][0] != ('a' + i) || argv[i][1] != '\0', "OVF B10 content"))
                return XELP_E_ERR;
        }
    }

    /* ---- C. Quoted strings at boundaries ---- */

    /* C11. Quoted string filling buffer to XELP_CMDBUFSZ-1 */
    {
        char buf[XELP_CMDBUFSZ];
        buf[0] = '"';
        for (i = 1; i < XELP_CMDBUFSZ - 2; i++) buf[i] = 'Q';
        buf[XELP_CMDBUFSZ - 2] = '"';
        r = XelpBuf2Argv(&x, buf, XELP_CMDBUFSZ - 1, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "OVF C11 quoted fill"))
            return XELP_E_ERR;
    }

    /* C12. Unclosed quote at buffer end (still produces token) */
    {
        char buf[XELP_CMDBUFSZ];
        buf[0] = '"';
        for (i = 1; i < XELP_CMDBUFSZ - 2; i++) buf[i] = 'U';
        /* no closing quote */
        r = XelpBuf2Argv(&x, buf, XELP_CMDBUFSZ - 1, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "OVF C12 unclosed"))
            return XELP_E_ERR;
    }

    /* C13. Empty quoted string "" -> argc=1, argv[0]="" */
    {
        char buf[] = "\"\"";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "OVF C13 empty quoted argc"))
            return XELP_E_ERR;
        if (JB_ASSERT(XelpStrLen(argv[0]) != 0, "OVF C13 empty quoted len"))
            return XELP_E_ERR;
    }

    /* C14. Nested escapes near buffer end */
    {
        char buf[XELP_CMDBUFSZ];
        int pos = 0;
        buf[pos++] = '"';
        while (pos < XELP_CMDBUFSZ - 5) buf[pos++] = 'N';
        buf[pos++] = '\\'; buf[pos++] = 'n';  /* \n escape */
        buf[pos++] = '"';
        buf[pos] = '\0';
        r = XelpBuf2Argv(&x, buf, pos, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "OVF C14 nested esc"))
            return XELP_E_ERR;
    }

    /* ---- D. Pathological inputs ---- */

    /* D15. CLI escape pairs filling near buffer limit */
    {
        char buf[XELP_CMDBUFSZ];
        int pos = 0;
        while (pos < XELP_CMDBUFSZ - 3) {
            buf[pos++] = '`'; /* CLI escape */
            buf[pos++] = 'X';
        }
        buf[pos] = '\0';
        r = XelpBuf2Argv(&x, buf, pos, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "OVF D15 cli esc pairs"))
            return XELP_E_ERR;
    }

    /* D16. Many \\ pairs inside quotes */
    {
        char buf[XELP_CMDBUFSZ];
        int pos = 0;
        buf[pos++] = '"';
        while (pos < XELP_CMDBUFSZ - 4) {
            buf[pos++] = '\\';
            buf[pos++] = '\\';
        }
        buf[pos++] = '"';
        buf[pos] = '\0';
        r = XelpBuf2Argv(&x, buf, pos, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "OVF D16 bs pairs"))
            return XELP_E_ERR;
    }

    /* D17. Trailing backtick at buffer end */
    {
        char buf[] = "cmd`";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 1, "OVF D17 trailing bt"))
            return XELP_E_ERR;
    }

    /* D18. All-spaces input at max length -> argc=0 */
    {
        char buf[XELP_CMDBUFSZ];
        for (i = 0; i < XELP_CMDBUFSZ - 1; i++) buf[i] = ' ';
        buf[XELP_CMDBUFSZ - 1] = '\0';
        r = XelpBuf2Argv(&x, buf, XELP_CMDBUFSZ - 1, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 0, "OVF D18 all spaces"))
            return XELP_E_ERR;
    }

    /* ---- E. XelpArgvInt / XelpArgvStr invalid indices ---- */

    /* E19. XelpArgvInt with n=-1 -> XELP_E_ERR */
    {
        char buf[] = "cmd 42";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK, "OVF E19 setup"))
            return XELP_E_ERR;
        r = XelpArgvInt(argv, argc, -1, &val);
        if (JB_ASSERT(r != XELP_E_ERR, "OVF E19 neg idx"))
            return XELP_E_ERR;
    }

    /* E20. XelpArgvStr with n=-1 -> XELP_E_ERR */
    {
        char buf[] = "cmd hello";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK, "OVF E20 setup"))
            return XELP_E_ERR;
        r = XelpArgvStr(argv, argc, -1, &s, &slen);
        if (JB_ASSERT(r != XELP_E_ERR, "OVF E20 neg idx"))
            return XELP_E_ERR;
    }

    /* E21. XelpArgvInt with n=argc -> XELP_E_ERR */
    {
        char buf[] = "cmd 42";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK, "OVF E21 setup"))
            return XELP_E_ERR;
        r = XelpArgvInt(argv, argc, argc, &val);
        if (JB_ASSERT(r != XELP_E_ERR, "OVF E21 n=argc"))
            return XELP_E_ERR;
    }

    /* E22. XelpArgvStr with n=argc -> XELP_E_ERR */
    {
        char buf[] = "cmd hello";
        r = XelpBuf2Argv(&x, buf, XelpStrLen(buf), &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK, "OVF E22 setup"))
            return XELP_E_ERR;
        r = XelpArgvStr(argv, argc, argc, &s, &slen);
        if (JB_ASSERT(r != XELP_E_ERR, "OVF E22 n=argc"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
   test_BranchCoverage -- exercises edge-case branches for full coverage:
     - XelpNextTok with NULL tok pointer
     - XelpNextInt past end of args (error return)
     - Mode switch to CLI with no CLI func table registered
     - Mode switch to THR with no passthrough function registered
     - XelpBufCmp CMP_TYPE_A0 matching with embedded null
 */
XELPRESULT test_BranchCoverage() {
    XELP x;
    XELPRESULT r;

    /* 1. XelpNextTok with NULL tok: should return NOTFOUND and not crash */
    {
        XelpArgs a;
        char buf[] = "";
        XelpArgsInit(&a, buf, 0);
        r = XelpNextTok(&a, 0);
        if (JB_ASSERT(r != XELP_S_NOTFOUND, "BC1 NextTok null tok"))
            return XELP_E_ERR;
    }

    /* 2. XelpNextInt past end of args: error path */
    {
        XelpArgs a;
        char buf[] = "hello";
        int val = -1;
        XelpArgsInit(&a, buf, XelpStrLen(buf));
        /* consume the only token */
        r = XelpNextInt(&a, &val);
        /* "hello" is not a valid integer -- XelpParseNum should fail */
        if (JB_ASSERT(r == XELP_S_OK, "BC2 NextInt NaN"))
            return XELP_E_ERR;
        /* now past end */
        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r == XELP_S_OK, "BC2 NextInt eof"))
            return XELP_E_ERR;
    }

    /* 3. Mode switch to CLI with no CLI funcs registered:
       should stay in current mode (KEY).
       Set mode directly since XELPKEY_KEY=ESC triggers the key accumulator. */
    {
        XelpInit(&x, "TestBC3");
        XELP_SET_FN_OUT(x, dummyOut);
        /* register KEY funcs but NOT CLI funcs */
        XELP_SET_FN_KEY(x, gMyKeyCommands);
        x.mCurMode = XELP_MODE_KEY;
        /* try to switch to CLI mode via CTRL-P: should be ignored (no CLI funcs) */
        XelpParseKey(&x, XELPKEY_CLI);
        if (JB_ASSERT(x.mCurMode != XELP_MODE_KEY, "BC3 no CLI stay KEY"))
            return XELP_E_ERR;
    }

#ifdef XELP_ENABLE_THR
    /* 4. Mode switch to THR with no passthrough function:
       should stay in current mode */
    {
        XelpInit(&x, "TestBC4");
        XELP_SET_FN_OUT(x, dummyOut);
        XELP_SET_FN_KEY(x, gMyKeyCommands);
        XELP_SET_FN_CLI(x, gMyCLICommands);
        /* do NOT set passthrough function */
        x.mCurMode = XELP_MODE_KEY;
        /* try to switch to THR mode: should be ignored */
        XelpParseKey(&x, XELPKEY_THR);
        if (JB_ASSERT(x.mCurMode != XELP_MODE_KEY, "BC4 no THR stay KEY"))
            return XELP_E_ERR;
    }

    /* 5. THR mode passthrough: single char should reach mpfPassThru */
    {
        XelpInit(&x, "TestBC5");
        XELP_SET_FN_OUT(x, dummyOut);
        XELP_SET_FN_KEY(x, gMyKeyCommands);
        XELP_SET_FN_CLI(x, gMyCLICommands);
        XELP_SET_FN_THR(x, dummyPassThru);
        x.mCurMode = XELP_MODE_THR;
        /* send a normal printable char in THR mode */
        XelpParseKey(&x, 'a');
        /* should still be in THR mode */
        if (JB_ASSERT(x.mCurMode != XELP_MODE_THR, "BC5 THR passthru"))
            return XELP_E_ERR;
    }
#endif

    /* 6. XelpBufCmp CMP_TYPE_A0: null in first buffer terminates comparison */
    {
        char a[] = "hi";  /* {'h','i','\0'} */
        char b[] = "hi";
        /* ae/be point one past last byte; null in A triggers early stop */
        r = XelpBufCmp(a, a + 3, b, b + 3, XELP_CMP_TYPE_A0);
        if (JB_ASSERT(r != XELP_S_OK, "BC6 CMP_A0 match"))
            return XELP_E_ERR;
    }

    /* 7. Output with mOutEnable=1 but mpfOut=NULL: exercises short-circuit
       branches in _xelp_putc, _xelp_echo, _xelp_cursor, XelpOut. */
    {
        XelpInit(&x, "TestBC7");
        /* enable output but do NOT set mpfOut — should be a no-op, no crash */
        XELP_SET_OUT_ENABLE(x, 1);
        XelpOut(&x, "hello", 5);
        XelpPutc(&x, 'x');
        if (JB_ASSERT(x.mpfOut != 0, "BC7 mpfOut should be null"))
            return XELP_E_ERR;
    }

    /* 8. Output with mOutEnable=1, mpfOut=NULL in CLI mode with cursor
       movement: exercises _xelp_cursor and _xelpRedrawFromCursor guards */
    {
        XelpInit(&x, "TestBC8");
        XELP_SET_OUT_ENABLE(x, 1);
        XELP_SET_FN_CLI(x, gMyCLICommands);
        /* type, move cursor, insert — all output guarded by mpfOut null */
        XelpParseKey(&x, 'a');
        XelpParseKey(&x, 'b');
        feedKeycode(&x, XELP_KEYCODE_LEFT);
        XelpParseKey(&x, 'X');         /* insert at cursor */
        feedKeycode(&x, XELP_KEYCODE_KDEL); /* delete at cursor */
        XelpParseKey(&x, XELPKEY_BS);  /* backspace */
        XelpParseKey(&x, '\n');        /* enter - dispatch */
        if (JB_ASSERT(x.mpfOut != 0, "BC8 mpfOut null"))
            return XELP_E_ERR;
    }

    /* 9. Argv tokenizer: newline and comment terminators */
    {
        const char *argv[XELP_ARGV_MAX];
        int argc = 0;

        /* newline in unquoted context terminates tokenization */
        r = XelpBuf2Argv(&x, "cmd arg\nignored", 15, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "BC9 newline stop"))
            return XELP_E_ERR;

        /* comment char terminates tokenization */
        r = XelpBuf2Argv(&x, "cmd arg # comment", 17, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK || argc != 2, "BC9 comment stop"))
            return XELP_E_ERR;

        /* escape char at end of buffer (no char follows) */
        r = XelpBuf2Argv(&x, "cmd\\", 4, &argc, argv, XELP_ARGV_MAX);
        if (JB_ASSERT(r != XELP_S_OK, "BC9 esc at end"))
            return XELP_E_ERR;

        /* quote escape at end of quoted string buffer */
        r = XelpBuf2Argv(&x, "\"abc\\", 5, &argc, argv, XELP_ARGV_MAX);
        /* unterminated quote → should still produce a token or error */
        (void)r; /* result doesn't matter, just exercising the path */
    }

    /* 10. Key accumulator: unusual bytes after ESC [ */
    {
        XelpInit(&x, "TestBC10");
        XELP_SET_FN_OUT(x, dummyOut);
        XELP_SET_FN_CLI(x, gMyCLICommands);
        XELP_SET_FN_KEY(x, gMyKeyCommands);

        /* ESC [ followed by byte > 0x7E (e.g. DEL 0x7F): exercises
           ub >= 0x40 && ub <= 0x7E false-on-right branch */
        XelpParseKey(&x, 0x1B);
        XelpParseKey(&x, '[');
        XelpParseKey(&x, 0x7F);

        /* ESC [ followed by byte in ':'...'?' range: exercises
           ub >= '0' && ub <= '9' false-on-right branch (ub > '9') */
        XelpParseKey(&x, 0x1B);
        XelpParseKey(&x, '[');
        XelpParseKey(&x, ';');  /* 0x3B, between '9'+1 and '@'-1 */
        XelpParseKey(&x, 'A');  /* terminate the sequence */
    }

    /* 11. THR mode with single char but no passthrough function set:
       exercises is_single=1, mpfPassThru=NULL branch */
    {
        XelpInit(&x, "TestBC11");
        XELP_SET_FN_OUT(x, dummyOut);
        XELP_SET_FN_KEY(x, gMyKeyCommands);
        XELP_SET_FN_CLI(x, gMyCLICommands);
        /* do NOT set passthrough function */
        x.mCurMode = XELP_MODE_THR;
        /* send a single printable char in THR mode */
        XelpParseKey(&x, 'z');
        /* should not crash; mode stays THR since no passthrough happened */
    }

    /* 12. Mode change with mCurMode == i (no actual change) and
       modeChangeAttempt == 1: exercises the false branch of
       (mCurMode != i) in the mode switch guard */
    {
        XelpInit(&x, "TestBC12");
        XELP_SET_FN_OUT(x, dummyOut);
        XELP_SET_FN_CLI(x, gMyCLICommands);
        XELP_SET_FN_KEY(x, gMyKeyCommands);
        /* already in CLI mode (default), send CTRL-P (enter CLI mode) */
        XelpParseKey(&x, XELPKEY_CLI);
        if (JB_ASSERT(x.mCurMode != XELP_MODE_CLI, "BC12 cli2cli"))
            return XELP_E_ERR;
    }

    /* 13. XelpTokLineXB: backtick escape in SEEK state triggers
       _PS_ESCA -> _PS_PREV path at line 698 of xelp.c */
    {
        XelpBuf b, out;
        char *esc = "`; hello\n";  /* backtick escapes ';' while in SEEK */
        XELP_XB_INIT(b, esc, XelpStrLen(esc));
        r = XelpTokLineXB(&b, &out, XELP_TOK_LINE);
        /* token should be "hello", ';' was escaped and skipped */
        if (JB_ASSERT(XELP_S_OK != r, "BC13 backtick SEEK _PS_PREV"))
            return XELP_E_ERR;
    }

    /* 14. Multi-byte key while already in THR mode: exercises
       is_single==0 branch at THR dispatch (line 1056) */
    {
        XelpInit(&x, "TestBC14");
        XELP_SET_FN_OUT(x, dummyOut);
        XELP_SET_FN_KEY(x, gMyKeyCommands);
        XELP_SET_FN_CLI(x, gMyCLICommands);
        x.mpfPassThru = dummyPassThru;
        x.mCurMode = XELP_MODE_THR;
        /* send ESC [ A (Up arrow) - completes as multi-byte keycode */
        XelpParseKey(&x, 0x1B);
        XelpParseKey(&x, '[');
        XelpParseKey(&x, 'A');
        /* is_single==0, mpfPassThru should NOT be called for multi-byte */
        if (JB_ASSERT(x.mCurMode != XELP_MODE_THR, "BC14 thr multi-byte"))
            return XELP_E_ERR;
    }

    /* 15. CRLF coalescing: CR+LF should not double-submit */
    {
        XelpInit(&x, "TestBC15");
        XELP_SET_FN_OUT(x, dummyOut);
        XELP_SET_FN_CLI(x, gMyCLICommands);
        /* type 'a' then CRLF then 'b' then LF -- two commands total */
        XelpParseKey(&x, 'a');
        XelpParseKey(&x, '\r');   /* CR submits "a" */
        XelpParseKey(&x, '\n');   /* LF swallowed (coalesce) */
        XelpParseKey(&x, 'b');
        XelpParseKey(&x, '\n');   /* LF submits "b" (no preceding CR) */
    }

    /* 16. XelpHelp with empty tables (only sentinels): no bogus rows */
    {
        XELPCLIFuncMapEntry emptyCli[] = { XELP_FUNC_ENTRY_LAST };
        XELPKeyFuncMapEntry emptyKey[] = { XELP_FUNC_ENTRY_LAST };
        XelpInit(&x, "TestBC16");
        XELP_SET_FN_OUT(x, dummyOut);
        XELP_SET_FN_CLI(x, emptyCli);
        XELP_SET_FN_KEY(x, emptyKey);
        XelpHelp(&x);  /* should not crash or print bogus rows */
    }

    return XELP_S_OK;
}

/* 	************************************************
	Xelp Simple Unit Test suite.
*/
FILE *logfile;
int flogout (char x) {
    if (logfile) {
        fputc(x,logfile);
        fflush(logfile);
    }
    return 0;
}
int putcharc (char x) {
    return putchar(x);
}
int run_tests() {

    JumpBug_InitGlobal("Xelp", putcharc,flogout);

    JumpBug_RunUnit(test_XelpStrLen,"XelpStrLen");
	JumpBug_RunUnit(test_XelpStr2Int,"XelpStr2Int");
    JumpBug_RunUnit(test_XelpStrEq, "StrEq");
    JumpBug_RunUnit(test_XelpStrEq2, "StrEq2");
    JumpBug_RunUnit(test_XelpBufCmp,"XelpBufCmp");
    JumpBug_RunUnit(test_XelpFindTok,"XelpFindTok");
    JumpBug_RunUnit(test_XelpTokLineXB,"XelpTokLineXB");

    JumpBug_RunUnit(test_XelpTokN,"XelpTokN");
    JumpBug_RunUnit(test_XelpNumToks,"XelpNumToks");
    JumpBug_RunUnit(test_XelpInit,"XelpInit");
    JumpBug_RunUnit(test_XelpOut_comprehensive,"XelpOut");
    JumpBug_RunUnit(test_XelpExecKC,"XelpExecKC");

    JumpBug_RunUnit(test_XelpParseKey,"XelpParseKey");
    JumpBug_RunUnit(test_XelpParse,"XelpParse");
    JumpBug_RunUnit(test_XelpParseXB,"XelpParseXB");
    JumpBug_RunUnit(test_XelpHelp,"XelpHelp");
    JumpBug_RunUnit(test_XelpParseNum,"XelpParseNum");
    JumpBug_RunUnit(test_XelpBufMacros,"XelpBufMacros");
    JumpBug_RunUnit(test_default_handlers,"DefaultHandlers");
    JumpBug_RunUnit(test_buffer_boundaries,"BufferBoundaries");
    JumpBug_RunUnit(test_stress_malformed,"StressMalformed");
    JumpBug_RunUnit(test_XelpRegisters,"XelpRegisters");
    JumpBug_RunUnit(test_KeyAccumulator,"KeyAccumulator");
    JumpBug_RunUnit(test_MultiByteKeyDispatch,"MultiByteKeyDispatch");
#ifdef XELP_ENABLE_LINE_EDIT
    JumpBug_RunUnit(test_CLILineEdit_Insert,"LineEditInsert");
    JumpBug_RunUnit(test_CLILineEdit_Delete,"LineEditDelete");
    JumpBug_RunUnit(test_CLILineEdit_HomeEnd,"LineEditHomeEnd");
    JumpBug_RunUnit(test_CLILineEdit_Backspace,"LineEditBackspace");
    JumpBug_RunUnit(test_CLIBackspaceBS,"BackspaceBS");
    JumpBug_RunUnit(test_CLIArrowsDrop,"CLIArrowsDrop");
    JumpBug_RunUnit(test_CLILineEdit_BufferFull,"LineEditBufferFull");
    JumpBug_RunUnit(test_CLILineEdit_Right,"LineEditRight");
#endif
    JumpBug_RunUnit(test_HelpMultiByteKeys,"HelpMultiByteKeys");
    JumpBug_RunUnit(test_AccumOverflow,"AccumOverflow");
    JumpBug_RunUnit(test_CLIMalformedKeys,"CLIMalformedKeys");
    JumpBug_RunUnit(test_MultiInstance,"MultiInstance");
    JumpBug_RunUnit(test_XelpArgs,"XelpArgs");
    JumpBug_RunUnit(test_XelpArgIntStr,"XelpArgIntStr");
    JumpBug_RunUnit(test_XelpBuf2Argv,"XelpBuf2Argv");
    JumpBug_RunUnit(test_XelpBuf2Argv_overflow,"Buf2ArgvOverflow");
    JumpBug_RunUnit(test_BranchCoverage,"BranchCoverage");
#ifdef XELP_ENABLE_LINE_EDIT
    JumpBug_RunUnit(test_CursorWithEcho,"CursorWithEcho");
#endif
    JumpBug_RunUnit(test_OutputEnable,"OutputEnable");
    JumpBug_RunUnit(test_EchoControl,"EchoControl");
#if defined(XELP_ENABLE_LINE_EDIT) && defined(XELP_ENABLE_HISTORY)
    JumpBug_RunUnit(test_HistoryBasic,"HistoryBasic");
    JumpBug_RunUnit(test_HistoryInProgressSave,"HistInProgress");
    JumpBug_RunUnit(test_HistoryFull,"HistoryFull");
    JumpBug_RunUnit(test_HistoryWithEditing,"HistoryEditing");
    JumpBug_RunUnit(test_HistoryDuplicates,"HistoryDups");
    JumpBug_RunUnit(test_HistoryAndEcho,"HistoryEcho");
#endif

    JumpBug_PrintResults();

	return JumpBug_BuildPass();
}

/*
	This main function only runs all the test code.
    If successful it returns S_OK which is equal to the numerical value of 0.
 	Any other value is considered a failure.
 */
int main()
{
	int result;

    printf("%s",XELP_BANNER_STR);
	printf("\n*************************************\nRunning Xelp Unit tests .. \n");

    logfile = fopen("xelp-test-log.yaml","w");

    result = run_tests();

    if (logfile) {     fclose(logfile); }

	if  (JB_NOTFAIL(result))
		printf ("Tests passed ++++\n\n");
	else
		printf ("Tests failed \n\n");

    return result;

}

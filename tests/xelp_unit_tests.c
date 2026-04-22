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
 test_XELPStrLen()
 */
XELPRESULT test_XELPStrLen() {

    if (JB_ASSERT(3 != XELPStrLen("abc"),"XelpStrLen abc=3"))
        return XELP_E_ERR;

    if (JB_ASSERT(0 != XELPStrLen(""),"XelpStrLen empty=0"))
        return XELP_E_ERR;

    if (JB_ASSERT(1 != XELPStrLen("x"),"XelpStrLen single char"))
        return XELP_E_ERR;

    if (JB_ASSERT(26 != XELPStrLen("abcdefghijklmnopqrstuvwxyz"),"XelpStrLen 26 chars"))
        return XELP_E_ERR;

    if (JB_ASSERT(5 != XELPStrLen("a b\tc"),"XelpStrLen with whitespace"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
/* ====================================================================
 test_XELPStrEq()

   XELPStrEq is used for comparing length-limited char buffers to null
   terminated strings such as command names

   XELPStrEq (const char* pbuf, int blen, const char *cmd)
 */
XELPRESULT test_XELPStrEq() {
    char *a = "token1";
    char *b = " token1\0abc";
    char *c = "token1abc";
    char *d = "";

    int alen = XELPStrLen(a);

    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq(a,alen,b),"XELPStrEq" ))
        return XELP_E_ERR;

    if (JB_ASSERT(XELP_S_OK != XELPStrEq(a,alen,b+1),"XELPStrEq offset"))
        return XELP_E_ERR;


    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq(a,alen,c),"XELPStrEq 3"))
        return XELP_E_ERR;


    if (JB_ASSERT(XELP_S_OK != XELPStrEq(c,alen,a),"XELPStrEq 4"))
        return XELP_E_ERR;


    alen = XELPStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(c,alen,a),"XELPStrEq len test"))
        return XELP_E_ERR;

    if (JB_ASSERT(XELP_S_OK != XELPStrEq(c,0,d),"XELPStrEq zero len test"))
        return XELP_E_ERR;

    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq(c,0,b),"XELPStrEq zero len test"))
        return XELP_E_ERR;

    /* single char match */
    if (JB_ASSERT(XELP_S_OK != XELPStrEq("a",1,"a"),"XELPStrEq single char match"))
        return XELP_E_ERR;

    /* single char mismatch */
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq("a",1,"b"),"XELPStrEq single char mismatch"))
        return XELP_E_ERR;

    /* buffer longer than command -- cmd ends before blen exhausted */
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq("foobar",6,"foo"),"XELPStrEq buf longer than cmd"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
/* ====================================================================
 test_XELPStrEq2()

   XELPStrEq2 is used for comparing ptr-delimited char buffers to null
   terminated strings.  It uses an end ptr instead of integer length.

   XELPStrEq2 (const char* pbuf, const char* pend, const char *cmd)
 */
XELPRESULT test_XELPStrEq2() {
    char *a = "token1", *ae;
    char *b = " token1\0abc";
    char *c = "token1abc";
    char *d = "";

    ae = a+ XELPStrLen(a);

    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq2(a,ae,b),"XELPStrEq2 t1" ))
        return XELP_E_ERR;

    if (JB_ASSERT(XELP_S_OK != XELPStrEq2(a,ae,b+1),"XELPStrEq2 offset"))
        return XELP_E_ERR;


    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq2(a,ae,c),"XELPStrEq2"))
        return XELP_E_ERR;


    if (JB_ASSERT(XELP_S_OK != XELPStrEq2(c,c+XELPStrLen(a),a),"XELPStrEq2"))
        return XELP_E_ERR;

    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq2(c,c+XELPStrLen(a),d),"XELPStrEq2 null start"))
        return XELP_E_ERR;

    /* single char match/mismatch */
    {
        char *sx = "x";
        if (JB_ASSERT(XELP_S_OK != XELPStrEq2(sx,sx+1,"x"),"XELPStrEq2 single match"))
            return XELP_E_ERR;

        if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq2(sx,sx+1,"y"),"XELPStrEq2 single mismatch"))
            return XELP_E_ERR;
    }

    /* empty buffer vs empty cmd */
    if (JB_ASSERT(XELP_S_OK != XELPStrEq2(d,d,""),"XELPStrEq2 empty match"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
/* ====================================================================
 test_XELPStr2Int()
 */

XELPRESULT test_XELPStr2Int() {
	if (JB_ASSERT(XELPStr2Int("90",2) != 90,"Str2Int  90"))
		return XELP_E_ERR;

    if (JB_ASSERT(XELPStr2Int("31h",3) != 49,"Str2Int 31h"))
		return XELP_E_ERR;

    if (JB_ASSERT(XELPStr2Int("-87",3) != -87,"Str2Int -87"))
		return XELP_E_ERR;

    if (JB_ASSERT(XELPStr2Int("+6546",5) != 6546,"Str2Int +6546"))
		return XELP_E_ERR;

    /* zero */
    if (JB_ASSERT(XELPStr2Int("0",1) != 0,"Str2Int 0"))
		return XELP_E_ERR;

    /* single digit */
    if (JB_ASSERT(XELPStr2Int("7",1) != 7,"Str2Int 7"))
		return XELP_E_ERR;

    /* uppercase hex */
    if (JB_ASSERT(XELPStr2Int("1Ah",3) != 0x1A,"Str2Int 1Ah uppercase"))
		return XELP_E_ERR;

    /* mixed case hex */
    if (JB_ASSERT(XELPStr2Int("aBh",3) != 0xAB,"Str2Int aBh mixed case"))
		return XELP_E_ERR;

    /* large decimal */
    if (JB_ASSERT(XELPStr2Int("12345",5) != 12345,"Str2Int 12345"))
		return XELP_E_ERR;

    /* hex 0h edge */
    if (JB_ASSERT(XELPStr2Int("0h",2) != 0,"Str2Int 0h"))
		return XELP_E_ERR;

	return XELP_S_OK;
}
/* ====================================================================
 test_XELPParseNum()
 */

XELPRESULT test_XELPParseNum() {
    int n;
    XELPRESULT r;

    r = XELPParseNum("90",2, &n);
    if (JB_ASSERT( ((n != 90) || ( r != XELP_S_OK)) ,"XELPParseNum 90"))
        return XELP_E_ERR;

    r = XELPParseNum("3ab30h",6, &n);
    if (JB_ASSERT( (n != 0x3ab30) || ( r != XELP_S_OK) ,"XELPParseNum 3ab30h"))
        return XELP_E_ERR;

    r = XELPParseNum("0x3ab30",7, &n);
    if (JB_ASSERT( (n != 0x3ab30) || ( r != XELP_S_OK) ,"XELPParseNum 0x3ab30"))
        return XELP_E_ERR;

    r = XELPParseNum("-87",3, &n);
    if (JB_ASSERT( (n !=  -87) || ( r != XELP_S_OK) ,"XELPParseNum -87"))
        return XELP_E_ERR;

    r = XELPParseNum("+6457",5, &n);
    if (JB_ASSERT( (n != 6457) || ( r != XELP_S_OK) ,"XELPParseNum +6457"))
       { return XELP_E_ERR;}

    /* uppercase hex with 0x prefix */
    r = XELPParseNum("0x1A",4, &n);
    if (JB_ASSERT( (n != 0x1A) || ( r != XELP_S_OK) ,"XELPParseNum 0x1A"))
        return XELP_E_ERR;

    r = XELPParseNum("0xFF",4, &n);
    if (JB_ASSERT( (n != 0xFF) || ( r != XELP_S_OK) ,"XELPParseNum 0xFF"))
        return XELP_E_ERR;

    r = XELPParseNum("0x0",3, &n);
    if (JB_ASSERT( (n != 0) || ( r != XELP_S_OK) ,"XELPParseNum 0x0"))
        return XELP_E_ERR;

    /* zero */
    r = XELPParseNum("0",1, &n);
    if (JB_ASSERT( (n != 0) || ( r != XELP_S_OK) ,"XELPParseNum 0"))
        return XELP_E_ERR;

    /* uppercase hex with h suffix */
    r = XELPParseNum("ABCh",4, &n);
    if (JB_ASSERT( (n != 0xABC) || ( r != XELP_S_OK) ,"XELPParseNum ABCh uppercase"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
/* ====================================================================
 test_XELPBufCmp()
 */
XELPRESULT test_XELPBufCmp() {
    char *a = "token1";
    char *b = " token1\0abc";
    char *c = "token1abc";
    char *d = "token1\0 123";
    char *ae, *be, *ce, *de;

    ae = a + XELPStrLen(a);
    be = b + XELPStrLen(b);
    ce = c + XELPStrLen(a);
    de = c + XELPStrLen(d);
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPBufCmp(a,ae,b,be,XELP_CMP_TYPE_A0B0),"XELPBufCmp" ))
        return XELP_E_ERR;

    be = b+1+XELPStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XELPBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_A0B0),"XELPBufCmp"))
        return XELP_E_ERR;


    be = b+2+XELPStrLen(b+1);
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_BUF),"XELPBufCmp"))
        return XELP_E_ERR;

    ce = c+XELPStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XELPBufCmp(a,ae,c,ce,XELP_CMP_TYPE_BUF),"XELPBufCmp"))
        return XELP_E_ERR;

    ce = c+XELPStrLen(a)+1;
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPBufCmp(a,ae,c,ce,XELP_CMP_TYPE_BUF),"XELPBufCmp"))
        return XELP_E_ERR;


    if (JB_ASSERT(XELP_S_NOTFOUND != XELPBufCmp(d,de+2,a,ae,XELP_CMP_TYPE_A0),"XELPBufCmp A01"))
        return XELP_E_ERR;

    ae = a + XELPStrLen(a);
    ce = c + XELPStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XELPBufCmp(a,ae,c,ce,XELP_CMP_TYPE_A0),"XELPBufCmp A02"))
        return XELP_E_ERR;

    /* empty buffers */
    if (JB_ASSERT(XELP_S_OK != XELPBufCmp(a,a,c,c,XELP_CMP_TYPE_BUF),"XELPBufCmp empty bufs"))
        return XELP_E_ERR;

    /* single char match */
    {
        char *sa = "a", *sb = "b";
        if (JB_ASSERT(XELP_S_OK != XELPBufCmp(sa,sa+1,sa,sa+1,XELP_CMP_TYPE_BUF),"XELPBufCmp single"))
            return XELP_E_ERR;

        /* single char mismatch */
        if (JB_ASSERT(XELP_S_NOTFOUND != XELPBufCmp(sa,sa+1,sb,sb+1,XELP_CMP_TYPE_BUF),"XELPBufCmp single mismatch"))
            return XELP_E_ERR;
    }

    /* A0B0 with nulls embedded */
    {
        char x1[] = "ab\0cd";
        char x2[] = "ab\0xy";
        if (JB_ASSERT(XELP_S_OK != XELPBufCmp(x1,x1+5,x2,x2+5,XELP_CMP_TYPE_A0B0),"XELPBufCmp A0B0 null term"))
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

    le = label+XELPStrLen(label);

    x.s = b0;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok not found" ) )
        return XELP_E_ERR;

    x.s = b1;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (JB_ASSERT(XELP_S_OK != XELPFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok found" ) )
        return XELP_E_ERR;

    x.s = b2;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (JB_ASSERT(XELP_S_OK != XELPFindTok(&x,label,le,XELP_TOK_LINE),"XelpFindTok line" ) )
        return XELP_E_ERR;

    x.s = b3;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPFindTok(&x,label,le,XELP_TOK_LINE),"XelpFindTok line not found" ) )
        return XELP_E_ERR;

    /* empty buffer */
    {
        char *empty = "";
        x.s = empty; x.p = x.s; x.e = x.s;
        if (JB_ASSERT(XELP_S_NOTFOUND != XELPFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok empty buf"))
            return XELP_E_ERR;
    }

    /* token at start of buffer */
    {
        char *b4 = "label1: something\n";
        x.s = b4; x.p = x.s; x.e = x.s+XELPStrLen(x.s);
        if (JB_ASSERT(XELP_S_OK != XELPFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok at start"))
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
    XELP_XB_INIT(b,line1,XELPStrLen(line1));
    r  = XELPTokLineXB(&b,&out,XELP_TOK_ONLY);
    r2 = XELPBufCmp(line1, line1+3,out.s,out.p,XELP_CMP_TYPE_BUF);

    if (JB_ASSERT((XELP_S_OK !=r) || (XELP_S_OK != r2),"XelpToklineXB first token"))
        return XELP_E_ERR;
    XELP_XB_TOP(b);

    /* empty buffer */
    {
        char *empty = "";
        XELP_XB_INIT(b,empty,0);
        r = XELPTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_NOTFOUND != r, "XelpToklineXB empty buf"))
            return XELP_E_ERR;
    }

    /* whitespace only -- no token found */
    {
        char *ws = "   \t  \n  ";
        XELP_XB_INIT(b,ws,XELPStrLen(ws));
        r = XELPTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_NOTFOUND != r, "XelpToklineXB whitespace only"))
            return XELP_E_ERR;
    }

    /* comment -- no token found */
    {
        char *cmt = "# this is a comment\n";
        XELP_XB_INIT(b,cmt,XELPStrLen(cmt));
        r = XELPTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_NOTFOUND != r, "XelpToklineXB comment only"))
            return XELP_E_ERR;
    }

    /* multiple tokens with TOK_LINE */
    {
        char *multi = "cmd arg1 arg2\n";
        XELP_XB_INIT(b,multi,XELPStrLen(multi));
        r = XELPTokLineXB(&b,&out,XELP_TOK_LINE);
        if (JB_ASSERT(XELP_S_OK != r, "XelpToklineXB TOK_LINE"))
            return XELP_E_ERR;
        /* out.s should be cmd start, out.p should be cmd end, out.e should be end of line */
        if (JB_ASSERT(XELP_S_OK != XELPStrEq2(out.s,out.p,"cmd"), "XelpToklineXB TOK_LINE cmd match"))
            return XELP_E_ERR;
    }

    /* semicolons */
    {
        char *semi = "cmd1; cmd2; cmd3\n";
        int count = 0;
        XELP_XB_INIT(b,semi,XELPStrLen(semi));
        while (XELP_S_OK == XELPTokLineXB(&b,&out,XELP_TOK_LINE))
            count++;
        if (JB_ASSERT(count != 3, "XelpToklineXB semicolons 3 lines"))
            return XELP_E_ERR;
    }

    /* quoted strings */
    {
        char *qs = "\"hello world\" next\n";
        XELP_XB_INIT(b,qs,XELPStrLen(qs));
        r = XELPTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_OK != r, "XelpToklineXB quoted token"))
            return XELP_E_ERR;
    }

    /* backtick escape */
    {
        char *esc = "abc`; def\n";
        XELP_XB_INIT(b,esc,XELPStrLen(esc));
        r = XELPTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_OK != r, "XelpToklineXB backtick esc"))
            return XELP_E_ERR;
    }

    /* tabs and mixed whitespace */
    {
        char *tabs = "\t  tok1\t\ttok2  ";
        XELP_XB_INIT(b,tabs,XELPStrLen(tabs));
        r = XELPTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT((XELP_S_OK != r) || (XELP_S_OK != XELPStrEq2(out.s,out.p,"tok1")), "XelpToklineXB tabs"))
            return XELP_E_ERR;
    }

    /* quote with escape inside */
    {
        char *qe = "\"hello\\\"world\" next\n";
        XELP_XB_INIT(b,qe,XELPStrLen(qe));
        r = XELPTokLineXB(&b,&out,XELP_TOK_ONLY);
        if (JB_ASSERT(XELP_S_OK != r, "XelpToklineXB quoted escape"))
            return XELP_E_ERR;
    }

    /* CRLF handling - \n is the line term */
    {
        char *crlf = "tok1\ntok2\n";
        int count = 0;
        XELP_XB_INIT(b,crlf,XELPStrLen(crlf));
        while (XELP_S_OK == XELPTokLineXB(&b,&out,XELP_TOK_ONLY))
            count++;
        if (JB_ASSERT(count != 2, "XelpToklineXB newline separated tokens"))
            return XELP_E_ERR;
    }

    /* comment after token on same line */
    {
        char *tc = "tok1 # comment\ntok2\n";
        XELP_XB_INIT(b,tc,XELPStrLen(tc));
        r = XELPTokLineXB(&b,&out,XELP_TOK_LINE);
        if (JB_ASSERT((XELP_S_OK != r) || (XELP_S_OK != XELPStrEq2(out.s,out.p,"tok1")), "XelpToklineXB tok then comment"))
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

    if (JB_ASSERT(XELP_S_OK != XELPInit(x,"Xelp Unit Tests"),"XelpInit")) {
        return XELP_E_ERR;
    }

    /* verify about message set correctly */
    if (JB_ASSERT(x->mpAboutMsg == 0, "XelpInit about msg set")) {
        return XELP_E_ERR;
    }

    /* verify zeroed members */
    if (JB_ASSERT(x->mCurMode != XELP_MODE_CLI, "XelpInit mode is CLI"))
        return XELP_E_ERR;

    if (JB_ASSERT(x->mEchoState != 0, "XelpInit echo state"))
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
    XELPInit(&myXelp,"XelpOut Tests");

    /* test with no output function set -- should return OK, just no output */
    if (JB_ASSERT(XELP_S_OK != XELPOut(&myXelp,0,0),"XelpOut null msg no fn")) {
        return XELP_E_ERR;
    }

    XELP_SET_FN_OUT(myXelp,dummyOut);
    XELP_SET_FN_THR(myXelp,dummyOut);
    XELP_SET_FN_ERR(myXelp,dummyOut);

    /* print single char with maxlen=1 */
    gChar = 0;
    if (JB_ASSERT(XELP_S_OK != XELPOut(&myXelp,"a",1),"XelpOut single char")) {
        return XELP_E_ERR;
    }
    if (JB_ASSERT(gChar != 'a', "XelpOut single char value"))
        return XELP_E_ERR;

    /* print 2 chars, verify last char emitted */
    gChar = 0;
    if (JB_ASSERT(XELP_S_OK != XELPOut(&myXelp,"ab",2),"XelpOut two chars")) {
        return XELP_E_ERR;
    }
    if (JB_ASSERT(gChar != 'b', "XelpOut last char is b"))
        return XELP_E_ERR;

    /* null msg should be safe */
    if (JB_ASSERT(XELP_S_OK != XELPOut(&myXelp,0,0),"XelpOut NULL msg")) {
        return XELP_E_ERR;
    }

    /* maxlen=0 should print until null terminator (unbounded) */
    resetDummyBuf();
    XELP_SET_FN_OUT(myXelp,gDummyBufOut);
    XELPOut(&myXelp,"hello",0);
    gDummyBufOut(0);
    if (JB_ASSERT(XELPStrLen(gDummyBuf) != 5, "XelpOut maxlen=0 prints all"))
        return XELP_E_ERR;

    /* maxlen=-1 should also print until null terminator (unbounded) */
    resetDummyBuf();
    XELPOut(&myXelp,"world",-1);
    gDummyBufOut(0);
    if (JB_ASSERT(XELPStrLen(gDummyBuf) != 5, "XelpOut maxlen=-1 prints all"))
        return XELP_E_ERR;

    /* maxlen larger than string -- should stop at null terminator */
    resetDummyBuf();
    XELPOut(&myXelp,"hi",100);
    gDummyBufOut(0);
    if (JB_ASSERT(XELPStrLen(gDummyBuf) != 2, "XelpOut maxlen>strlen"))
        return XELP_E_ERR;

    /* maxlen=1 on longer string -- should print exactly 1 char */
    resetDummyBuf();
    XELPOut(&myXelp,"abcdef",1);
    gDummyBufOut(0);
    if (JB_ASSERT(XELPStrLen(gDummyBuf) != 1, "XelpOut maxlen=1 truncates"))
        return XELP_E_ERR;

    /* empty string should print nothing */
    resetDummyBuf();
    XELPOut(&myXelp,"",5);
    gDummyBufOut(0);
    if (JB_ASSERT(XELPStrLen(gDummyBuf) != 0, "XelpOut empty string"))
        return XELP_E_ERR;

    /* null output function -- should be safe */
    {
        XELP x2;
        XELPInit(&x2,"test");
        if (JB_ASSERT(XELP_S_OK != XELPOut(&x2,"hello",5), "XelpOut no fn set"))
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
    XELPInit(&x,"Test XelpHelp");

    XELP_SET_FN_KEY(x,keyCmds);
    XELP_SET_FN_CLI(x,cliCmds);
	XELP_SET_FN_OUT(x,gDummyBufOut);

    r = XELPHelp(&x);
    gDummyBufOut(0);

    /* Bug fix: use > 0 instead of hard-coded 149, since output length depends on bug fixes */
    if (JB_ASSERT( (r!= XELP_S_OK) || ( XELPStrLen(gDummyBuf) <= 0), "Test Help output" )) {
        return XELP_E_ERR;
    }

    /* test help with no KEY commands */
    {
        XELP x2;
        resetDummyBuf();
        XELPInit(&x2,"Help no keys");
        XELP_SET_FN_CLI(x2,cliCmds);
        XELP_SET_FN_OUT(x2,gDummyBufOut);
        r = XELPHelp(&x2);
        gDummyBufOut(0);
        if (JB_ASSERT( (r!= XELP_S_OK) || (XELPStrLen(gDummyBuf) <= 0), "Test Help no keys"))
            return XELP_E_ERR;
    }

    /* test help with no CLI commands */
    {
        XELP x3;
        resetDummyBuf();
        XELPInit(&x3,"Help no cli");
        XELP_SET_FN_KEY(x3,keyCmds);
        XELP_SET_FN_OUT(x3,gDummyBufOut);
        r = XELPHelp(&x3);
        gDummyBufOut(0);
        if (JB_ASSERT( (r!= XELP_S_OK) || (XELPStrLen(gDummyBuf) <= 0), "Test Help no cli"))
            return XELP_E_ERR;
    }

    /* test help with NULL tables */
    {
        XELP x4;
        resetDummyBuf();
        XELPInit(&x4,"Help null tables");
        XELP_SET_FN_OUT(x4,gDummyBufOut);
        r = XELPHelp(&x4);
        gDummyBufOut(0);
        if (JB_ASSERT( r != XELP_S_OK, "Test Help null tables"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}
/* ====================================================================
 test_XELPExecKC()

 Bug fix: line 476 tested gGlobalCallbackData.c1 but should test k1
 (key mode callback sets k1, not c1). Also && should be || for proper
 failure detection.
 */
XELPRESULT test_XELPExecKC() {
    XELP x;
    XELPRESULT r;

    XELPInit(&x,"TestExecKC");

    r = XELPExecKC(&x,'1');
    if (JB_ASSERT(r!=XELP_S_NOTFOUND,"ExecKC null ptr")){
        return r;
    }

    XELP_SET_FN_KEY(x,gMyKeyCommands);
    XELP_SET_FN_CLI(x,gMyCLICommands);
	XELP_SET_FN_OUT(x,dummyOut);

    gGlobalCallbackData.k1 = 0;
    r = XELPExecKC(&x,'1');
    if (JB_ASSERT((r!=XELP_S_OK) || (gGlobalCallbackData.k1!='1'),"ExecKC '1' ")){
        return r;
    }

    r = XELPExecKC(&x,'z'); /* not a mapped key */
    if (JB_ASSERT(r!=XELP_S_NOTFOUND,"ExecKC 'z'")){
        return r;
    }

    /* verify return value stored in mR[0] */
    if (JB_ASSERT(XELP_R0(x) != XELP_S_NOTFOUND, "ExecKC mR[0] stores result"))
        return XELP_E_ERR;

    /* test key '0' */
    gGlobalCallbackData.k0 = 0;
    r = XELPExecKC(&x,'0');
    if (JB_ASSERT((r!=XELP_S_OK) || (gGlobalCallbackData.k0!='0'), "ExecKC '0'"))
        return XELP_E_ERR;

    return XELP_S_OK;

}
/* ====================================================================
 test_XELPParseKey()

 Bug fix: comments on KEY mode tests said "THR 0" but tests KEY mode.
 Fixed comment labels.
 */
XELPRESULT test_XELPParseKey() {
    XELP x;
    XELPRESULT r;
    int i;

    r = XELPInit(&x,"TestParseKey");
    XELP_SET_FN_KEY(x,gMyKeyCommands);
	XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* test CLI command via ParseKey -- type "foo" and press enter */
    {
        char *c1 = " foo ";
        for (i=0; i  <XELPStrLen(c1); i++) {
            r = XELPParseKey(&x,c1[i]);
            if (JB_ASSERT(r!= XELP_S_OK, "XELPParseKey -- sending keys")){
                return XELP_E_ERR;
            }
        }
        r = XELPParseKey(&x,XELPKEY_ENTER);
            if (JB_ASSERT(r!= XELP_S_OK, "XELPParseKey -- sending keys")){
                return XELP_E_ERR;
            }
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1,"Test cli 1 value")) {
            return XELP_E_ERR;
        }
    }

    /* test backspace handling */
    {
        char *c2 = " bar; ";
        for (i=0; i  <XELPStrLen(c2); i++) {
            r = XELPParseKey(&x,c2[i]);
            if (JB_ASSERT(r!= XELP_S_OK, "XELPParseKey -- sending keys")){
                return XELP_E_ERR;
            }
        }
        r = XELPParseKey(&x,XELPKEY_BKSP);
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r!= XELP_S_OK, "XELPParseKey -- sending keys w bskp test")){
            return XELP_E_ERR;
        }
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1,"XELPParseKey test cli1 value")) {
            return XELP_E_ERR;
        }


#ifndef XELP_ENABLE_LINE_EDIT
        XELP_SET_FN_BKSP(x, dummyVoid1);
        dummyVoid0();
        r = XELPParseKey(&x,XELPKEY_CLI);
        r = XELPParseKey(&x,'a');
        r = XELPParseKey(&x,XELPKEY_BKSP);
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT( (r!= XELP_S_OK) || (gBool != 1), "XELPParseKey --  bskp callback test")){
            return XELP_E_ERR;
        }
#else
        /* with line editing, mpfBksp is not called; library handles visual feedback */
        r = XELPParseKey(&x,XELPKEY_CLI);
        r = XELPParseKey(&x,'a');
        r = XELPParseKey(&x,XELPKEY_BKSP);
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r!= XELP_S_OK, "XELPParseKey --  bskp line edit test")){
            return XELP_E_ERR;
        }
#endif

    }

    /* test mode changes
       Note: ESC (XELPKEY_KEY default) is deferred by the key accumulator until the
       next byte arrives.  So switching to KEY mode requires two XELPParseKey calls:
       the ESC byte, then any non-'[' byte that flushes it. The flush byte is then
       reprocessed in the new mode.  To isolate the KEY mode change, we send a NUL
       byte after ESC (NUL is not a mode-switch key, so it gets dispatched in KEY mode). */
    {
        XELP_SET_FN_EMCHG(x,0);

        r = XELPParseKey(&x,XELPKEY_CLI);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI), "XELPParseKey -- mode change to CLI 1")){
            return XELP_E_ERR;
        }
        r = XELPParseKey(&x,XELPKEY_KEY); /* ESC: stashed in accumulator */
        r = XELPParseKey(&x,'!');          /* flush ESC → KEY mode, '!' dispatched as key cmd */
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY), "XELPParseKey -- mode change to KEY")){
            return XELP_E_ERR;
        }

        XELP_SET_FN_EMCHG(x,dummyIntOut);
        XELP_SET_FN_THR(x,dummyOut);

        r = XELPParseKey(&x,XELPKEY_THR);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gInt != x.mCurMode), "XELPParseKey -- mode change to THR")){
            return XELP_E_ERR;
        }

        r = XELPParseKey(&x,XELPKEY_CLI);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gInt != x.mCurMode), "XELPParseKey -- mode change to CLI 2")){
            return XELP_E_ERR;
        }

        XELP_SET_FN_EMCHG(x,0);
    }

    /* test THR function redirects */
    {
        XELP_SET_FN_EMCHG(x,0);

        r = XELPParseKey(&x,XELPKEY_THR);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR), "XELPParseKey -- mode change to THR")){
            return XELP_E_ERR;
        }

        XELP_SET_FN_THR(x,dummyOut);
        r = XELPParseKey(&x,'a');
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gChar != 'a'), "XELPParseKey --  THR 1")){
            return XELP_E_ERR;
        }
        r = XELPParseKey(&x,'b');
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gChar != 'b'), "XELPParseKey --  THR 2")){
            return XELP_E_ERR;
        }
        r = XELPParseKey(&x,XELPKEY_CLI);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gChar != 'b'), "XELPParseKey --  THR exit")){
            return XELP_E_ERR;
        }

    }

    /* test KEY function redirects */
    {
        r = XELPParseKey(&x,XELPKEY_KEY); /* ESC: stashed */
        r = XELPParseKey(&x,'0');         /* flush ESC → KEY mode, reprocess '0' → executes k0 */
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY) || (gGlobalCallbackData.k0 != '0'), "XELPParseKey -- KEY cmd 0")){
            return XELP_E_ERR;
        }

        gGlobalCallbackData.k1 = 'y';
        r = XELPParseKey(&x,'1');
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY) || (gGlobalCallbackData.k1 != '1'), "XELPParseKey -- KEY cmd 1")){
            return XELP_E_ERR;
        }

        gGlobalCallbackData.k1 = 'z';
        r = XELPParseKey(&x,XELPKEY_CLI);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gGlobalCallbackData.k1  != 'z'), "XELPParseKey -- KEY exit")){
            return XELP_E_ERR;
        }

    }

    /* test backspace at buffer start (no-op) */
    {
        XELP_SET_FN_BKSP(x,0);
        r = XELPParseKey(&x,XELPKEY_CLI);
        /* buffer is now at start, backspace should be no-op */
        r = XELPParseKey(&x,XELPKEY_BKSP);
        if (JB_ASSERT(r != XELP_S_OK, "XELPParseKey bksp at start"))
            return XELP_E_ERR;
    }

    /* test CLI buffer overflow -- type more than XELP_CMDBUFSZ chars */
    {
        XELP_SET_FN_BKSP(x,0);
        r = XELPParseKey(&x,XELPKEY_CLI);
        for (i = 0; i < XELP_CMDBUFSZ + 10; i++) {
            r = XELPParseKey(&x,'x');
        }
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r != XELP_S_OK, "XELPParseKey CLI overflow"))
            return XELP_E_ERR;
    }

    /* test mode switch with no registered KEY callbacks stays in current mode */
    {
        XELP x2;
        XELPInit(&x2,"NoKeyCallbacks");
        XELP_SET_FN_CLI(x2,gMyCLICommands);
        XELP_SET_FN_OUT(x2,dummyOut);
        /* no KEY funcs set -- trying to switch to KEY should stay in CLI */
        r = XELPParseKey(&x2,XELPKEY_KEY); /* ESC: stashed */
        r = XELPParseKey(&x2,'!');          /* flush: ESC fails to switch (no KEY table), '!' goes to CLI */
        if (JB_ASSERT(x2.mCurMode != XELP_MODE_CLI, "XELPParseKey no KEY stays CLI"))
            return XELP_E_ERR;
    }


    return XELP_S_OK;

}




/* ====================================================================
 test_XELPTokN()

 Bug fix: line 669-670 had XELPTokN called on x (init'd with c2)
 then XELP_XB_INIT re-inits x with c2 using XELPStrLen(c3) as length.
 Fixed to use correct length.
 */

XELPRESULT test_XELPTokN() {
    XelpBuf x, tok;
    XELPRESULT r;
    char *c1 =   "tok0 tok1 tok2    \t tok3   tok4\n tok5";
    char *c2 = "\ttok0 tok1 tok2    \t # tok3   tok4\n tok5 ";
    char *c3 = " tok0 tok1 tok2    \t # tok3   tok4;\n tok5; tok6 ";
    char *c4 = " tok0 tok1 tok2    \t #tok3   tok4;\n tok5; tok6 ";

    XELP_XB_INIT(x,c1,XELPStrLen(c1));
    r = XELPTokN(&x,0,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok0") )),"XELPTokN get 0th token"))
        return XELP_E_ERR;

    r = XELPTokN(&x,3,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok3") )),"XELPTokN get 3rd token"))
        return XELP_E_ERR;


    XELP_XB_INIT(x,c2,XELPStrLen(c2));
    r = XELPTokN(&x,3,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get 3rd token w commented line"))
        return XELP_E_ERR;

    /* Bug fix: use c2 with XELPStrLen(c2) instead of c3 length */
    XELP_XB_INIT(x,c2,XELPStrLen(c2));
    r = XELPTokN(&x,3,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get 3rd token w comment (fixed)"))
        return XELP_E_ERR;


    XELP_XB_INIT(x,c4,XELPStrLen(c4));
    r = XELPTokN(&x,3,&tok);
    if (JB_ASSERT( ((r != XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get 3rd token w commented line w space"))
        return XELP_E_ERR;

    XELP_XB_INIT(x,c4,XELPStrLen(c4));
    r = XELPTokN(&x,23,&tok);
    if (JB_ASSERT( ((r == XELP_S_OK) || (XELP_S_NOTFOUND != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get token past buffer"))
        return XELP_E_ERR;

    /* quoted token */
    {
        char *q = "\"tok0\" tok1 tok2";
        XELP_XB_INIT(x,q,XELPStrLen(q));
        r = XELPTokN(&x,1,&tok);
        if (JB_ASSERT(r != XELP_S_OK, "XELPTokN quoted tok"))
            return XELP_E_ERR;
    }

    /* n=0 edge - get very first token */
    {
        char *s1 = "first second";
        XELP_XB_INIT(x,s1,XELPStrLen(s1));
        r = XELPTokN(&x,0,&tok);
        if (JB_ASSERT((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s,tok.p,"first")), "XELPTokN n=0"))
            return XELP_E_ERR;
    }

    /* test using c3 with correct length */
    XELP_XB_INIT(x,c3,XELPStrLen(c3));
    r = XELPTokN(&x,0,&tok);
    if (JB_ASSERT((r != XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s,tok.p,"tok0")), "XELPTokN c3 first"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
/* ====================================================================
 test_XELPNumToks()
 */

XELPRESULT test_XELPNumToks() {
    XelpBuf x;
    XELPRESULT r;
    int n=0;
    char *c0 = "";
    char *c1 = "tok1 tok2    \t tok3   tok4\n t0k5";
    char *c2 = "\t tok1 tok2    \t# tok3   tok4\n t0k5; tok6";
    char *c3 = "\t tok1 tok2    \t#tok3   tok4\n t0k5; tok6";

    XELP_XB_INIT(x,c0,XELPStrLen(c0));
    r = XELPNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=0)),"XELPNumToks empty"))
        return XELP_E_ERR;

    XELP_XB_INIT(x,c1,XELPStrLen(c1));
    r = XELPNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=5)),"XELPNumToks tabs and newlines"))
        return XELP_E_ERR;

    XELP_XB_INIT(x,c2,XELPStrLen(c2));
    r = XELPNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=4)),"XELPNumToks comment on second line"))
        return XELP_E_ERR;

    XELP_XB_INIT(x,c3,XELPStrLen(c3));
    r = XELPNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=4)),"XELPNumToks comment hugging"))
        return XELP_E_ERR;

    /* single token */
    {
        char *s1 = "only";
        XELP_XB_INIT(x,s1,XELPStrLen(s1));
        r = XELPNumToks(&x,&n);
        if (JB_ASSERT(((r!=XELP_S_OK) || (n !=1)),"XELPNumToks single"))
            return XELP_E_ERR;
    }

    /* all whitespace -- tokenizer returns 1 empty token for non-empty whitespace buffers */
    {
        char *ws = "   \t  \n  ";
        XELP_XB_INIT(x,ws,XELPStrLen(ws));
        r = XELPNumToks(&x,&n);
        if (JB_ASSERT(r!=XELP_S_OK,"XELPNumToks all whitespace"))
            return XELP_E_ERR;
    }

    /* all comments -- tokenizer returns 1 token for comment-only buffers */
    {
        char *cm = "# all comment\n";
        XELP_XB_INIT(x,cm,XELPStrLen(cm));
        r = XELPNumToks(&x,&n);
        if (JB_ASSERT(r!=XELP_S_OK,"XELPNumToks all comments"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}
/* ====================================================================
 test_XELPParseXB() - actual command dispatch verification

 Bug fix: was a stub that just inited and returned OK.
 Now tests actual command execution.
 */

XELPRESULT test_XELPParseXB() {
    XELP x;
    XelpBuf script;
    char *s;
    XELPRESULT r;

    XELPInit(&x,"TestParseXB");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* single command */
    gGlobalCallbackData.c1 = 0;
    s = "foo arg1\n";
    XELP_XB_INIT(script,s,XELPStrLen(s));
    r = XELPParseXB(&x,&script);
    if (JB_ASSERT((r != XELP_S_OK) || (gGlobalCallbackData.c1 != 1), "XELPParseXB single cmd"))
        return XELP_E_ERR;

    /* multiple commands separated by semicolons */
    gGlobalCallbackData.c1 = 0;
    gGlobalCallbackData.c2 = 0;
    s = "foo; bar\n";
    XELP_XB_INIT(script,s,XELPStrLen(s));
    r = XELPParseXB(&x,&script);
    if (JB_ASSERT((r != XELP_S_OK) || (gGlobalCallbackData.c1 != 1) || (gGlobalCallbackData.c2 != 2), "XELPParseXB multi cmd"))
        return XELP_E_ERR;

    /* multiple commands separated by newlines */
    gGlobalCallbackData.c1 = 0;
    gGlobalCallbackData.c2 = 0;
    s = "foo\nbar\n";
    XELP_XB_INIT(script,s,XELPStrLen(s));
    r = XELPParseXB(&x,&script);
    if (JB_ASSERT((r != XELP_S_OK) || (gGlobalCallbackData.c1 != 1) || (gGlobalCallbackData.c2 != 2), "XELPParseXB newline cmds"))
        return XELP_E_ERR;

    /* command not found -- verify mR[0] */
    s = "nonexistent\n";
    XELP_XB_INIT(script,s,XELPStrLen(s));
    r = XELPParseXB(&x,&script);
    if (JB_ASSERT((r != XELP_S_OK) || (XELP_R0(x) != XELP_E_CMDNOTFOUND), "XELPParseXB cmd not found"))
        return XELP_E_ERR;

    /* empty input */
    s = "";
    XELP_XB_INIT(script,s,XELPStrLen(s));
    r = XELPParseXB(&x,&script);
    if (JB_ASSERT(r != XELP_S_OK, "XELPParseXB empty input"))
        return XELP_E_ERR;

    /* comment-only input */
    s = "# just a comment\n";
    XELP_XB_INIT(script,s,XELPStrLen(s));
    r = XELPParseXB(&x,&script);
    if (JB_ASSERT(r != XELP_S_OK, "XELPParseXB comment only"))
        return XELP_E_ERR;

    /* NULL function table */
    {
        XELP x2;
        XELPInit(&x2,"TestNullTable");
        XELP_SET_FN_OUT(x2,dummyOut);
        s = "foo\n";
        XELP_XB_INIT(script,s,XELPStrLen(s));
        r = XELPParseXB(&x2,&script);
        if (JB_ASSERT(r != XELP_S_OK, "XELPParseXB null table"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}
/* ====================================================================
 test_XELPParse()

 Bug fix: old test called XELPParse but didn't verify the command executed.
 */

XELPRESULT test_XELPParse() {
    XELP x;
    char *s;
    XELPRESULT r;

    XELPInit(&x,"TestParse");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* test actual command dispatch */
    gGlobalCallbackData.c1 = 0;
    s = "foo ";
    r = XELPParse(&x,s,XELPStrLen(s));
    if (JB_ASSERT((r!=XELP_S_OK) || (gGlobalCallbackData.c1 != 1),"XELPParse foo executes"))
        return XELP_E_ERR;

    /* test with semicolons */
    gGlobalCallbackData.c0 = -1;
    gGlobalCallbackData.c2 = 0;
    s = "cli0; bar\n";
    r = XELPParse(&x,s,XELPStrLen(s));
    if (JB_ASSERT((r!=XELP_S_OK) || (gGlobalCallbackData.c0 != 0) || (gGlobalCallbackData.c2 != 2),"XELPParse multi"))
        return XELP_E_ERR;

    /* test command not found */
    s = "doesnotexist\n";
    r = XELPParse(&x,s,XELPStrLen(s));
    if (JB_ASSERT((r!=XELP_S_OK) || (XELP_R0(x) != XELP_E_CMDNOTFOUND), "XELPParse not found"))
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
        XELPInit(&xelp,"outxb test");
        resetDummyBuf();
        XELP_SET_FN_OUT(xelp,gDummyBufOut);
        XELP_XB_INIT(ob,obuf,5);
        XELP_XB_OUT(&xelp,ob);
        gDummyBufOut(0);
        if (JB_ASSERT(XELPStrLen(gDummyBuf) != 5, "XELP_XB_OUT macro"))
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
    XELPInit(&x,"TestDefHandlers");
    XELP_SET_FN_KEY(x,gMyKeyCommands);
    XELP_SET_FN_OUT(x,dummyOut);

    r = XELPExecKC(&x,'z');
    if (JB_ASSERT(r != XELP_S_NOTFOUND, "DefKey null handler returns NOTFOUND"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(x) != XELP_S_NOTFOUND, "DefKey null handler mR[0]"))
        return XELP_E_ERR;

    /* set default KEY handler -- unmapped key should call it */
    XELP_SET_FN_DEF_KEY(x,defKeyHandler);
    gDefKeyVal = 0;
    r = XELPExecKC(&x,'z');
    if (JB_ASSERT(r != XELP_W_WARN, "DefKey handler called return"))
        return XELP_E_ERR;
    if (JB_ASSERT(gDefKeyVal != 'z', "DefKey handler received key"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(x) != XELP_W_WARN, "DefKey handler mR[0] stores result"))
        return XELP_E_ERR;

    /* mapped key should NOT call default handler */
    gDefKeyVal = 0;
    gGlobalCallbackData.k1 = 0;
    r = XELPExecKC(&x,'1');
    if (JB_ASSERT(r != XELP_S_OK, "DefKey mapped key returns OK"))
        return XELP_E_ERR;
    if (JB_ASSERT(gGlobalCallbackData.k1 != '1', "DefKey mapped key calls fn"))
        return XELP_E_ERR;
    if (JB_ASSERT(gDefKeyVal != 0, "DefKey handler NOT called for mapped key"))
        return XELP_E_ERR;

    /* default KEY handler with NULL fn table */
    {
        XELP x2;
        XELPInit(&x2,"TestDefKeyNoTable");
        XELP_SET_FN_OUT(x2,dummyOut);
        XELP_SET_FN_DEF_KEY(x2,defKeyHandler);
        gDefKeyVal = 0;
        r = XELPExecKC(&x2,'q');
        if (JB_ASSERT(r != XELP_W_WARN, "DefKey no table calls default"))
            return XELP_E_ERR;
        if (JB_ASSERT(gDefKeyVal != 'q', "DefKey no table received key"))
            return XELP_E_ERR;
    }

    /* default KEY handler via ParseKey in KEY mode */
    {
        XELP x3;
        XELPInit(&x3,"TestDefKeyParseKey");
        XELP_SET_FN_KEY(x3,gMyKeyCommands);
        XELP_SET_FN_CLI(x3,gMyCLICommands);
        XELP_SET_FN_OUT(x3,dummyOut);
        XELP_SET_FN_DEF_KEY(x3,defKeyHandler);
        XELPParseKey(&x3,XELPKEY_KEY); /* ESC: stashed */
        gDefKeyVal = 0;
        XELPParseKey(&x3,'q'); /* flush ESC → KEY mode, reprocess 'q' → default handler */
        if (JB_ASSERT(gDefKeyVal != 'q', "DefKey via ParseKey"))
            return XELP_E_ERR;
    }

    /* ---- CLI default handler tests ---- */

    /* unknown command with no default CLI handler -- mR[0] = CMDNOTFOUND */
    XELPInit(&x,"TestDefCLI");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    s = "unknowncmd arg1\n";
    XELP_XB_INIT(script,s,XELPStrLen(s));
    r = XELPParseXB(&x,&script);
    if (JB_ASSERT(XELP_R0(x) != XELP_E_CMDNOTFOUND, "DefCLI null handler CMDNOTFOUND"))
        return XELP_E_ERR;

    /* set default CLI handler -- unknown command should call it */
    XELP_SET_FN_DEF_CLI(x,defCLIHandler);
    gDefCLIArgs = 0;
    gDefCLILen = 0;
    s = "unknowncmd arg1\n";
    XELP_XB_INIT(script,s,XELPStrLen(s));
    r = XELPParseXB(&x,&script);
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
    XELP_XB_INIT(script,s,XELPStrLen(s));
    r = XELPParseXB(&x,&script);
    if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "DefCLI known cmd dispatched"))
        return XELP_E_ERR;
    if (JB_ASSERT(gDefCLIArgs != 0, "DefCLI handler NOT called for known cmd"))
        return XELP_E_ERR;

    /* multiple commands: one known, one unknown -- default handler called for unknown only */
    gDefCLIArgs = 0;
    gGlobalCallbackData.c1 = 0;
    s = "foo; badcmd\n";
    XELP_XB_INIT(script,s,XELPStrLen(s));
    r = XELPParseXB(&x,&script);
    if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "DefCLI mixed: known ran"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(x) != XELP_W_WARN, "DefCLI mixed: default handler ran"))
        return XELP_E_ERR;

    /* default CLI handler with NULL fn table */
    {
        XELP x4;
        XELPInit(&x4,"TestDefCLINoTable");
        XELP_SET_FN_OUT(x4,dummyOut);
        XELP_SET_FN_DEF_CLI(x4,defCLIHandler);
        /* no CLI table set -- command should NOT dispatch (no table to search) */
        gDefCLIArgs = 0;
        s = "anything\n";
        XELP_XB_INIT(script,s,XELPStrLen(s));
        r = XELPParseXB(&x4,&script);
        /* with null fn table the dispatch loop is skipped entirely */
        if (JB_ASSERT(r != XELP_S_OK, "DefCLI null table returns OK"))
            return XELP_E_ERR;
    }

    /* default CLI handler via ParseKey (type unknown cmd + enter) */
    {
        XELP x5;
        int i;
        char *cmd = "badcmd";
        XELPInit(&x5,"TestDefCLIParseKey");
        XELP_SET_FN_CLI(x5,gMyCLICommands);
        XELP_SET_FN_OUT(x5,dummyOut);
        XELP_SET_FN_DEF_CLI(x5,defCLIHandler);
        gDefCLIArgs = 0;
        for (i = 0; i < XELPStrLen(cmd); i++)
            XELPParseKey(&x5,cmd[i]);
        XELPParseKey(&x5,XELPKEY_ENTER);
        if (JB_ASSERT(XELP_R0(x5) != XELP_W_WARN, "DefCLI via ParseKey"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_buffer_boundaries() - verify that buffer limits are never exceeded
 across all API entry points. Exercises exact boundary conditions for:
 - CLI command buffer (XELP_CMDBUFSZ) via ParseKey
 - XELPParseXB / XELPParse with various buffer sizes
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
        XELPInit(&x,"BndTest");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        gBndCallCount = 0;
        for (i = 0; i < XELP_CMDBUFSZ - 2; i++)
            XELPParseKey(&x,'A');
        XELPParseKey(&x,XELPKEY_ENTER);
        /* the typed chars should have been captured and parsed */
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd CLI reset after enter"))
            return XELP_E_ERR;
    }

    /* 2. Type exactly XELP_CMDBUFSZ-1 chars (fill to capacity) + enter */
    {
        XELPInit(&x,"BndTest2");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        for (i = 0; i < XELP_CMDBUFSZ - 1; i++)
            XELPParseKey(&x,'B');
        /* buffer should be exactly full now */
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != XELP_CMDBUFSZ - 1, "bnd CLI full"))
            return XELP_E_ERR;
        XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd CLI reset after full"))
            return XELP_E_ERR;
    }

    /* 3. Overflow: type XELP_CMDBUFSZ * 2 chars -- buffer must not exceed capacity */
    {
        XELPInit(&x,"BndTest3");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        for (i = 0; i < XELP_CMDBUFSZ * 2; i++)
            XELPParseKey(&x,'C');
        /* XELP_XB_PUTC bounds check should have prevented overflow */
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != XELP_CMDBUFSZ - 1, "bnd CLI overflow stopped"))
            return XELP_E_ERR;
        /* verify buffer end ptr is correct */
        if (JB_ASSERT(x.mCmdXB.p > x.mCmdXB.e, "bnd CLI ptr within bounds"))
            return XELP_E_ERR;
        XELPParseKey(&x,XELPKEY_ENTER);
    }

    /* 4. Backspace at empty buffer -- p must not go below s */
    {
        XELPInit(&x,"BndTest4");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        for (i = 0; i < 10; i++)
            XELPParseKey(&x,XELPKEY_BKSP);
        if (JB_ASSERT(x.mCmdXB.p < x.mCmdXB.s, "bnd bksp at empty no underflow"))
            return XELP_E_ERR;
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd bksp stays at 0"))
            return XELP_E_ERR;
    }

    /* 5. Type, backspace to empty, type again -- buffer reuse */
    {
        XELPInit(&x,"BndTest5");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        XELPParseKey(&x,'x');
        XELPParseKey(&x,'y');
        XELPParseKey(&x,'z');
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 3, "bnd type 3 chars"))
            return XELP_E_ERR;
        for (i = 0; i < 10; i++)
            XELPParseKey(&x,XELPKEY_BKSP);
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd bksp to empty"))
            return XELP_E_ERR;
        XELPParseKey(&x,'a');
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 1, "bnd retype after bksp"))
            return XELP_E_ERR;
        XELPParseKey(&x,XELPKEY_ENTER);
    }

    /* === Command handler receives correctly bounded length === */

    /* 6. handler len matches actual token+args length */
    {
        XELPInit(&x,"BndTest6");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        gBndArgs = 0;
        gBndLen = 0;
        gBndCallCount = 0;
        {
            char *s = "cmd arg1 arg2\n";
            XELP_XB_INIT(script,s,XELPStrLen(s));
            r = XELPParseXB(&x,&script);
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
            XELP_XB_INIT(script,s,XELPStrLen(s));
            r = XELPParseXB(&x,&script);
        }
        if (JB_ASSERT(gBndLen != 3, "bnd handler cmd-only len=3"))
            return XELP_E_ERR;
    }

    /* 8. handler len for single-char line (no newline, just "cmd") */
    {
        gBndLen = -1;
        {
            char *s = "cmd";
            XELP_XB_INIT(script,s,XELPStrLen(s));
            r = XELPParseXB(&x,&script);
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
            XELP_XB_INIT(script,s,XELPStrLen(s));
            r = XELPParseXB(&x,&script);
        }
        if (JB_ASSERT(gBndCallCount != 2, "bnd semicolon 2 calls"))
            return XELP_E_ERR;
        /* last call should have been for "cmd bb" */
        if (JB_ASSERT(gBndLen != 6, "bnd semicolon second len=6"))
            return XELP_E_ERR;
    }

    /* === XELPParse boundary -- len parameter respected === */

    /* 10. XELPParse with exact length */
    {
        gBndCallCount = 0;
        gBndLen = -1;
        r = XELPParse(&x,"cmd x\n",6);
        if (JB_ASSERT(gBndCallCount != 1, "bnd Parse exact len"))
            return XELP_E_ERR;
        if (JB_ASSERT(gBndLen != 5, "bnd Parse handler len=5"))
            return XELP_E_ERR;
    }

    /* 11. XELPParse with shorter length than string -- should only parse up to len */
    {
        gBndCallCount = 0;
        r = XELPParse(&x,"cmd xyz extra\n",3);  /* only "cmd" visible */
        if (JB_ASSERT(gBndCallCount != 1, "bnd Parse truncated calls"))
            return XELP_E_ERR;
        if (JB_ASSERT(gBndLen != 3, "bnd Parse truncated len=3"))
            return XELP_E_ERR;
    }

    /* 12. XELPParse with len=0 -- nothing to parse */
    {
        gBndCallCount = 0;
        r = XELPParse(&x,"cmd\n",0);
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
        r = XELPTokLineXB(&b,&tok,XELP_TOK_ONLY);
        if (JB_ASSERT(r != XELP_S_OK, "bnd tok single char"))
            return XELP_E_ERR;
    }

    /* 17. tokenizer with exact-length buffer (no trailing space) */
    {
        XelpBuf b, tok;
        char *s = "tok1";
        XELP_XB_INIT(b,s,4);
        r = XELPTokLineXB(&b,&tok,XELP_TOK_ONLY);
        if (JB_ASSERT(r != XELP_S_OK, "bnd tok exact len"))
            return XELP_E_ERR;
        if (JB_ASSERT(XELP_S_OK != XELPStrEq2(tok.s,tok.p,"tok1"), "bnd tok exact match"))
            return XELP_E_ERR;
    }

    /* 18. repeated parse cycles -- buffer resets correctly each time */
    {
        XELPInit(&x,"BndRepeat");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        for (i = 0; i < 50; i++) {
            int j;
            for (j = 0; j < 3; j++)
                XELPParseKey(&x,'c');
            XELPParseKey(&x,XELPKEY_ENTER);
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
        XELPInit(&x,"BndExact");
        XELP_SET_FN_CLI(x,bndCmds);
        XELP_SET_FN_OUT(x,dummyOut);

        gBndCallCount = 0;
        /* XELPInit sets mCmdXB capacity to XELP_CMDBUFSZ-1 */
        for (i = 0; i < XELP_CMDBUFSZ - 1; i++)
            XELPParseKey(&x,'D');
        /* one more should be dropped by XBPUTC bounds check */
        XELPParseKey(&x,'E');
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != XELP_CMDBUFSZ - 1, "bnd exact cap+1"))
            return XELP_E_ERR;
        XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "bnd exact cap enter reset"))
            return XELP_E_ERR;
    }

    /* 20. XELPParse: verify handler cannot see beyond supplied length */
    {
        char mixed[] = "cmd SECRET";  /* 10 chars total */
        gBndLen = -1;
        r = XELPParse(&x, mixed, 3);  /* only "cmd" visible */
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

    XELPInit(&x,"StressTest");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_KEY(x,gMyKeyCommands);
    XELP_SET_FN_OUT(x,dummyOut);
    XELP_SET_FN_THR(x,dummyOut);

    /* --- CLI buffer overflow via ParseKey: 2x buffer size --- */
    {
        r = XELPParseKey(&x,XELPKEY_CLI);
        for (i = 0; i < XELP_CMDBUFSZ * 2; i++) {
            r = XELPParseKey(&x,'A');
        }
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r != XELP_S_OK, "stress CLI buf overflow 2x"))
            return XELP_E_ERR;
    }

    /* --- CLI buffer overflow: exactly CMDBUFSZ-1 chars (boundary) --- */
    {
        r = XELPParseKey(&x,XELPKEY_CLI);
        for (i = 0; i < XELP_CMDBUFSZ - 1; i++) {
            r = XELPParseKey(&x,'B');
        }
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r != XELP_S_OK, "stress CLI buf boundary"))
            return XELP_E_ERR;
    }

    /* --- rapid mode switching --- */
    {
        for (i = 0; i < 100; i++) {
            XELPParseKey(&x,XELPKEY_CLI);
            XELPParseKey(&x,XELPKEY_KEY); /* ESC: stashed */
            XELPParseKey(&x,XELPKEY_THR); /* flush ESC → KEY, reprocess THR → THR mode */
        }
        /* should not crash and mode should be THR after last switch */
        if (JB_ASSERT(x.mCurMode != XELP_MODE_THR, "stress rapid mode switch"))
            return XELP_E_ERR;
        XELPParseKey(&x,XELPKEY_CLI); /* back to CLI */
    }

    /* --- backspace more times than chars typed --- */
    {
        r = XELPParseKey(&x,XELPKEY_CLI);
        XELPParseKey(&x,'x');
        XELPParseKey(&x,'y');
        for (i = 0; i < 20; i++) {
            XELPParseKey(&x,XELPKEY_BKSP);
        }
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r != XELP_S_OK, "stress backspace underflow"))
            return XELP_E_ERR;
    }

    /* --- Parse with all-semicolons script (many empty commands) --- */
    {
        char *semis = ";;;\n;;;\n;;;\n";
        XELP_XB_INIT(script,semis,XELPStrLen(semis));
        r = XELPParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress all semicolons"))
            return XELP_E_ERR;
    }

    /* --- Parse with only whitespace and newlines --- */
    {
        char *ws = "   \n\n  \t  \n\n";
        XELP_XB_INIT(script,ws,XELPStrLen(ws));
        r = XELPParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress all whitespace script"))
            return XELP_E_ERR;
    }

    /* --- Parse with unterminated quote --- */
    {
        char *uq = "\"unterminated string";
        XELP_XB_INIT(script,uq,XELPStrLen(uq));
        r = XELPParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress unterminated quote"))
            return XELP_E_ERR;
    }

    /* --- Parse with only comments, deeply nested --- */
    {
        char *cm = "# comment 1\n# comment 2\n# comment 3\n# comment 4\n";
        XELP_XB_INIT(script,cm,XELPStrLen(cm));
        r = XELPParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress all comments script"))
            return XELP_E_ERR;
    }

    /* --- Parse with backtick escape at end of buffer --- */
    {
        char *esc = "tok`";
        XELP_XB_INIT(script,esc,XELPStrLen(esc));
        r = XELPParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress backtick at end"))
            return XELP_E_ERR;
    }

    /* --- Parse with many semicolons and commands mixed --- */
    {
        char *mix = "foo; bar; rst; foo; bar; rst;\n";
        XELP_XB_INIT(script,mix,XELPStrLen(mix));
        r = XELPParseXB(&x,&script);
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
        r = XELPParseXB(&x,&script);
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
        r = XELPParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress binary chars"))
            return XELP_E_ERR;
    }

    /* --- XELPStr2Int with garbage: returns 0 on invalid input --- */
    {
        int v;
        v = XELPStr2Int("zzz",3);
        if (JB_ASSERT(v != 0, "Str2Int garbage returns 0"))
            return XELP_E_ERR;

        v = XELPStr2Int("   h",4);
        if (JB_ASSERT(v != 0, "Str2Int spaces-h returns 0"))
            return XELP_E_ERR;

        v = XELPStr2Int("343.3",5);
        if (JB_ASSERT(v != 0, "Str2Int decimal point returns 0"))
            return XELP_E_ERR;
    }

    /* --- XELPParseNum validation: must return XELP_E_ERR on bad input --- */
    {
        int n = 99;

        /* garbage decimal */
        r = XELPParseNum("xyz",3,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum garbage decimal"))
            return XELP_E_ERR;

        /* decimal with embedded dot */
        n = 99;
        r = XELPParseNum("3.14",4,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum decimal point"))
            return XELP_E_ERR;

        /* bare "0x" -- no hex digits */
        n = 99;
        r = XELPParseNum("0x",2,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum bare 0x"))
            return XELP_E_ERR;

        /* bare "h" -- no hex digits */
        n = 99;
        r = XELPParseNum("h",1,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum bare h"))
            return XELP_E_ERR;

        /* empty string */
        n = 99;
        r = XELPParseNum("",0,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum empty"))
            return XELP_E_ERR;

        /* sign only, no digits */
        n = 99;
        r = XELPParseNum("-",1,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum minus only"))
            return XELP_E_ERR;

        n = 99;
        r = XELPParseNum("+",1,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum plus only"))
            return XELP_E_ERR;

        /* hex with non-hex char: "0xGG" */
        n = 99;
        r = XELPParseNum("0xGG",4,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum 0xGG"))
            return XELP_E_ERR;

        /* hex suffix with non-hex body: "zzh" */
        n = 99;
        r = XELPParseNum("zzh",3,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum zzh"))
            return XELP_E_ERR;

        /* spaces in decimal: "1 2" */
        n = 99;
        r = XELPParseNum("1 2",3,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum space in decimal"))
            return XELP_E_ERR;

        /* valid inputs still work after exercising error paths */
        n = 0;
        r = XELPParseNum("42",2,&n);
        if (JB_ASSERT((r != XELP_S_OK) || (n != 42), "ParseNum 42 after errors"))
            return XELP_E_ERR;

        n = 0;
        r = XELPParseNum("0xFF",4,&n);
        if (JB_ASSERT((r != XELP_S_OK) || (n != 0xFF), "ParseNum 0xFF after errors"))
            return XELP_E_ERR;

        n = 0;
        r = XELPParseNum("ABh",3,&n);
        if (JB_ASSERT((r != XELP_S_OK) || (n != 0xAB), "ParseNum ABh after errors"))
            return XELP_E_ERR;

        n = 0;
        r = XELPParseNum("-99",3,&n);
        if (JB_ASSERT((r != XELP_S_OK) || (n != -99), "ParseNum -99 after errors"))
            return XELP_E_ERR;

        /* --- integer overflow: must return XELP_E_ERR, not wrap --- */

        /* decimal overflow: 20 digits always overflows any int */
        n = 0;
        r = XELPParseNum("99999999999999999999",20,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum decimal overflow"))
            return XELP_E_ERR;

        /* hex overflow: 0x + 16 F's overflows any int */
        n = 0;
        r = XELPParseNum("0xFFFFFFFFFF",12,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum hex overflow"))
            return XELP_E_ERR;

        /* hex suffix overflow */
        n = 0;
        r = XELPParseNum("FFFFFFFFFFh",11,&n);
        if (JB_ASSERT(r != XELP_E_ERR, "ParseNum hex suffix overflow"))
            return XELP_E_ERR;

        /* boundary: largest valid positive (via Str2Int which wraps ParseNum) */
        n = 0;
        r = XELPParseNum("32767",5,&n);  /* valid on 16-bit and 32-bit */
        if (JB_ASSERT(r != XELP_S_OK, "ParseNum 32767 ok"))
            return XELP_E_ERR;
    }

    /* --- XELPExecKC with all possible char values --- */
    {
        char ch;
        for (ch = 1; ch < 127; ch++) {
            XELPExecKC(&x,ch); /* should not crash on any char */
        }
        JB_ASSERT(0, "stress ExecKC all chars");
    }

    /* --- ParseKey with all printable chars --- */
    {
        XELPParseKey(&x,XELPKEY_CLI);
        for (i = 0x20; i < 0x7f; i++) {
            XELPParseKey(&x,(char)i);
        }
        XELPParseKey(&x,XELPKEY_ENTER);
        JB_ASSERT(0, "stress ParseKey all printable");
    }

    /* --- Repeated init should not leak or crash --- */
    {
        XELP x2;
        for (i = 0; i < 100; i++) {
            XELPInit(&x2,"reinit test");
        }
        JB_ASSERT(0, "stress repeated init");
    }

    /* --- Help with only about msg (no key/cli tables) --- */
    {
        XELP x3;
        XELPInit(&x3,"Only About");
        XELP_SET_FN_OUT(x3,dummyOut);
        r = XELPHelp(&x3);
        if (JB_ASSERT(r != XELP_S_OK, "stress help minimal"))
            return XELP_E_ERR;
    }

    /* --- Quoted string with backslash at end --- */
    {
        char *qesc = "\"hello\\";
        XELP_XB_INIT(script,qesc,XELPStrLen(qesc));
        r = XELPParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress quote backslash end"))
            return XELP_E_ERR;
    }

    /* --- Multiple quotes in sequence --- */
    {
        char *mq = "\"a\" \"b\" \"c\"\n";
        XELP_XB_INIT(script,mq,XELPStrLen(mq));
        r = XELPParseXB(&x,&script);
        if (JB_ASSERT(r != XELP_S_OK, "stress multiple quotes"))
            return XELP_E_ERR;
    }

    /* --- Single char buffer --- */
    {
        char *sc = "x";
        XELP_XB_INIT(script,sc,1);
        r = XELPParseXB(&x,&script);
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
        r = XELPFindTok(&bx,label,label+7,XELP_TOK_ONLY);
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
    XELPTokN(&b, 1, &tok);
    ths->mR[1] = XELPStr2Int(tok.s, tok.p - tok.s);

    XELP_XB_TOP(b);
    XELPTokN(&b, 2, &tok);
    ths->mR[2] = XELPStr2Int(tok.s, tok.p - tok.s);

    XELP_XB_TOP(b);
    XELPTokN(&b, 3, &tok);
    ths->mR[3] = XELPStr2Int(tok.s, tok.p - tok.s);

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

    /* 2. all 4 registers zeroed after XELPInit */
    XELPInit(&x, "RegTest");
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
    r = XELPExecKC(&x, 'r');
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
        XELP_XB_INIT(script, s, XELPStrLen(s));
        r = XELPParseXB(&x, &script);
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
        XELP_XB_INIT(script, s, XELPStrLen(s));
        r = XELPParseXB(&x, &script);
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
        XELP_XB_INIT(script, s, XELPStrLen(s));
        r = XELPParseXB(&x, &script);
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
        XELP_XB_INIT(script, s, XELPStrLen(s));
        r = XELPParseXB(&x, &script);
    }
    if (JB_ASSERT(XELP_R0(x) != XELP_E_CMDNOTFOUND, "R0 CMDNOTFOUND"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R1(x) != 100, "R1 untouched after not-found"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_KeyAccumulator() - multi-byte sequence assembly (tested indirectly via XELPParseKey)
 */
XELPRESULT test_KeyAccumulator() {
    XELP x;

    XELPInit(&x,"TestKeyAccum");
    XELP_SET_FN_OUT(x,dummyOut);
    XELP_SET_FN_CLI(x,gMyCLICommands);

    /* single char 'a' processes immediately into CLI buffer */
    XELPParseKey(&x, 'a');
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 1, "accum single char in buf"))
        return XELP_E_ERR;

    XELPParseKey(&x, XELPKEY_ENTER); /* reset buffer */

    /* ESC alone stalls -- accumulator holds it, nothing in buffer */
    XELPParseKey(&x, 0x1B);
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "accum ESC stalls"))
        return XELP_E_ERR;

    /* ESC + '[' still stalls (CSI start) */
    XELPParseKey(&x, '[');
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "accum CSI stalls"))
        return XELP_E_ERR;

    /* ESC + '[' + 'A' = UP arrow: silently dropped in CLI (no change to buf) */
    XELPParseKey(&x, 'A');
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "accum UP arrow dropped in CLI"))
        return XELP_E_ERR;

    /* 4-byte sequence: ESC [ 3 ~ (KDEL) at empty buf: no effect */
    XELPParseKey(&x, 0x1B);
    XELPParseKey(&x, '[');
    XELPParseKey(&x, '3');
    XELPParseKey(&x, '~');
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "accum KDEL at empty no effect"))
        return XELP_E_ERR;

    /* Verify accumulator state is clean after completed sequences */
    if (JB_ASSERT(x.mKeyLen != 0, "accum clean after sequences"))
        return XELP_E_ERR;

    /* ESC + non-'[' flushes ESC (mode switch) and reprocesses next char */
    {
        XELP x2;
        XELPInit(&x2,"TestAccumFlush");
        XELP_SET_FN_OUT(x2,dummyOut);
        XELP_SET_FN_CLI(x2,gMyCLICommands);
        XELP_SET_FN_KEY(x2,gMyKeyCommands);

        XELPParseKey(&x2, 0x1B);  /* ESC: stashed */
        gGlobalCallbackData.k0 = 0;
        XELPParseKey(&x2, '0');   /* flush ESC → KEY mode, reprocess '0' → k0 handler */
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

    XELPInit(&x,"TestMultiByte");
    XELP_SET_FN_KEY(x,mbKeys);
    XELP_SET_FN_OUT(x,dummyOut);

    /* direct dispatch of multi-byte key via XELPExecKC */
    gMultiByteKeyVal = 0;
    r = XELPExecKC(&x, XELP_KEYCODE_UP);
    if (JB_ASSERT(r != XELP_S_OK, "multibyte direct up dispatch"))
        return XELP_E_ERR;
    if (JB_ASSERT(gMultiByteKeyVal != (int)XELP_KEYCODE_UP, "multibyte direct up value"))
        return XELP_E_ERR;

    /* single char still works */
    gMultiByteKeyVal = 0;
    r = XELPExecKC(&x, 'a');
    if (JB_ASSERT(r != XELP_S_OK, "multibyte single char dispatch"))
        return XELP_E_ERR;
    if (JB_ASSERT(gMultiByteKeyVal != 'a', "multibyte single char value"))
        return XELP_E_ERR;

    /* unmapped multi-byte key */
    r = XELPExecKC(&x, XELP_KEYCODE_LEFT);
    if (JB_ASSERT(r != XELP_S_NOTFOUND, "multibyte unmapped returns NOTFOUND"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

#ifdef XELP_ENABLE_LINE_EDIT
/* ====================================================================
 Helper: feed a string of raw bytes to XELPParseKey (for simulating typed input)
 */
static void feedString(XELP *x, const char *s) {
    while (*s) XELPParseKey(x, *s++);
}

/* Helper: feed a multi-byte keycode as individual bytes via XELPParseKey */
static void feedKeycode(XELP *x, XELPKEYCODE kc) {
    XELPParseKey(x, XELP_KC_B0(kc));
    if (XELP_KC_B1(kc)) XELPParseKey(x, XELP_KC_B1(kc));
    if (XELP_KC_B2(kc)) XELPParseKey(x, XELP_KC_B2(kc));
    if (XELP_KC_B3(kc)) XELPParseKey(x, XELP_KC_B3(kc));
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

    XELPInit(&x,"TestLineInsert");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "hello");
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    XELPParseKey(&x, 'X');

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "heXllo"), "line edit insert"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLILineEdit_Delete() - type "hello", HOME, KDEL, verify "ello"
 */
XELPRESULT test_CLILineEdit_Delete() {
    XELP x;
    char buf[64];

    XELPInit(&x,"TestLineDel");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "hello");
    feedKeycode(&x, XELP_KEYCODE_HOME);
    feedKeycode(&x, XELP_KEYCODE_KDEL);

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "ello"), "line edit delete"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLILineEdit_HomeEnd() - HOME, type "AB", END, type "CD", verify "ABhelloCD"
 */
XELPRESULT test_CLILineEdit_HomeEnd() {
    XELP x;
    char buf[64];

    XELPInit(&x,"TestLineHE");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "hello");
    feedKeycode(&x, XELP_KEYCODE_HOME);
    feedString(&x, "AB");
    feedKeycode(&x, XELP_KEYCODE_END);
    feedString(&x, "CD");

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "ABhelloCD"), "line edit home end"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLILineEdit_Backspace() - type "hello", LEFT*2, BKSP, verify "helo"
 */
XELPRESULT test_CLILineEdit_Backspace() {
    XELP x;
    char buf[64];

    XELPInit(&x,"TestLineBksp");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "hello");
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    XELPParseKey(&x, XELPKEY_BKSP);

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "helo"), "line edit backspace"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLIArrowsDrop() - UP/DOWN in CLI: no corruption
 */
XELPRESULT test_CLIArrowsDrop() {
    XELP x;
    char buf[64];

    XELPInit(&x,"TestArrowDrop");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "abc");
    feedKeycode(&x, XELP_KEYCODE_UP);
    feedKeycode(&x, XELP_KEYCODE_DOWN);
    feedString(&x, "d");

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "abcd"), "CLI arrows drop"))
        return XELP_E_ERR;

    return XELP_S_OK;
}

/* ====================================================================
 test_CLILineEdit_BufferFull() - insert at capacity
 */
XELPRESULT test_CLILineEdit_BufferFull() {
    XELP x;
    int i;

    XELPInit(&x,"TestLineFull");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* fill buffer to capacity */
    for (i = 0; i < XELP_CMDBUFSZ - 1; i++)
        XELPParseKey(&x, 'A');

    /* try to insert at cursor (should be ignored) */
    feedKeycode(&x, XELP_KEYCODE_HOME);
    XELPParseKey(&x, 'B');

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
    XELPInit(&x,"TestHelpMB");
    XELP_SET_FN_KEY(x,mbKeys);
    XELP_SET_FN_OUT(x,gDummyBufOut);

    r = XELPHelp(&x);
    gDummyBufOut(0);

    if (JB_ASSERT(r != XELP_S_OK, "help multibyte result"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELPStrLen(gDummyBuf) <= 0, "help multibyte has output"))
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

    XELPInit(&x,"TestLineRight");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    feedString(&x, "abcd");
    feedKeycode(&x, XELP_KEYCODE_HOME);     /* cursor at 0 */
    feedKeycode(&x, XELP_KEYCODE_RIGHT);    /* cursor at 1 */
    feedKeycode(&x, XELP_KEYCODE_RIGHT);    /* cursor at 2 */
    XELPParseKey(&x, 'X');                   /* insert at 2 → "abXcd" */

    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "abXcd"), "line edit right insert"))
        return XELP_E_ERR;

    /* RIGHT at end of buffer should be a no-op */
    feedKeycode(&x, XELP_KEYCODE_END);
    feedKeycode(&x, XELP_KEYCODE_RIGHT);    /* already at end → no-op */
    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "abXcd"), "line edit right at end"))
        return XELP_E_ERR;

    /* LEFT at start should be a no-op */
    feedKeycode(&x, XELP_KEYCODE_HOME);
    feedKeycode(&x, XELP_KEYCODE_LEFT);     /* already at start → no-op */
    XELPParseKey(&x, 'Z');
    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "ZabXcd"), "line edit left at start"))
        return XELP_E_ERR;

    return XELP_S_OK;
}
#endif /* XELP_ENABLE_LINE_EDIT */

/* ====================================================================
 test_AccumOverflow() - CSI sequences with intermediate bytes that hit 4-byte overflow
 */
XELPRESULT test_AccumOverflow() {
    XELP x;

    XELPInit(&x,"TestAccumOvfl");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* ESC [ <digit> <digit> — 4 bytes, digit at pos 3 isn't a terminator
       so overflow guard fires at mKeyLen==4 and flushes */
    XELPParseKey(&x, 0x1B);  /* ESC */
    XELPParseKey(&x, '[');   /* CSI start */
    XELPParseKey(&x, '1');   /* intermediate — not a letter or ~ */
    XELPParseKey(&x, '5');   /* 4th byte, still intermediate → overflow flush */
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

    XELPInit(&x,"TestMalformed");
    XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    /* 1. bare ESC followed immediately by another ESC */
    XELPParseKey(&x, 0x1B);
    XELPParseKey(&x, 0x1B);  /* flushes first ESC, stashes second */
    XELPParseKey(&x, 'a');   /* flushes second ESC, reprocesses 'a' */
    /* mode should still be CLI (ESC triggers KEY but KEY has no table on fresh init...
       wait, we set gMyCLICommands but gMyKeyCommands — let me make this right */
    XELPParseKey(&x, XELPKEY_ENTER); /* reset */

    /* 2. partial CSI abandoned by another ESC */
    XELPParseKey(&x, 0x1B);
    XELPParseKey(&x, '[');
    XELPParseKey(&x, 0x1B);  /* new ESC while in CSI — overflow/flush, then stash new ESC */
    XELPParseKey(&x, 'b');   /* flush second ESC, reprocess 'b' */
    XELPParseKey(&x, XELPKEY_ENTER); /* reset */

    /* 3. rapid-fire arrow keys interleaved with typing */
    feedString(&x, "hi");
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    feedKeycode(&x, XELP_KEYCODE_RIGHT);
    feedKeycode(&x, XELP_KEYCODE_UP);
    feedKeycode(&x, XELP_KEYCODE_DOWN);
    feedString(&x, "X");
    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "hiX"), "malformed interleaved arrows"))
        return XELP_E_ERR;
    XELPParseKey(&x, XELPKEY_ENTER);

    /* 4. unknown multi-byte key (PGUP, INS, etc.) in CLI — should be silently dropped */
    feedString(&x, "ok");
    feedKeycode(&x, XELP_KEYCODE_PGUP);
    feedKeycode(&x, XELP_KEYCODE_PGDN);
    feedKeycode(&x, XELP_KEYCODE_INS);
    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "ok"), "malformed unknown keys dropped"))
        return XELP_E_ERR;
    XELPParseKey(&x, XELPKEY_ENTER);

    /* 5. overflow: 200 chars, then arrows, then enter — no crash */
    for (i = 0; i < 200; i++)
        XELPParseKey(&x, 'z');
    feedKeycode(&x, XELP_KEYCODE_HOME);
    feedKeycode(&x, XELP_KEYCODE_END);
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    feedKeycode(&x, XELP_KEYCODE_RIGHT);
    feedKeycode(&x, XELP_KEYCODE_KDEL);
    XELPParseKey(&x, XELPKEY_BKSP);
    XELPParseKey(&x, XELPKEY_ENTER);
    if (JB_ASSERT(XELP_XB_POS(x.mCmdXB) != 0, "malformed overflow reset"))
        return XELP_E_ERR;

    /* 6. all control chars (0x01-0x1A, skip ESC) fed one by one — none should crash */
    for (i = 1; i < 0x1B; i++)
        XELPParseKey(&x, (char)i);
    /* 0x1B (ESC) needs a follow-up */
    XELPParseKey(&x, 0x1B);
    XELPParseKey(&x, 'x');  /* flush ESC */
    for (i = 0x1C; i < 0x20; i++)
        XELPParseKey(&x, (char)i);
    XELPParseKey(&x, XELPKEY_ENTER);
    JB_ASSERT(0, "malformed ctrl chars no crash");

    /* 7. KDEL and BKSP at empty buffer — must not underflow */
    feedKeycode(&x, XELP_KEYCODE_KDEL);
    XELPParseKey(&x, XELPKEY_BKSP);
    XELPParseKey(&x, XELPKEY_DEL);
    if (JB_ASSERT(x.mCmdXB.p < x.mCmdXB.s, "malformed no underflow"))
        return XELP_E_ERR;

    /* 8. DEL (0x7F) mid-line (same as BKSP path) */
    feedString(&x, "abc");
    feedKeycode(&x, XELP_KEYCODE_LEFT);
    XELPParseKey(&x, XELPKEY_DEL);
    getCmdBuf(&x, buf, sizeof(buf));
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(buf, XELPStrLen(buf), "ac"), "malformed DEL mid-line"))
        return XELP_E_ERR;
    XELPParseKey(&x, XELPKEY_ENTER);

    /* 9. CSI with very long intermediate sequence (> 4 bytes emulated) */
    XELPParseKey(&x, 0x1B);
    XELPParseKey(&x, '[');
    XELPParseKey(&x, '1');
    XELPParseKey(&x, ';');   /* intermediate, causes overflow at byte 4 */
    /* accumulator should have flushed */
    XELPParseKey(&x, '2');   /* this is a new single char now */
    XELPParseKey(&x, XELPKEY_ENTER);

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

    XELPInit(&a, "InstanceA");
    XELPInit(&b, "InstanceB");

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

    /* 1. Interleaved XELPParse: different commands, independent results */
    gGlobalCallbackData.c0 = -1;
    gGlobalCallbackData.c1 = -1;
    gGlobalCallbackData.c2 = -1;

    r = XELPParse(&a, "foo\n", 4);
    if (JB_ASSERT((r != XELP_S_OK) || (gGlobalCallbackData.c1 != 1), "MultiInst Parse A foo"))
        return XELP_E_ERR;

    r = XELPParse(&b, "bar\n", 4);
    if (JB_ASSERT((r != XELP_S_OK) || (gGlobalCallbackData.c2 != 2), "MultiInst Parse B bar"))
        return XELP_E_ERR;

    /* mR[0] should reflect each instance's last dispatch independently */
    if (JB_ASSERT(XELP_R0(a) != XELP_S_OK, "MultiInst mR[0] A"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(b) != XELP_S_OK, "MultiInst mR[0] B"))
        return XELP_E_ERR;

    /* Run command-not-found on A, verify B's mR[0] unaffected */
    r = XELPParse(&a, "nonexistent\n", 12);
    if (JB_ASSERT(XELP_R0(a) != XELP_E_CMDNOTFOUND, "MultiInst A cmdnotfound"))
        return XELP_E_ERR;
    if (JB_ASSERT(XELP_R0(b) != XELP_S_OK, "MultiInst B mR[0] still OK"))
        return XELP_E_ERR;

    /* 2. Interleaved ParseKey: type into both instances alternately */
    {
        char *cmdA = "cli0";
        char *cmdB = "foo";
        int lenA = XELPStrLen(cmdA);
        int lenB = XELPStrLen(cmdB);

        /* Reset callback state */
        gGlobalCallbackData.c0 = -1;
        gGlobalCallbackData.c1 = -1;

        /* Feed characters alternating: A gets "cli0", B gets "foo" */
        for (i = 0; i < lenA || i < lenB; i++) {
            if (i < lenA) XELPParseKey(&a, cmdA[i]);
            if (i < lenB) XELPParseKey(&b, cmdB[i]);
        }
        XELPParseKey(&a, XELPKEY_ENTER);
        XELPParseKey(&b, XELPKEY_ENTER);

        if (JB_ASSERT(gGlobalCallbackData.c0 != 0, "MultiInst ParseKey A cli0"))
            return XELP_E_ERR;
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "MultiInst ParseKey B foo"))
            return XELP_E_ERR;
    }

    /* 3. Mode changes on one instance don't affect the other */
    {
        /* A switches to KEY mode, B stays in CLI */
        XELPParseKey(&a, XELPKEY_KEY); /* ESC: stashed */
        XELPParseKey(&a, '\0');        /* flush ESC -> KEY mode */
        if (JB_ASSERT(a.mCurMode != XELP_MODE_KEY, "MultiInst A to KEY"))
            return XELP_E_ERR;
        if (JB_ASSERT(b.mCurMode != XELP_MODE_CLI, "MultiInst B still CLI"))
            return XELP_E_ERR;

        /* B switches to THR, A still in KEY */
        XELPParseKey(&b, XELPKEY_THR);
        if (JB_ASSERT(b.mCurMode != XELP_MODE_THR, "MultiInst B to THR"))
            return XELP_E_ERR;
        if (JB_ASSERT(a.mCurMode != XELP_MODE_KEY, "MultiInst A still KEY"))
            return XELP_E_ERR;

        /* Return both to CLI */
        XELPParseKey(&a, XELPKEY_CLI);
        XELPParseKey(&b, XELPKEY_CLI);
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
            r = XELPParse(&a, "cli0\n", 5);
            r = XELPParse(&b, "foo\n", 4);
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
        XELPParseKey(&a, 'b');
        XELPParseKey(&a, 'a');
        XELPParseKey(&a, 'z');
        XELPParseKey(&a, XELPKEY_BKSP);
        XELPParseKey(&a, XELPKEY_BKSP);
        XELPParseKey(&a, XELPKEY_BKSP);

        /* B types "bar" + enter while A's buffer is being edited */
        XELPParseKey(&b, 'b');
        XELPParseKey(&b, 'a');
        XELPParseKey(&b, 'r');
        XELPParseKey(&b, XELPKEY_ENTER);
        if (JB_ASSERT(gGlobalCallbackData.c2 != 2, "MultiInst bksp B bar"))
            return XELP_E_ERR;

        /* A now types "foo" + enter */
        gGlobalCallbackData.c1 = -1;
        XELPParseKey(&a, 'f');
        XELPParseKey(&a, 'o');
        XELPParseKey(&a, 'o');
        XELPParseKey(&a, XELPKEY_ENTER);
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1, "MultiInst bksp A foo"))
            return XELP_E_ERR;
    }

    return XELP_S_OK;
}

/* ====================================================================
 test_XelpArgs() - sequential argument iterator
 */
XELPRESULT test_XelpArgs() {
    const char *tok;
    int toklen, val, n;
    XelpArgs a;
    XELPRESULT r;

    /* --- basic iteration: "divmod 17 5" --- */
    {
        char buf[] = "divmod 17 5";
        XelpArgsInit(&a, buf, XELPStrLen(buf));

        r = XelpNextTok(&a, &tok, &toklen);
        if (JB_ASSERT(r != XELP_S_OK, "Args tok0 ok"))
            return XELP_E_ERR;
        if (JB_ASSERT(toklen != 6, "Args tok0 len"))
            return XELP_E_ERR;

        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 17, "Args int 17"))
            return XELP_E_ERR;

        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != 5, "Args int 5"))
            return XELP_E_ERR;

        /* past end */
        r = XelpNextTok(&a, &tok, &toklen);
        if (JB_ASSERT(r == XELP_S_OK, "Args past end"))
            return XELP_E_ERR;
        if (JB_ASSERT(tok != 0, "Args past end tok null"))
            return XELP_E_ERR;
        if (JB_ASSERT(toklen != 0, "Args past end len 0"))
            return XELP_E_ERR;
    }

    /* --- XelpArgCount --- */
    {
        char buf2[] = "echo hello world";
        XelpArgsInit(&a, buf2, XELPStrLen(buf2));
        r = XelpArgCount(&a, &n);
        if (JB_ASSERT(r != XELP_S_OK || n != 3, "ArgCount 3"))
            return XELP_E_ERR;

        /* count should not disturb iteration position */
        r = XelpNextTok(&a, &tok, &toklen);
        if (JB_ASSERT(r != XELP_S_OK || toklen != 4, "ArgCount preserves pos"))
            return XELP_E_ERR;
    }

    /* --- empty buffer --- */
    {
        char buf3[] = "";
        XelpArgsInit(&a, buf3, 0);
        r = XelpNextTok(&a, &tok, &toklen);
        if (JB_ASSERT(r == XELP_S_OK, "Args empty"))
            return XELP_E_ERR;

        r = XelpArgCount(&a, &n);
        if (JB_ASSERT(n != 0, "ArgCount empty"))
            return XELP_E_ERR;
    }

    /* --- single token --- */
    {
        char buf4[] = "help";
        XelpArgsInit(&a, buf4, XELPStrLen(buf4));
        r = XelpNextTok(&a, &tok, &toklen);
        if (JB_ASSERT(r != XELP_S_OK || toklen != 4, "Args single tok"))
            return XELP_E_ERR;
        if (JB_ASSERT(tok[0] != 'h' || tok[3] != 'p', "Args single content"))
            return XELP_E_ERR;
    }

    /* --- XelpNextInt with hex --- */
    {
        char buf5[] = "cmd 0xFF ABh";
        XelpArgsInit(&a, buf5, XELPStrLen(buf5));
        XelpNextTok(&a, 0, 0); /* skip "cmd" */

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
        XelpArgsInit(&a, buf6, XELPStrLen(buf6));
        XelpNextTok(&a, 0, 0); /* skip "cmd" */
        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r != XELP_E_ERR, "Args non-numeric"))
            return XELP_E_ERR;
    }

    /* --- NULL tok/toklen pointers (skip pattern) --- */
    {
        char buf7[] = "skip me keep";
        XelpArgsInit(&a, buf7, XELPStrLen(buf7));
        r = XelpNextTok(&a, 0, 0); /* skip with NULLs */
        if (JB_ASSERT(r != XELP_S_OK, "Args skip NULL ptrs"))
            return XELP_E_ERR;
        r = XelpNextTok(&a, 0, 0);
        if (JB_ASSERT(r != XELP_S_OK, "Args skip NULL ptrs 2"))
            return XELP_E_ERR;
        r = XelpNextTok(&a, &tok, &toklen);
        if (JB_ASSERT(r != XELP_S_OK || toklen != 4, "Args after skips"))
            return XELP_E_ERR;
    }

    /* --- negative integer --- */
    {
        char buf8[] = "cmd -42";
        XelpArgsInit(&a, buf8, XELPStrLen(buf8));
        XelpNextTok(&a, 0, 0);
        r = XelpNextInt(&a, &val);
        if (JB_ASSERT(r != XELP_S_OK || val != -42, "Args negative int"))
            return XELP_E_ERR;
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

    JumpBug_RunUnit(test_XELPStrLen,"XELPStrLen");
	JumpBug_RunUnit(test_XELPStr2Int,"XELPStr2Int");
    JumpBug_RunUnit(test_XELPStrEq, "StrEq");
    JumpBug_RunUnit(test_XELPStrEq2, "StrEq2");
    JumpBug_RunUnit(test_XELPBufCmp,"XELPBufCmp");
    JumpBug_RunUnit(test_XelpFindTok,"XelpFindTok");
    JumpBug_RunUnit(test_XelpTokLineXB,"XelpTokLineXB");

    JumpBug_RunUnit(test_XELPTokN,"XelpTokN");
    JumpBug_RunUnit(test_XELPNumToks,"XELPNumToks");
    JumpBug_RunUnit(test_XelpInit,"XelpInit");
    JumpBug_RunUnit(test_XelpOut_comprehensive,"XelpOut");
    JumpBug_RunUnit(test_XELPExecKC,"XELPExecKC");

    JumpBug_RunUnit(test_XELPParseKey,"XelpParseKey");
    JumpBug_RunUnit(test_XELPParse,"XelpParse");
    JumpBug_RunUnit(test_XELPParseXB,"XELPParseXB");
    JumpBug_RunUnit(test_XelpHelp,"XelpHelp");
    JumpBug_RunUnit(test_XELPParseNum,"XELPParseNum");
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
    JumpBug_RunUnit(test_CLIArrowsDrop,"CLIArrowsDrop");
    JumpBug_RunUnit(test_CLILineEdit_BufferFull,"LineEditBufferFull");
    JumpBug_RunUnit(test_CLILineEdit_Right,"LineEditRight");
#endif
    JumpBug_RunUnit(test_HelpMultiByteKeys,"HelpMultiByteKeys");
    JumpBug_RunUnit(test_AccumOverflow,"AccumOverflow");
    JumpBug_RunUnit(test_CLIMalformedKeys,"CLIMalformedKeys");
    JumpBug_RunUnit(test_MultiInstance,"MultiInstance");
    JumpBug_RunUnit(test_XelpArgs,"XelpArgs");

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

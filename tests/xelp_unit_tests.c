/**************************************************************************************************

  @xelp_unit_tests.c - simple test file example file for xelp embedded cli/scripting library
  @copy Copyright (C)   <M. A. Chatterjee>
  @author M A Chatterjee <deftio [at] deftio [dot] com>
   
  @license: 
	Copyright (c) 2011-2019, M. A. Chatterjee <deftio at deftio dot com>
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

//=======================================================
//esc or single-key mode commads
struct {
    int k0;
    int k1;
    int k2;
    int c0;
    int c1;
    int c2;
}gGlobalCallbackData;

XELPRESULT k0 (int k) {
    gGlobalCallbackData.k0 = k;
    return XELP_S_OK;
}
XELPRESULT k1 (int k) {
    gGlobalCallbackData.k1 = k;
    return XELP_S_OK;
}
XELPRESULT k2 (int k) {
    gGlobalCallbackData.k2 = k;
    return XELP_S_OK;
}

//declare static map for function in single key mode
XELPKeyFuncMapEntry gMyKeyCommands[] =
{
	{&k0       ,'0', "sets k0"       },
	{&k1       ,'1', "sets k1"       },
	{&k2       ,'2', "sets k2"       },
	XELP_FUNC_ENTRY_LAST
};

XELPRESULT cli0 (const char *c, int max) {
    gGlobalCallbackData.c0 = 0;
    return XELP_S_OK;
}
XELPRESULT cli1 (const char *c, int max) {
    gGlobalCallbackData.c1 = 1;
    return XELP_S_OK;
}
XELPRESULT cli2 (const char *c, int max) {
    gGlobalCallbackData.c2 = 2;
    return XELP_S_OK;
}
XELPRESULT cli3 (const char *c, int max) {
    gGlobalCallbackData.c0 = 0;
    gGlobalCallbackData.c1 = 0;
    gGlobalCallbackData.c2 = 0;
    return XELP_S_OK;
}
//declare a command map for functions in parse mode
XELPCLIFuncMapEntry gMyCLICommands[] = {
	{&cli0    		    , "cli0",  "cli function 0   "       },
	{&cli1    		    , "foo"	,  "foo cli function "       },
    {&cli2    		    , "bar"	,  "bar cli function "       },
    {&cli3              , "rst" ,  "reset all cli global test vars to zero"},
	XELP_FUNC_ENTRY_LAST
};

/*************************************************
Unit Test Cases for XELP() functions below
*/

/* ====================================================================
 test_XELPStrLen()
 */
XELPRESULT test_XELPStrLen() {
    
    if (JB_ASSERT(3 != XELPStrLen("abc"),"XelpStrLen")) 
        return XELP_E_Err;

    if (JB_ASSERT(0 != XELPStrLen(""),"XelpStrLen")) 
        return XELP_E_Err;

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
        return XELP_E_Err;
    
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(a,alen,b+1),"XELPStrEq offset")) 
        return XELP_E_Err;
    

    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq(a,alen,c),"XELPStrEq 3")) 
        return XELP_E_Err;
    
    
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(c,alen,a),"XELPStrEq 4")) 
        return XELP_E_Err;
    

    alen = XELPStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XELPStrEq(c,alen,a),"XELPStrEq len test")) 
        return XELP_E_Err;

    if (JB_ASSERT(XELP_S_OK != XELPStrEq(c,0,d),"XELPStrEq zero len test")) 
        return XELP_E_Err;

    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq(c,0,b),"XELPStrEq zero len test")) 
        return XELP_E_Err;

    return XELP_S_OK;
}
/* ====================================================================
 test_XELPStrEq2()
  
   XELPStrEq is used for comparing length-limited char buffers to null 
   terminated strings such as command names.  It uses an end ptr instead
   of a integer length for determining the buffer end.

   XELPStrEq (const char* pbuf, const char* pend, const char *cmd)
 */
XELPRESULT test_XELPStrEq2() {
    char *a = "token1", *ae;
    char *b = " token1\0abc";
    char *c = "token1abc";
    char *d = "";
    
    ae = a+ XELPStrLen(a);
    
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq2(a,ae,b),"XELPStrEq2 t1" ))
        return XELP_E_Err;
    
    if (JB_ASSERT(XELP_S_OK != XELPStrEq2(a,ae,b+1),"XELPStrEq2 offset")) 
        return XELP_E_Err;
    

    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq2(a,ae,c),"XELPStrEq2")) 
        return XELP_E_Err;
    
    
    if (JB_ASSERT(XELP_S_OK != XELPStrEq2(c,c+XELPStrLen(a),a),"XELPStrEq2")) 
        return XELP_E_Err;
    
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPStrEq2(c,c+XELPStrLen(a),d),"XELPStrEq2 null start")) 
        return XELP_E_Err;
    
    return XELP_S_OK;
}
/* ====================================================================
 test_XELPStr2Int()
 */

XELPRESULT test_XELPStr2Int() {
	if (JB_ASSERT(XELPStr2Int("90",2) != 90,"Str2Int  90"))
		return XELP_E_Err;

    if (JB_ASSERT(XELPStr2Int("31h",3) != 49,"Str2Int 31h")) // hex parser
		return XELP_E_Err;

    if (JB_ASSERT(XELPStr2Int("-87",3) != -87,"Str2Int -87")) // hex parser
		return XELP_E_Err;

    if (JB_ASSERT(XELPStr2Int("+6546",5) != 6546,"Str2Int +6546")) // hex parser
		return XELP_E_Err;

	return XELP_S_OK;
}
/* ====================================================================
 test_XelpParseNum()
 */

XELPRESULT test_XelpParseNum() {
    int n;
    XELPRESULT r;
    

  
    r = XelpParseNum("90",2, &n); 
    if (JB_ASSERT( ((n != 90) || ( r != XELP_S_OK)) ,"XELPParseNum 90"))
        return XELP_E_Err;



    r = XelpParseNum("3ab30h",6, &n); 
    if (JB_ASSERT( (n != 0x3ab30) || ( r != XELP_S_OK) ,"XELPParseNum 3ab30h"))
        return XELP_E_Err;


    r = XelpParseNum("0x3ab30",7, &n); 
    if (JB_ASSERT( (n != 0x3ab30) || ( r != XELP_S_OK) ,"XELPParseNum 0x3ab30"))
        return XELP_E_Err;

    
    r = XelpParseNum("-87",3, &n); 
    if (JB_ASSERT( (n !=  -87) || ( r != XELP_S_OK) ,"XELPParseNum -87"))
        return XELP_E_Err;


    r = XelpParseNum("+6457",5, &n); 
    if (JB_ASSERT( (n != 6457) || ( r != XELP_S_OK) ,"XELPParseNum +6547"))
       { return XELP_E_Err;}

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

    ae = a + XELPStrLen(a);
    be = b + XELPStrLen(b);
    ce = c + XELPStrLen(a);
    de = c + XELPStrLen(d);
    if (JB_ASSERT(XELP_S_NOTFOUND != XelpBufCmp(a,ae,b,be,XELP_CMP_TYPE_A0B0),"XelpBufCmp" ))
        return XELP_E_Err;
    
    be = b+1+XELPStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XelpBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_A0B0),"XelpBufCmp")) 
        return XELP_E_Err;
    

    be = b+2+XELPStrLen(b+1);
    if (JB_ASSERT(XELP_S_NOTFOUND != XelpBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_BUF),"XelpBufCmp")) 
        return XELP_E_Err;
    
    ce = c+XELPStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_BUF),"XelpBufCmp")) 
        return XELP_E_Err;
    
    ce = c+XELPStrLen(a)+1;
    if (JB_ASSERT(XELP_S_NOTFOUND != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_BUF),"XelpBufCmp")) 
        return XELP_E_Err;


    if (JB_ASSERT(XELP_S_NOTFOUND != XelpBufCmp(d,de+2,a,ae,XELP_CMP_TYPE_A0),"XelpBufCmp A01")) 
        return XELP_E_Err;

    ae = a + XELPStrLen(a);
    ce = c + XELPStrLen(a);
    if (JB_ASSERT(XELP_S_OK != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_A0),"XelpBufCmp A02")) 
        return XELP_E_Err;

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
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok" ) )
        return XELP_E_Err;

    x.s = b1;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (JB_ASSERT(XELP_S_OK != XELPFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok" ) )
        return XELP_E_Err;

    x.s = b2;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (JB_ASSERT(XELP_S_OK != XELPFindTok(&x,label,le,XELP_TOK_LINE),"XelpFindTok" ) )
        return XELP_E_Err;

    x.s = b3;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (JB_ASSERT(XELP_S_NOTFOUND != XELPFindTok(&x,label,le,XELP_TOK_LINE),"XelpFindTok" ) )
        return XELP_E_Err;


    return XELP_S_OK;
}
/* ====================================================================
 test_XelpTokLineXB()
 */
XELPRESULT test_XelpTokLineXB() {

    char *line1 = "abc def ghi";
    XelpBuf b,out;
    XELPRESULT r,r2;

    //=====
    XELP_XBInit(b,line1,XELPStrLen(line1));
    r  = XELPTokLineXB(&b,&out,XELP_TOK_ONLY);
    r2 = XelpBufCmp(line1, line1+3,out.s,out.p,XELP_CMP_TYPE_BUF);

    if (JB_ASSERT((XELP_S_OK !=r) || (XELP_S_OK != r2),"XelpToklineXB first token"))
        return XELP_E_Err;
    XELP_XBTOP(b);
    

    return XELP_S_OK;
}
/* ====================================================================
 test_XelpInit()
 */

XELPRESULT test_XelpInit() {
    XELP myXelp, *x;
    x = &myXelp;

    if (JB_ASSERT(XELP_S_OK != XELPInit(x,"Xelp Unit Tests"),"XelpInit")) {
        return XELP_E_Err;
    }
    
    return XELP_S_OK;
}

char gChar;
void dummyOut(char c) {gChar = c;}

int gInt;
void dummyIntOut(int i) {gInt = i;}

int gBool;
void dummyVoid0() {gBool = 0;}
void dummyVoid1() {gBool = 1;}

XELPRESULT test_XelpOut_XelpThru_XelpErr() {
    
    XELP myXelp;
    XELPInit(&myXelp,"XelpOut Tests");
    
    if (JB_ASSERT(XELP_S_OK != XELPOut(&myXelp,0,0),"XelpOut test XOut FN not set")) {
        return XELP_E_Err;
    }

    XELP_SET_FN_OUT(myXelp,dummyOut);
    XELP_SET_FN_THR(myXelp,dummyOut);
    XELP_SET_FN_ERR(myXelp,dummyOut);

    if (JB_ASSERT(XELP_S_OK != XELPOut(&myXelp,"a",1),"XelpOut")) {
        return XELP_E_Err;
        if (gChar != 'a')
            return XELP_E_Err;
    }

    if (JB_ASSERT(XELP_S_OK != XELPOut(&myXelp,"ab",2),"XelpOut")) {
        return XELP_E_Err;
        if (gChar != 'b')
            return XELP_E_Err;
    }

    if (JB_ASSERT(XELP_S_OK != XELPOut(&myXelp,0,0),"XelpOut test NULL msg")) {
        return XELP_E_Err;
    }
    return XELP_S_OK;
}

/* ====================================================================
 test_XelpHelp()
 */
#define GDUMMYBUFLEN (0x1000)

char gDummyBuf[GDUMMYBUFLEN];
XelpBuf gDummyXelpBuf;


void gDummyBufOut(char c) {
    XELP_XBPUTC(gDummyXelpBuf,c);
}

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

    XELP_XBInit(gDummyXelpBuf,gDummyBuf,GDUMMYBUFLEN);
    XELPInit(&x,"Test XelpHelp");

    XELP_SET_FN_KEY(x,keyCmds);
    XELP_SET_FN_CLI(x,cliCmds);
	XELP_SET_FN_OUT(x,gDummyBufOut);

    r = XELPHelp(&x);
    gDummyBufOut(0);
    
    if (JB_ASSERT( (r!= XELP_S_OK) || ( XELPStrLen(gDummyBuf) != 149), "Test Help" )) {
        return XELP_E_Err;
    }
    return XELP_S_OK;
}
/* ====================================================================
 test_XELPExecKC()
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

    r = XELPExecKC(&x,'1');
    if (JB_ASSERT((r!=XELP_S_OK)&&(gGlobalCallbackData.c1!='1'),"ExecKC '1' ")){
        // printf("r=%d\n",r);
        return r;
    }

    r = XELPExecKC(&x,'z'); // not a mapped key...
    if (JB_ASSERT(r!=XELP_S_NOTFOUND,"ExecKC 'z'")){
        return r;
    }
    
    return XELP_S_OK;
    
}
/* ====================================================================
 test_XELPParseKey()
 */
XELPRESULT test_XELPParseKey() {
    XELP x;
    XELPRESULT r;
    int i;
  
    r = XELPInit(&x,"TestParseKey");
    XELP_SET_FN_KEY(x,gMyKeyCommands);
	XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    //begin actual test
    {
        char *c1 = " foo ";
        for (i=0; i  <XELPStrLen(c1); i++) {
            r = XELPParseKey(&x,c1[i]);
            if (JB_ASSERT(r!= XELP_S_OK, "XELPParseKey -- sending keys")){
                return XELP_E_Err;
            }
        }
        r = XELPParseKey(&x,XELPKEY_ENTER);
            if (JB_ASSERT(r!= XELP_S_OK, "XELPParseKey -- sending keys")){
                return XELP_E_Err;
            }
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1,"Test cli 1 value")) {
            return XELP_E_Err;
        }
    }
    
    //begin actual test
    {
        char *c2 = " bar; ";
        for (i=0; i  <XELPStrLen(c2); i++) {
            r = XELPParseKey(&x,c2[i]);
            if (JB_ASSERT(r!= XELP_S_OK, "XELPParseKey -- sending keys")){
                return XELP_E_Err;
            }
        }
        r = XELPParseKey(&x,XELPKEY_BKSP);
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT(r!= XELP_S_OK, "XELPParseKey -- sending keys w bskp test")){
            return XELP_E_Err;
        }
        if (JB_ASSERT(gGlobalCallbackData.c1 != 1,"XELPParseKey test cli1 value")) {
            return XELP_E_Err;
        }


        XELP_SET_FN_BKSP(x, dummyVoid1);
        dummyVoid0();
        r = XELPParseKey(&x,XELPKEY_CLI);
        r = XELPParseKey(&x,'a');
        r = XELPParseKey(&x,XELPKEY_BKSP);
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (JB_ASSERT( (r!= XELP_S_OK) || (gBool != 1), "XELPParseKey --  bskp callback test")){
            return XELP_E_Err;
        }

    }

    //test mode changes...
    {
        XELP_SET_FN_EMCHG(x,0);
        
        r = XELPParseKey(&x,XELPKEY_CLI);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI), "XELPParseKey -- mode change to CLI 1")){
            return XELP_E_Err;
        }
        r = XELPParseKey(&x,XELPKEY_KEY);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY), "XELPParseKey -- mode change to KEY")){
            return XELP_E_Err;
        }

        XELP_SET_FN_EMCHG(x,dummyIntOut); // set up test for mode-change callback fn
        XELP_SET_FN_THR(x,dummyOut); // can only change to THR mode if there is a valid call back fn.

        r = XELPParseKey(&x,XELPKEY_THR);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gInt != x.mCurMode), "XELPParseKey -- mode change to THR")){
            return XELP_E_Err;
        }

        r = XELPParseKey(&x,XELPKEY_CLI);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gInt != x.mCurMode), "XELPParseKey -- mode change to CLI 2")){
            return XELP_E_Err;
        }
        
        XELP_SET_FN_EMCHG(x,0);
    }

    //test THR function redirects
    {
        XELP_SET_FN_EMCHG(x,0); // don't want to deal with mode-chnage call backs
        
        r = XELPParseKey(&x,XELPKEY_THR);
        if (JB_ASSERT( (r!= XELP_S_OK) && (x.mCurMode != XELP_MODE_THR), "XELPParseKey -- mode change to THR")){
            return XELP_E_Err;
        }

        XELP_SET_FN_THR(x,dummyOut);
        r = XELPParseKey(&x,'a');
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gChar != 'a'), "XELPParseKey --  THR 1")){
            return XELP_E_Err;
        }
        r = XELPParseKey(&x,'b');
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gChar != 'b'), "XELPParseKey --  THR 2")){
            return XELP_E_Err;
        }
        r = XELPParseKey(&x,XELPKEY_CLI); // change to CLI now THRU function shouldn't be called and dummy value should be unchnaged
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gChar != 'b'), "XELPParseKey --  THR x")){
            return XELP_E_Err;
        }

    }

    //test KEY function redirects
    {
        r = XELPParseKey(&x,XELPKEY_KEY);
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY) , "XELPParseKey --  KEY 1")){
            return XELP_E_Err;
        }

        gGlobalCallbackData.k0 = 'x';
        r = XELPParseKey(&x,'0');
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY) || (gGlobalCallbackData.k0 != '0'), "XELPParseKey --  THR 0")){
            return XELP_E_Err;
        }
        
        gGlobalCallbackData.k1 = 'y';
        r = XELPParseKey(&x,'1');
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY) || (gGlobalCallbackData.k1 != '1'), "XELPParseKey --  THR 1")){
            return XELP_E_Err;
        }

        // end of tests ... change mode to CLI and we shouldn't be getting key callbacks anymore
        gGlobalCallbackData.k1 = 'z';
        r = XELPParseKey(&x,XELPKEY_CLI); // change to CLI now THRU function shouldn't be called and dummy value should be unchnaged
        if (JB_ASSERT( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gGlobalCallbackData.k1  != 'z'), "XELPParseKey--  THR x")){
            return XELP_E_Err;
        }

    }

    
    return XELP_S_OK;

}




/* ====================================================================
 test_XELPTokN()
 XELPRESULT XELPTokN (XelpBuf *b, int *n)
 */

XELPRESULT test_XELPTokN() {
    XelpBuf x, tok;
    XELPRESULT r;
    char *c1 =   "tok0 tok1 tok2    \t tok3   tok4\n tok5";
    char *c2 = "\ttok0 tok1 tok2    \t # tok3   tok4\n tok5 "; // test with comment
    char *c3 = " tok0 tok1 tok2    \t # tok3   tok4;\n tok5; tok6 "; // test with comment
    char *c4 = " tok0 tok1 tok2    \t #tok3   tok4;\n tok5; tok6 "; // test with comment hugging token
    
    XELP_XBInit(x,c1,XELPStrLen(c1));   
    r = XELPTokN(&x,0,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok0") )),"XELPTokN get 0th token"))
        return XELP_E_Err;

    r = XELPTokN(&x,3,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok3") )),"XELPTokN get 3rd token"))
        return XELP_E_Err;


    XELP_XBInit(x,c2,XELPStrLen(c2));   
    r = XELPTokN(&x,3,&tok);
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get 3rd token w commented line"))
        return XELP_E_Err;

    r = XELPTokN(&x,3,&tok);
    XELP_XBInit(x,c2,XELPStrLen(c3));   
    if (JB_ASSERT( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get 3rd token w commented line"))
        return XELP_E_Err;


    XELP_XBInit(x,c4,XELPStrLen(c4));   
    r = XELPTokN(&x,3,&tok);
    if (JB_ASSERT( ((r != XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get 3rd token w commented line w space"))
        return XELP_E_Err;

    XELP_XBInit(x,c4,XELPStrLen(c4));   
    r = XELPTokN(&x,23,&tok);
    if (JB_ASSERT( ((r == XELP_S_OK) || (XELP_S_NOTFOUND != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get token past buffer"))
        return XELP_E_Err;

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
    char *c2 = "\t tok1 tok2    \t# tok3   tok4\n t0k5; tok6"; // test with comment
    char *c3 = "\t tok1 tok2    \t#tok3   tok4\n t0k5; tok6";  // test with comment

    XELP_XBInit(x,c0,XELPStrLen(c0));
    r = XelpNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=0)),"XelpNumToks tabs and newlines"))
        return XELP_E_Err;

    XELP_XBInit(x,c1,XELPStrLen(c1));
    r = XelpNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=5)),"XelpNumToks tabs and newlines"))
        return XELP_E_Err;

    XELP_XBInit(x,c2,XELPStrLen(c2));
    r = XelpNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=4)),"XelpNumToks comment on second line"))
        return XELP_E_Err;

    XELP_XBInit(x,c3,XELPStrLen(c3));
    r = XelpNumToks(&x,&n);
    if (JB_ASSERT(((r!=XELP_S_OK) || (n !=4)),"XelpNumToks comment on second line"))
        return XELP_E_Err;
    return XELP_S_OK;
}
/* ====================================================================
 test_XELPParseXB()
 */

XELPRESULT test_XELPParseXB() {
    XELP x;

    XELPInit(&x,"TestParseXB");
    return XELP_S_OK;
}
/* ====================================================================
 test_XELPParse()
 */

XELPRESULT test_XELPParse() {
    XELP x;
    char *s = "foo ";
    XELPRESULT r;
    
    XELPInit(&x,"TestParse");
    XELP_SET_FN_OUT(x,dummyOut);

    r = XELPParse(&x,s,XELPStrLen(s));
    
    if (JB_ASSERT(r!=XELP_S_OK,"XELPParse"))
        return XELP_E_Err;

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
    
    
    
    JumpBug_InitGlobal("Xelp", putcharc,flogout); // initialize the test case counters

    JumpBug_RunUnit(test_XELPStrLen,"XELPStrLen");
	JumpBug_RunUnit(test_XELPStr2Int,"XELPStr2Int");
    JumpBug_RunUnit(test_XELPStrEq, "StrEq");
    JumpBug_RunUnit(test_XELPStrEq2, "StrEq2");
    JumpBug_RunUnit(test_XelpBufCmp,"XelpBufCmp");
    JumpBug_RunUnit(test_XelpFindTok,"XelpFindTOk");
    JumpBug_RunUnit(test_XelpTokLineXB,"XelpTokLineXB");
    
    JumpBug_RunUnit(test_XELPTokN,"XelpTokN");
    //Problem in comment handling

    JumpBug_RunUnit(test_XelpNumToks,"XelpNumToks");
    JumpBug_RunUnit(test_XelpInit,"XelpInit");
    JumpBug_RunUnit(test_XelpOut_XelpThru_XelpErr,"XelpOut_XelpThru_XelpErr");
    JumpBug_RunUnit(test_XELPExecKC,"XELPExecKC");
    
    JumpBug_RunUnit(test_XELPParseKey,"XelpParseKey");
    JumpBug_RunUnit(test_XELPParse,"XelpParse");
    JumpBug_RunUnit(test_XELPParseXB,"XELPParseXB");
    JumpBug_RunUnit(test_XelpHelp,"XelpHelp");
    JumpBug_RunUnit(test_XelpParseNum,"XelpParseNum");
   
    JumpBug_PrintResults();


    
	return JumpBug_BuildPass(); // return whether we passed for CI purposes.  Modify gBuildPass() if there is a diff way to report build pass 
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
    
    //print size of the JumpBug test framework
    //printf("JumpBug..\n size of JumpBug Instance %d (bytes)\n===> %s, %d <===\n",(int)sizeof(JB_UnitTestData),JUMPBUG_DBG_FILE, JUMPBUG_DBG_LINE);
    return result;  /* remember the value 0 is considered passing in a *nix build continuous integration sense */

}
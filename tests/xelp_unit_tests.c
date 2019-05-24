/*

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
#include "xelp_simple_unit_test_fw.h"
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
XELPCLIFuncMapEntry gMyCLICommands[] =
{
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
    
    if (LOGTEST(3 != XELPStrLen("abc"),"XelpStrLen")) 
        return XELP_E_Err;

    if (LOGTEST(0 != XELPStrLen(""),"XelpStrLen")) 
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
    
    if (LOGTEST(XELP_S_NOTFOUND != XELPStrEq(a,alen,b),"XELPStrEq" ))
        return XELP_E_Err;
    
    if (LOGTEST(XELP_S_OK != XELPStrEq(a,alen,b+1),"XELPStrEq offset")) 
        return XELP_E_Err;
    

    if (LOGTEST(XELP_S_NOTFOUND != XELPStrEq(a,alen,c),"XELPStrEq 3")) 
        return XELP_E_Err;
    
    
    if (LOGTEST(XELP_S_OK != XELPStrEq(c,alen,a),"XELPStrEq 4")) 
        return XELP_E_Err;
    

    alen = XELPStrLen(a);
    if (LOGTEST(XELP_S_OK != XELPStrEq(c,alen,a),"XELPStrEq len test")) 
        return XELP_E_Err;

    if (LOGTEST(XELP_S_OK != XELPStrEq(c,0,d),"XELPStrEq zero len test")) 
        return XELP_E_Err;

    if (LOGTEST(XELP_S_NOTFOUND != XELPStrEq(c,0,b),"XELPStrEq zero len test")) 
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
    
    if (LOGTEST(XELP_S_NOTFOUND != XELPStrEq2(a,ae,b),"XELPStrEq2 t1" ))
        return XELP_E_Err;
    
    if (LOGTEST(XELP_S_OK != XELPStrEq2(a,ae,b+1),"XELPStrEq2 offset")) 
        return XELP_E_Err;
    

    if (LOGTEST(XELP_S_NOTFOUND != XELPStrEq2(a,ae,c),"XELPStrEq2")) 
        return XELP_E_Err;
    
    
    if (LOGTEST(XELP_S_OK != XELPStrEq2(c,c+XELPStrLen(a),a),"XELPStrEq2")) 
        return XELP_E_Err;
    
    if (LOGTEST(XELP_S_NOTFOUND != XELPStrEq2(c,c+XELPStrLen(a),d),"XELPStrEq2 null start")) 
        return XELP_E_Err;
    
    return XELP_S_OK;
}
/* ====================================================================
 test_XELPStr2Int()
 */

XELPRESULT test_XELPStr2Int() {
	if (LOGTEST(XELPStr2Int("90",2) != 90,"Str2Int"))
		return XELP_E_Err;

    if (LOGTEST(XELPStr2Int("31h",3) != 49,"Str2Int")) // hex parser
		return XELP_E_Err;

    if (LOGTEST(XELPStr2Int("-87",3) != -87,"Str2Int")) // hex parser
		return XELP_E_Err;

    if (LOGTEST(XELPStr2Int("+6546",5) != 6546,"Str2Int")) // hex parser
		return XELP_E_Err;

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
    if (LOGTEST(XELP_S_NOTFOUND != XelpBufCmp(a,ae,b,be,XELP_CMP_TYPE_A0B0),"XelpBufCmp" ))
        return XELP_E_Err;
    
    be = b+1+XELPStrLen(a);
    if (LOGTEST(XELP_S_OK != XelpBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_A0B0),"XelpBufCmp")) 
        return XELP_E_Err;
    

    be = b+2+XELPStrLen(b+1);
    if (LOGTEST(XELP_S_NOTFOUND != XelpBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_BUF),"XelpBufCmp")) 
        return XELP_E_Err;
    
    ce = c+XELPStrLen(a);
    if (LOGTEST(XELP_S_OK != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_BUF),"XelpBufCmp")) 
        return XELP_E_Err;
    
    ce = c+XELPStrLen(a)+1;
    if (LOGTEST(XELP_S_NOTFOUND != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_BUF),"XelpBufCmp")) 
        return XELP_E_Err;


    if (LOGTEST(XELP_S_NOTFOUND != XelpBufCmp(d,de+2,a,ae,XELP_CMP_TYPE_A0),"XelpBufCmp A01")) 
        return XELP_E_Err;

    ae = a + XELPStrLen(a);
    ce = c + XELPStrLen(a);
    if (LOGTEST(XELP_S_OK != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_A0),"XelpBufCmp A02")) 
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
    if (LOGTEST(XELP_S_NOTFOUND != XELPFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok" ) )
        return XELP_E_Err;

    x.s = b1;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (LOGTEST(XELP_S_OK != XELPFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok" ) )
        return XELP_E_Err;

    x.s = b2;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (LOGTEST(XELP_S_OK != XELPFindTok(&x,label,le,XELP_TOK_LINE),"XelpFindTok" ) )
        return XELP_E_Err;

    x.s = b3;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (LOGTEST(XELP_S_NOTFOUND != XELPFindTok(&x,label,le,XELP_TOK_LINE),"XelpFindTok" ) )
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

    if (LOGTEST((XELP_S_OK !=r) || (XELP_S_OK != r2),"XelpToklineXB first token"))
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

    if (LOGTEST(XELP_S_OK != XELPInit(x,"Xelp Unit Tests"),"XelpInit fail")) {
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
    
    if (LOGTEST(XELP_S_OK != XELPOut(&myXelp,0,0),"XelpOut test XOut FN not set")) {
        return XELP_E_Err;
    }

    XELP_SET_FN_OUT(myXelp,dummyOut);
    XELP_SET_FN_THR(myXelp,dummyOut);
    XELP_SET_FN_ERR(myXelp,dummyOut);

    if (LOGTEST(XELP_S_OK != XELPOut(&myXelp,"a",1),"XelpOut fail")) {
        return XELP_E_Err;
        if (gChar != 'a')
            return XELP_E_Err;
    }

    if (LOGTEST(XELP_S_OK != XELPOut(&myXelp,"ab",2),"XelpOut fail")) {
        return XELP_E_Err;
        if (gChar != 'b')
            return XELP_E_Err;
    }

    if (LOGTEST(XELP_S_OK != XELPOut(&myXelp,0,0),"XelpOut test Null msg")) {
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
    //printf("%s\n",gDummyBuf); 
    //printf("%d",XELPStrLen(gDummyBuf));
    if (LOGTEST( (r!= XELP_S_OK) || ( XELPStrLen(gDummyBuf) != 149), "Test Help fail" )) {
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
    if (LOGTEST(r!=XELP_S_NOTFOUND,"failed ExecKC null ptr")){
        return r;
    }
    
    XELP_SET_FN_KEY(x,gMyKeyCommands);
    XELP_SET_FN_CLI(x,gMyCLICommands);
	XELP_SET_FN_OUT(x,dummyOut);

    r = XELPExecKC(&x,'1');
    if (LOGTEST((r!=XELP_S_OK)&&(gGlobalCallbackData.c1!='1'),"failed ExecKC '1' ")){
        printf("r=%d\n",r);
        return r;
    }

    r = XELPExecKC(&x,'z'); // not a mapped key...
    if (LOGTEST(r!=XELP_S_NOTFOUND,"failed ExecKC 'z'")){
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
    //char c;
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
            if (LOGTEST(r!= XELP_S_OK, "Failed Parse Key -- sending keys")){
                return XELP_E_Err;
            }
        }
        r = XELPParseKey(&x,XELPKEY_ENTER);
            if (LOGTEST(r!= XELP_S_OK, "Failed Parse Key -- sending keys")){
                return XELP_E_Err;
            }
        if (LOGTEST(gGlobalCallbackData.c1 != 1,"Failed Parse Key test cli1 value")) {
            return XELP_E_Err;
        }
    }
    
    //begin actual test
    {
        char *c2 = " bar; ";
        for (i=0; i  <XELPStrLen(c2); i++) {
            r = XELPParseKey(&x,c2[i]);
            if (LOGTEST(r!= XELP_S_OK, "Failed Parse Key -- sending keys")){
                return XELP_E_Err;
            }
        }
        r = XELPParseKey(&x,XELPKEY_BKSP);
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (LOGTEST(r!= XELP_S_OK, "Failed Parse Key -- sending keys w bskp test")){
            return XELP_E_Err;
        }
        if (LOGTEST(gGlobalCallbackData.c1 != 1,"Failed Parse Key test cli1 value")) {
            return XELP_E_Err;
        }


        XELP_SET_FN_BKSP(x, dummyVoid1);
        dummyVoid0();
        r = XELPParseKey(&x,XELPKEY_CLI);
        r = XELPParseKey(&x,'a');
        r = XELPParseKey(&x,XELPKEY_BKSP);
        r = XELPParseKey(&x,XELPKEY_ENTER);
        if (LOGTEST( (r!= XELP_S_OK) || (gBool != 1), "Failed Parse Key --  bskp callback test")){
            return XELP_E_Err;
        }

    }

    //test mode changes...
    {
        XELP_SET_FN_EMCHG(x,0);
        
        r = XELPParseKey(&x,XELPKEY_CLI);
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI), "Failed Parse Key -- mode change to CLI 1")){
            return XELP_E_Err;
        }
        r = XELPParseKey(&x,XELPKEY_KEY);
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY), "Failed Parse Key -- mode change to KEY")){
            return XELP_E_Err;
        }

        XELP_SET_FN_EMCHG(x,dummyIntOut); // set up test for mode-change callback fn
        XELP_SET_FN_THR(x,dummyOut); // can only change to THR mode if there is a valid call back fn.

        r = XELPParseKey(&x,XELPKEY_THR);
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gInt != x.mCurMode), "Failed Parse Key -- mode change to THR")){
            return XELP_E_Err;
        }

        r = XELPParseKey(&x,XELPKEY_CLI);
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gInt != x.mCurMode), "Failed Parse Key -- mode change to CLI 2")){
            return XELP_E_Err;
        }
        
        XELP_SET_FN_EMCHG(x,0);
    }

    //test THR function redirects
    {
        XELP_SET_FN_EMCHG(x,0); // don't want to deal with mode-chnage call backs
        
        r = XELPParseKey(&x,XELPKEY_THR);
        if (LOGTEST( (r!= XELP_S_OK) && (x.mCurMode != XELP_MODE_THR), "Failed ParseKey -- mode change to THR")){
            return XELP_E_Err;
        }

        XELP_SET_FN_THR(x,dummyOut);
        r = XELPParseKey(&x,'a');
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gChar != 'a'), "Failed ParseKey --  THR 1")){
            return XELP_E_Err;
        }
        r = XELPParseKey(&x,'b');
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_THR) || (gChar != 'b'), "Failed ParseKey --  THR 2")){
            return XELP_E_Err;
        }
        r = XELPParseKey(&x,XELPKEY_CLI); // change to CLI now THRU function shouldn't be called and dummy value should be unchnaged
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gChar != 'b'), "Failed ParseKey --  THR x")){
            return XELP_E_Err;
        }

    }

    //test KEY function redirects
    {
        r = XELPParseKey(&x,XELPKEY_KEY);
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY) , "Failed ParseKey --  KEY 1")){
            return XELP_E_Err;
        }

        gGlobalCallbackData.k0 = 'x';
        r = XELPParseKey(&x,'0');
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY) || (gGlobalCallbackData.k0 != '0'), "Failed ParseKey --  THR 2")){
            return XELP_E_Err;
        }
        
        gGlobalCallbackData.k1 = 'y';
        r = XELPParseKey(&x,'1');
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_KEY) || (gGlobalCallbackData.k1 != '1'), "Failed ParseKey --  THR 2")){
            return XELP_E_Err;
        }

        // end of tests ... change mode to CLI and we shouldn't be getting key callbacks anymore
        gGlobalCallbackData.k1 = 'z';
        r = XELPParseKey(&x,XELPKEY_CLI); // change to CLI now THRU function shouldn't be called and dummy value should be unchnaged
        if (LOGTEST( (r!= XELP_S_OK) || (x.mCurMode != XELP_MODE_CLI) || (gGlobalCallbackData.k1  != 'z'), "Failed ParseKey --  THR x")){
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
    char *c1 = "tok0 tok1 tok2    \t tok3   tok4\n tok5";
    char *c2 = " tokk0 tok1 tok2    \t # tok3   tok4\n tok5 "; // test with comment
    char *c3 = " tokk0 tok1 tok2    \t # tok3   tok4\n tok5"; // test with comment
    
    XELP_XBInit(x,c1,XELPStrLen(c1));   
    r = XELPTokN(&x,0,&tok);
    if (LOGTEST( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok0") )),"XELPTokN get 0th token"))
        return XELP_E_Err;

    r = XELPTokN(&x,3,&tok);
    if (LOGTEST( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok3") )),"XELPTokN get 3rd token"))
        return XELP_E_Err;


    XELP_XBInit(x,c2,XELPStrLen(c2));   
    r = XELPTokN(&x,3,&tok);
    if (LOGTEST( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get 3rd token w commented line"))
        return XELP_E_Err;

    XELP_XBInit(x,c2,XELPStrLen(c3));   
    r = XELPTokN(&x,3,&tok);
    if (LOGTEST( ((r!=XELP_S_OK) || (XELP_S_OK != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get 3rd token w commented line"))
        return XELP_E_Err;

    XELP_XBInit(x,c2,XELPStrLen(c2));   
    r = XELPTokN(&x,23,&tok);
    if (LOGTEST( ((r == XELP_S_OK) || (XELP_S_NOTFOUND != XELPStrEq2(tok.s, tok.p,"tok5") )),"XELPTokN get 3rd token w commented line"))
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
    char *c1 = "tok1 tok2    \t tok3   tok4\n t0k5";
    char *c2 = " tok1 tok2    \t# tok3   tok4\n t0k5"; // test with comment

    XELP_XBInit(x,c1,XELPStrLen(c1));
    r = XelpNumToks(&x,&n);
    if (LOGTEST(((r!=XELP_S_OK) && (n !=5)),"XelpNumToks tabs and newlines"))
        return XELP_E_Err;

    XELP_XBInit(x,c2,XELPStrLen(c2));
    r = XelpNumToks(&x,&n);
    if (LOGTEST(((r!=XELP_S_OK) && (n !=3)),"XelpNumToks comment on second line"))
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


   // r = XELPParse(&x,s,XELPStrLen(s));
    
   // if (LOGTEST(r!=XELP_S_OK,"XELPParse Fail"))
   //     return XELP_E_Err;

    return XELP_S_OK;
}


/* 	************************************************
	Xelp Simple Unit Test suite.  
*/

int run_tests() {
    
    XelpUnit_InitGlobal(); // initialize the test case counters

    XelpUnit_RunUnit(test_XELPStrLen,"test_XELPStrLen");
	XelpUnit_RunUnit(test_XELPStr2Int,"test_XELPStr2Int");
    XelpUnit_RunUnit(test_XELPStrEq, "test_StrEq");
    XelpUnit_RunUnit(test_XELPStrEq2, "test_StrEq2");
    XelpUnit_RunUnit(test_XelpBufCmp,"test_XelpBufCmp");
    XelpUnit_RunUnit(test_XelpFindTok,"test_XelpFindTOk");
    XelpUnit_RunUnit(test_XelpTokLineXB,"test_XelpTokLineXB");
    XelpUnit_RunUnit(test_XELPTokN,"test_XelpTokN");
    XelpUnit_RunUnit(test_XelpNumToks,"test_XelpNumToks");
    XelpUnit_RunUnit(test_XelpInit,"test_XelpInit");
    XelpUnit_RunUnit(test_XelpOut_XelpThru_XelpErr,"failed test_XelpOut_XelpThru_XelpErr");
    XelpUnit_RunUnit(test_XELPExecKC,"test_XELPExecKC");
    
    XelpUnit_RunUnit(test_XELPParseKey,"Test ParseKey");
    XelpUnit_RunUnit(test_XELPParse,"Test Parse");
    XelpUnit_RunUnit(test_XELPParseXB,"Test ParseXB");
    XelpUnit_RunUnit(test_XelpHelp,"test_XelpHelp");
   
    XelpUnit_PrintResults();
	return XelpUnit_BuildPass(); // return whether we passed for CI purposes.  MOdify gBuildPass() if there is a diff way to report build pass 
}

/* 
	This main function only runs all the test code.
    If successful it returns S_OK which is equal to the numerical value of 0.
 	Any other value is considered a failure.
 */
int main()
{
	int result;
	
	printf("Running Xelp Unit tests .. \n");
	result = run_tests();

	if  (XELPUNIT_NOTFAIL(result)) 
		printf ("Tests passed.\n");
	else
		printf ("Tests failed.\n");

    return result;  /* remember the value 0 is considered passing in a *nix build continuous integration sense */

}
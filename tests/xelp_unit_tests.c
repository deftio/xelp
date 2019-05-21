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
#include "../src/xelp.h"

int FailMsg (const char *c, int n) {
    printf("TestCase: %3d FAIL: %s\n",n,c);
    return XELP_S_OK;
}

typedef struct {
    int totalCases; // total test cases run
    int totalPassed;

    int totalUnitsTested; // num units tested (eg functions)
    int totalUnitsPassed;

    int curCases;  //tests run in the current unit
    int curCasesPassed;
} GTESTDATA;

GTESTDATA gTestData;

void gUnitInit() {
    gTestData.curCases=0;
    gTestData.curCasesPassed=0;
    gTestData.totalUnitsTested++;
}

int gUnitRunUnit( int (*f)(), char *testName) {
    if (XELP_S_OK == f()) {
        gTestData.totalUnitsPassed++;
        return XELP_S_OK;
    }
    printf("Failed Unit Test: %s",testName);
    return XELP_E_Err;

}
XELPRESULT logTest(int result, char *msg) {
    gTestData.totalCases++; // global count
    gTestData.curCases++; // unit count...
    
    if (result == 0) { // pass
        gTestData.totalPassed++;
        gTestData.curCasesPassed++;
    }
    else {
        FailMsg(msg,gTestData.curCases);
    }
    return result;
}
//end of test loging section
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
	//{0         , 0 , ""              }
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

//=================================================

int test_XELPStrLen() {
    gUnitInit();

    if (logTest (3 != XELPStrLen("abc"),"XelpStrLen")) 
        return XELP_E_Err;

    if (logTest (0 != XELPStrLen(""),"XelpStrLen")) 
        return XELP_E_Err;

    return XELP_S_OK;
}
//=================================================
/*   test cases for string num parser function  */
int test_XELPStr2Int() {
	gUnitInit();
	if (logTest(XELPStr2Int("90",2) != 90,"Str2Int"))
		return XELP_E_Err;

    if (logTest(XELPStr2Int("31h",3) != 49,"Str2Int")) // hrc parser
		return XELP_E_Err;

	return XELP_S_OK;
}
//=================================================

int test_XelpBufCmp() {
    gUnitInit();
    char *a = "token1";
    char *b = " token1\0abc";
    char *c = "token1abc";
    char *ae, *be, *ce;

    ae = a + XELPStrLen(a);
    be = b + XELPStrLen(b);
    ce = c + XELPStrLen(a);
  
    if (logTest(XELP_S_NOTFOUND != XelpBufCmp(a,ae,b,be,XELP_CMP_TYPE_A0B0),"XelpBufCmp" ))
        return XELP_E_Err;
    
    be = b+1+XELPStrLen(a);
    if (logTest(XELP_S_OK != XelpBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_A0B0),"XelpBufCmp")) 
        return XELP_E_Err;
    

    be = b+2+XELPStrLen(b+1);
    if (logTest(XELP_S_NOTFOUND != XelpBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_BUF),"XelpBufCmp")) 
        return XELP_E_Err;
    
    ce = c+XELPStrLen(a);
    if (logTest(XELP_S_OK != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_BUF),"XelpBufCmp")) 
        return XELP_E_Err;
    

    ce = c+XELPStrLen(a)+1;
    if (logTest(XELP_S_NOTFOUND != XelpBufCmp(a,ae,c,ce,XELP_CMP_TYPE_BUF),"XelpBufCmp")) 
        return XELP_E_Err;

    return XELP_S_OK;
}


int test_XelpFindTok() {
    gUnitInit();
    char *label = "label1:",*le;
    char *b0 = "token1 token2; token3 token4 ; \n token5 token6 token7\n";
    char *b1 = "token1 token2 label1:\n";
    char *b2 = "token1 token2; \ntoken3 token4 token5\n   label1: token7 token8\n token9 label1: token10\n token11;";
    char *b3 = "token1 token2; \ntoken3 token4 token5\n   xlabel1: token7 token8\n token9 label1: token10\n token11;";
    XelpBuf x;
    
    le = label+XELPStrLen(label);

    x.s = b0;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (logTest(XELP_S_NOTFOUND != XELPFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok" ) )
        return XELP_E_Err;

    x.s = b1;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (logTest(XELP_S_OK != XELPFindTok(&x,label,le,XELP_TOK_ONLY),"XelpFindTok" ) )
        return XELP_E_Err;

    x.s = b2;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (logTest(XELP_S_OK != XELPFindTok(&x,label,le,XELP_TOK_LINE),"XelpFindTok" ) )
        return XELP_E_Err;

    x.s = b3;  x.p = x.s; x.e = x.s+XELPStrLen(x.s);
    if (logTest(XELP_S_NOTFOUND != XELPFindTok(&x,label,le,XELP_TOK_LINE),"XelpFindTok" ) )
        return XELP_E_Err;



    return XELP_S_OK;
}

int test_XelpTokLineXB() {
    gUnitInit();

    char *line1 = "abc def ghi";
    XelpBuf b,out;
    XELPRESULT r,r2;

    //=====
    XELP_XBInit(b,line1,XELPStrLen(line1));
    r  = XELPTokLineXB(&b,&out,XELP_TOK_ONLY);
    r2 = XelpBufCmp(line1, line1+3,out.s,out.p,XELP_CMP_TYPE_BUF);

    if (logTest((XELP_S_OK !=r) || (XELP_S_OK != r2),"XelpToklineXB first token"))
        return XELP_E_Err;
    XELP_XBTOP(b);
    

    return XELP_S_OK;
}

int test_XelpInit() {
    XELP myXelp, *x;
    x = &myXelp;

    gUnitInit();

    if (logTest(XELP_S_OK != XELPInit(x,"Xelp Unit Tests"),"XelpInit fail")) {
        return XELP_E_Err;
    }
    
    return XELP_S_OK;
}

char gChar;
void dummyOut(char c) {gChar = c;}
int test_XelpOut_XelpThru_XelpErr() {
    gUnitInit();

    XELP myXelp;
    XELPInit(&myXelp,"XelpOut Tests");
    
    if (logTest(XELP_S_OK != XELPOut(&myXelp,0,0),"XelpOut test XOut FN not set")) {
        return XELP_E_Err;
    }

    XELP_SET_FN_OUT(myXelp,dummyOut);
    XELP_SET_FN_THR(myXelp,dummyOut);
    XELP_SET_FN_ERR(myXelp,dummyOut);

    if (logTest(XELP_S_OK != XELPOut(&myXelp,"a",1),"XelpOut fail")) {
        return XELP_E_Err;
        if (gChar != 'a')
            return XELP_E_Err;
    }

    if (logTest(XELP_S_OK != XELPOut(&myXelp,"ab",2),"XelpOut fail")) {
        return XELP_E_Err;
        if (gChar != 'b')
            return XELP_E_Err;
    }

    if (logTest(XELP_S_OK != XELPOut(&myXelp,0,0),"XelpOut test Null msg")) {
        return XELP_E_Err;
    }
    return XELP_S_OK;
}

int test_XELPExecKC() {
    XELP x;
    XELPRESULT r;
    gUnitInit();

    XELPInit(&x,"TestExecKC");

    r = XELPExecKC(&x,'1');
    if (logTest(r!=XELP_S_NOTFOUND,"failed ExecKC null ptr")){
        return r;
    }
    
    XELP_SET_FN_KEY(x,gMyKeyCommands);
    XELP_SET_FN_CLI(x,gMyCLICommands);
	XELP_SET_FN_OUT(x,dummyOut);

    r = XELPExecKC(&x,'1');
    if (logTest((r!=XELP_S_OK)&&(gGlobalCallbackData.c1!='1'),"failed ExecKC '1' ")){
        printf("r=%d\n",r);
        return r;
    }

    r = XELPExecKC(&x,'z'); // not a mapped key...
    if (logTest(r!=XELP_S_NOTFOUND,"failed ExecKC 'z'")){
        return r;
    }
    
    return XELP_S_OK;
    
}
int test_XELPParseKey() {
    XELP x;
    XELPRESULT r;
    //char c;
    int i;
    gUnitInit();
    r = XELPInit(&x,"TestParseKey");
    XELP_SET_FN_KEY(x,gMyKeyCommands);
	XELP_SET_FN_CLI(x,gMyCLICommands);
    XELP_SET_FN_OUT(x,dummyOut);

    //begin actual test
    {
        char *c1 = " foo ";
        for (i=0; i  <XELPStrLen(c1); i++) {
            r = XELPParseKey(&x,c1[i]);
            if (logTest(r!= XELP_S_OK, "Failed Parse Key -- sending keys")){
                return XELP_E_Err;
            }
        }
        r = XELPParseKey(&x,XELPKEY_ENTER);
            if (logTest(r!= XELP_S_OK, "Failed Parse Key -- sending keys")){
                return XELP_E_Err;
            }
        if (logTest(gGlobalCallbackData.c1 != 1,"Failed Parse Key test cli1 value")) {
            return XELP_E_Err;
        }
    }
        //begin actual test
    {
        char *c2 = " bar; ";
        for (i=0; i  <XELPStrLen(c2); i++) {
            r = XELPParseKey(&x,c2[i]);
            if (logTest(r!= XELP_S_OK, "Failed Parse Key -- sending keys")){
                return XELP_E_Err;
            }
        }
        r = XELPParseKey(&x,XELPKEY_BKSP);
        r = XELPParseKey(&x,XELPKEY_ENTER);
            if (logTest(r!= XELP_S_OK, "Failed Parse Key -- sending keys w bskp test")){
                return XELP_E_Err;
            }
        if (logTest(gGlobalCallbackData.c1 != 1,"Failed Parse Key test cli1 value")) {
            return XELP_E_Err;
        }
    }

    return XELP_S_OK;
}



int test_XELPParse() {
    XELP x;
    char *s = "foo";
    XELPRESULT r;
    gUnitInit();
    
    XELPInit(&x,"TestParse");
    XELP_SET_FN_OUT(x,dummyOut);


    r = XELPParse(&x,s,XELPStrLen(s));
    
    if (logTest(r!=XELP_S_OK,"XELPParse Fail"))
        return XELP_E_Err;
    return XELP_S_OK;
}

int test_XELPParseXB() {
    XELP x;

    XELPInit(&x,"TestParseXB");
    gUnitInit();
    return XELP_S_OK;
}
/* 	************************************************
	Xelp simple unit test suite.  
*/

int run_tests() {
    
    gTestData.totalCases        = 0;
    gTestData.totalPassed       = 0;
    gTestData.totalUnitsTested  = 0;
    gTestData.totalUnitsPassed  = 0;

    if (XELP_S_OK != test_XELPStrLen()) {
        printf("failed test_XELPStrLen\n");
        return XELP_E_Err;
    }

	if (XELP_S_OK != test_XELPStr2Int()) {
		printf("failed test_XELPStr2Int()\n");
		return XELP_E_Err;
	}

    if (XELP_S_OK != test_XelpBufCmp()) {
		printf("failed test_XelpBufCmp()\n");
		return XELP_E_Err;
	}

    if (XELP_S_OK != test_XelpFindTok()) {
        printf("failed test_XelpFindTOk()\n");
    }

    if (XELP_S_OK != test_XelpTokLineXB() ) {
        printf("failed test_XelpTokLineXB()");
    }

    if (XELP_S_OK != test_XelpInit() ) {
        printf("failed test_XelpInit()");
    }

    if (XELP_S_OK != test_XelpOut_XelpThru_XelpErr()) {
        printf("failed test_XelpOut_XelpThru_XelpErr");
    }

    if (XELP_S_OK != test_XELPExecKC()){
        printf("failed test_XELPExecKC()");
    }

    if (XELP_S_OK != test_XELPParseKey()) {
        printf("failed test_XelpParseKey()");
    }
    printf("Test Cases Run\n");
    printf("Total Cases Passed %4d of %4d\n\n",gTestData.totalPassed, gTestData.totalCases);
    

	return XELP_S_OK;
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

	if (result == XELP_S_OK) 
		printf ("tests passed.\n");
	else
		printf ("tests failed.\n");

    return result;  /* remember the value 0 is considered passing in a travis-ci sense */

}
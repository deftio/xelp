/*

  @xelp_unit_tests.c - simple test file example file for xelp embedded cli/scripting library

  @copy Copyright (C)   <M. A. Chatterjee>
  @author M A Chatterjee <deftio [at] deftio [dot] com>
 
  
  @license: 
	Copyright (c) 2011-2018, M. A. Chatterjee <deftio at deftio dot com>
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
    int totalCases;
    int totalPassed;
    int curCases;
    int curCasesPassed;
} GTESTDATA;

/*************************************************
Unit Test Cases for XELP() functions below
*/


int test_XELPStrLen() {
    int testno = 0;

    if (3 != XELPStrLen("abc")) {
        FailMsg("XelpStrLen",testno);
        return XELP_E_Err;
    }

    return XELP_S_OK;
}
/*   test cases for string parser function  */
int test_XELPStr2Int() {
	
	if (XELPStr2Int("90",2) != 90)
		return XELP_E_Err;

	return XELP_S_OK;
}

int test_XelpBufCmp() {
    int   testno = 0;
    char *a = "token1";
    char *b = " token1\0";
    char *ae, *be;

    ae = a + XELPStrLen(a);
    be = b + XELPStrLen(b);

    if (XELP_S_NOTFOUND != XelpBufCmp(a,ae,b,be,XELP_CMP_TYPE_A0B0)) {
        FailMsg("XelpBufCmp",testno);
        return XELP_E_Err;
    }
    testno++;
    if (XELP_S_OK == XelpBufCmp(a,ae,b+1,be,XELP_CMP_TYPE_A0B0)) {
        FailMsg("XelpBufCmp",testno);
        return XELP_E_Err;
    }
    testno++;

    return XELP_S_OK;
}


/* 	************************************************
	Xelp  simple test suite.  
*/
int run_tests() {
    GTESTDATA gTestData;
    gTestData.totalCases  = 0;
    gTestData.totalPassed = 0;

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
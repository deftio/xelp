/*

  @xelp_simple_unit_test_fw.c - simple test file unit test framework
  used for xelp but genericish for most basic unit test tasks on posix systems.

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

#include "xelp_simple_unit_test_fw.h"
#include <stdio.h>


/***************************************
 Global instance of unit test statistics
*/
XELP_UnitTestData gTestData;


/***************************************

*/
XELPUNIT_RESULT XelpUnit_FailMsg (const char *c, int n) {
    printf("TestCase: %3d FAIL: %s\n",n,c);
    return XELPUNIT_PASS;
}


/***************************************

*/
XELPUNIT_RESULT XelpUnit_InitGlobal(){
    gTestData.totalCases        = 0;
    gTestData.totalPassed       = 0;
    gTestData.totalUnitsTested  = 0;
    gTestData.totalUnitsPassed  = 0;
    gTestData.curCases          = 0;
    gTestData.curCasesPassed    = 0;
    return XELPUNIT_PASS;
}

/***************************************

*/
XELPUNIT_RESULT XelpUnit_InitUnit() {
    gTestData.curCases=0;
    gTestData.curCasesPassed=0;
    gTestData.totalUnitsTested++;
}

/***************************************

*/
XELPUNIT_RESULT XelpUnit_RunUnit( int (*f)(), char *unitName) {
    XelpUnit_InitUnit();
    int r;
    
    r= f();
    if (XELPUNIT_PASS == r) {
        gTestData.totalUnitsPassed++;
        if (gTestData.curCases < 1)  // unit was run but no tests were encountered..
        {
            printf("Warning ==> Unit: \"%s\" no test cases run\n",unitName);
        }
    }
    else {
        printf("Failed Unit Test: %s",unitName);    
    }
    return r;

}

/***************************************

*/
XELPUNIT_RESULT XelpUnit_LogTest (int result, char *msg) {

    gTestData.totalCases++; // global individual test cases count
    gTestData.curCases++;   // global unit count...
    
    if (result == 0) { // pass
        gTestData.totalPassed++;
        gTestData.curCasesPassed++;
    }
    else {
        XelpUnit_FailMsg(msg,gTestData.curCases);
    }
    return result;
}
/***************************************

*/

XELPUNIT_RESULT XelpUnit_BuildPass() {
    XELPUNIT_RESULT r = XELPUNIT_FAIL;

    if ((gTestData.totalCases == gTestData.totalPassed) && (gTestData.totalUnitsPassed == gTestData.totalUnitsTested)) {
        r = XELPUNIT_PASS;
    }

    return r;
}
/***************************************

*/
XELPUNIT_RESULT XelpUnit_PrintResults() {
    printf("\n");
    printf("Test Cases Run\n");
    printf("Total Units Passed %4d of %4d\n", gTestData.totalUnitsPassed,gTestData.totalUnitsTested);
    printf("Total Cases Passed %4d of %4d\n\n",gTestData.totalPassed, gTestData.totalCases);
}

//end of test loging section
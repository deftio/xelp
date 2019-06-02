/*

  @jumpbug_unit_test_fw.c - simple test file unit test framework


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

#include "jumpbug_unit_test_fw.h"
#include <stdio.h>


/***************************************
 Global instance of unit test statistics
*/
JB_UnitTestData gTestData;

/***************************************

*/
JB_RESULT JumpBug_FailMsg (const char *c, int n) {
    printf("TestCase: %3d FAIL: %s\n",n,c);
    return JB_PASS;
}

JB_RESULT JumpBug_InitGlobal(){
    return JumpBug_InitStats(&gTestData);
}
/***************************************

*/
JB_RESULT JumpBug_InitStats(JB_UnitTestData *x){
    
    x->totalCases           = 0;
    x->totalPassed          = 0;
    x->totalPassedWarn      = 0;

    x->totalUnitsTested     = 0;
    x->totalUnitsPassed     = 0;
    x->totalUnitsPassedWarn = 0;
    
    x->curCases             = 0;
    x->curCasesPassed       = 0;
    x->curCasesPassedWarn   = 0;
    
    return JB_PASS;
}

/***************************************

*/
JB_RESULT JumpBug_InitUnit() {
    gTestData.curCases=0;
    gTestData.curCasesPassed=0;
    gTestData.totalUnitsTested++;
    return JB_PASS;
}

/***************************************

*/
JB_RESULT JumpBug_RunUnit( int (*f)(), char *unitName) {
    JumpBug_InitUnit();
    int result;
    
    result = f();
    if (JB_NOTFAIL(result)) {
        gTestData.totalUnitsPassed++;
        if ((result != JB_PASS) || (gTestData.curCases < 1))
            gTestData.totalUnitsPassedWarn++;

        if (gTestData.curCases < 1)  // unit was run but no tests were encountered..
        {
            printf("Warning ==> Unit: \"%s\" no test cases run\n",unitName);
        }
    }
    else {
        printf("Failed Unit Test: %s",unitName);    
    }
    return result;

}

/***************************************

*/
JB_RESULT JumpBug_LogTest (int result, char *msg) {

    gTestData.totalCases++; // global individual test cases count
    gTestData.curCases++;   // global unit count...
    
    if (JB_NOTFAIL(result)) { // pass
        gTestData.totalPassed++;
        gTestData.curCasesPassed++;
        if (result != JB_PASS) {
            gTestData.totalPassedWarn++;
            gTestData.curCasesPassedWarn++;
        }
    }
    else {
        JumpBug_FailMsg(msg,gTestData.curCases);
    }
    return result;
}
/***************************************
 * Test whether the build passed (warnings are OK)
*/

JB_RESULT JumpBug_BuildPass() {
    JB_RESULT r = JB_FAIL;

    if ((gTestData.totalCases == gTestData.totalPassed) && (gTestData.totalUnitsPassed == gTestData.totalUnitsTested)) {
        r = JB_PASS;
    }

    return r;
}
/***************************************

*/
JB_RESULT JumpBug_PrintResults() {
    printf("\n");
    printf("Test Cases Run\n");
    printf("Total Units Passed %4d of %4d with (%d warnings)\n", gTestData.totalUnitsPassed,gTestData.totalUnitsTested,gTestData.totalPassedWarn);
    printf("Total Cases Passed %4d of %4d with (%d warnings)\n\n",gTestData.totalPassed, gTestData.totalCases,gTestData.totalUnitsPassedWarn);
    return JB_PASS;
}

//end of test loging section
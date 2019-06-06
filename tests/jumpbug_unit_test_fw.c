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


/**************************************
 * Helper fns
 */
int JumpBug_StrLen (const char* c) {
    int l=0;
    while (*c++ != 0) {
        l++;
    }
    return l;
}

/***************************************
 write out a string.
 @param f  function pointer to char out function
 @param s  char buffer
 @max      max len of char buffer

 if (max == -1) treat as null terminated string only.

 */
JB_RESULT JumpBug_OutS( int (*f)(char), char *s, int max) {
    
    if (f && s && (*s) ) {
        max = max < 0 ? JumpBug_StrLen(s) : max;
        while ((max--) && (*s != 0)){
            f(*s++);
        } 
        return JB_PASS;
    }
    return JB_FAIL;
}

void intParseRecursive (int t, int o,  int (*f)(char))
{
        if(t != 0) intParseRecursive(t / 10, t % 10, f);
        f((char)o  + '0');
}
JB_RESULT JumpBug_OutN( int (*f)(char), int n ){
    if (f) {
        if(n < 0) { n = -n, f( '-');}
        intParseRecursive(n/10,n%10,f);
        return JB_PASS;
    }
    return JB_FAIL;
}

JB_RESULT JumpBug_OutH( int (*f)(char), int n ){
    int d,x = ((sizeof(int))<<1)-1;
    
    if (f) {
        f('0');
        f('x');
        do  {
            d = (n >> (x<<2))&0xf;
            d = (d > 9) ? (d-0xa + 'a') : (d + '0');
            f(d);
        }while (x--);
        return JB_PASS;
    }
    return JB_FAIL;
}
/***************************************

*/
JB_RESULT JumpBug_FailMsg (const char *c, int n) {
    printf("TestCase: %3d FAIL: %s\n",n,c);
    return JB_PASS;
}

JB_RESULT JumpBug_InitGlobal(int (*f)(char), int (*flog)(char)) {
    JumpBug_InitStats(&gTestData, f, flog);
}
/***************************************

*/
JB_RESULT JumpBug_InitStats(JB_UnitTestData *x,int (*f)(char), int (*flog)(char)) 
{
    int i   = sizeof(JB_UnitTestData);
 	char *p = (char *) x;  
	
    if (x) {
        while (i--)
		*p++=0;

        x->mpfPutChar = f;
        x->mpfPutCharLog = flog;
        return JB_PASS;
    }
    
    return JB_FAIL;
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
        if ((result != JB_PASS) || (gTestData.curCases < 1)) {
            gTestData.totalUnitsPassedWarn++;
        }

        if (gTestData.curCases < 1) {/* unit was run but no tests were encountered..*/
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
 * Test whether the build passed 
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
    printf("Total Units Passed %4d of %4d with (%d warnings)\n", gTestData.totalUnitsPassed,gTestData.totalUnitsTested,gTestData.totalUnitsPassedWarn);
    printf("Total Cases Passed %4d of %4d with (%d warnings)\n\n",gTestData.totalPassed, gTestData.totalCases,gTestData.totalPassedWarn);
    
    printf("\n");
    
    return JB_PASS;
}

//end of test loging section
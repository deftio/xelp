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


/***************************************
 Global instance of unit test statistics
*/
JB_UnitTestData gTestData;

/* gloabl emits */
#define OUTS(s) {JumpBug_OutS(gTestData.mpfPutChar,s,-1);}
#define OUTN(n) {JumpBug_OutN(gTestData.mpfPutChar,n);}


/**************************************
 * Helper fns
 */
int JumpBug_StrLen (const char* c) {
    int l=0;
    while (*c++ != 0) { l++; }
    return l;
}

/***************************************
 JumpBug_OutS write out a string.
 @param f  function pointer to char out function
 @param s  char buffer
 @max      max len of char buffer

 if (max == -1) treat as null terminated string only.

 */
int JumpBug_OutS( int (*f)(char), const char *s, int max) {
    if (f && s && (*s) ) {
        max = max < 0 ? JumpBug_StrLen(s) : max;
        while ((max--) && (*s != 0)){
            f(*s++);
        } 
        return JB_PASS;
    }
    return JB_FAIL;
}
/***************************************
 JumpBug_OutN write out a decimal integer
 */


int JumpBug_OutN( int (*f)(char), int n ){
    int t=10;
    if (f) {
        if(n < 0) { n = -n, f('-');}
        while ( (n/t) > 0) { t*=10; }
        while ( t>=10) {  
            t/=10;
            f((char)((n/t)%10)+'0');          
        }        
        return JB_PASS;
    }
    return JB_FAIL;
}
/***************************************
 JumpBug_OutNR write out a decimal integer
 old way uses recursion..
 
void intParseRecursive (int t, int o,  int (*f)(char)) {
    if(t != 0) intParseRecursive(t / 10, t % 10, f);
    f((char)o  + '0');
}
int JumpBug_OutNR ( int (*f)(char), int n ){
    if (f) {
        if(n < 0) { n = -n, f('-');}
        intParseRecursive(n/10,n%10,f);
        return JB_PASS;
    }
    return JB_FAIL;
}
*/
/***************************************
 JumpBug_OutN write out a decimal integer with space padding
  equiavlent ot %d in printf family
  e.g. printf("%4d",12) ==> "  12"  
   
 */
int JumpBug_OutNd( int (*f)(char), int n, int pad ){
    int t=10,s=0;
    if (f) {
        if(n < 0) { n = -n, s=1;pad--;}
        while ( (n/t) > 0) { t*=10; pad--;}
        while (pad-- > 0) {f(' ');}
        if (s) f('-');
        while ( t>=10) {  
            t/=10;
            f((char)((n/t)%10)+'0');          
        }        
        return JB_PASS;
    }
    return JB_FAIL;
}
/***************************************
 JumpBug_OutH write out a integer as hex
 */

int JumpBug_OutH( int (*f)(char), int n ){
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
 Logging support functions (YAML)
 */
#ifdef JUMPBUG_LOGGING_SUPPORT 
int JumpBug_YAML_Cmt(int (*f)(char), char *comment){                   /* YAML comment and \n e.g.'#my comment \n     */
    if (!f)
        return JB_FAIL;
    f('#');
    JumpBug_OutS(f,comment,-1);
    f('\n');
    return JB_PASS;
}
int JumpBug_YAML_Block(int (*f)(char), char *string, int indent){      /* YAML block begin e.g.   '  myblock:\n'      */
    if (!f)
        return JB_FAIL;
    while (indent--)
        f(JUMPBUG_YAML_INDENT);
    JumpBug_OutS(f,string,-1);
    f(':');
    f('\n');
    return JB_PASS;
}
int JumpBug_YAML_BlockN(int (*f)(char), char *string, int n, int indent){      /* YAML block begin e.g.   '  myblock:\n'      */
    if (!f)
        return JB_FAIL;
    while (indent--)
        f(JUMPBUG_YAML_INDENT);
    JumpBug_OutS(f,string,-1);
    JumpBug_OutN(f,n);
    f(':');
    f('\n');
    return JB_PASS;
}
int JumpBug_YAML_SS(int (*f)(char), char *key, char *val, int indent){ /* YAML string : string eg '   "key":"value"\n'*/
    if (!f)
        return JB_FAIL;
    while (indent--)
        f(JUMPBUG_YAML_INDENT);
    JumpBug_OutS(f,key,-1);
    f(':');f(' ');
    JumpBug_OutS(f,val,-1);
    f(' ');f('\n');
    return JB_PASS;
}
int JumpBug_YAML_SN(int (*f)(char), char *key, int val, int indent){   /* YAML string : num    eg '   "key":123    \n'*/
    if (!f)
        return JB_FAIL;
    while (indent--)
        f(JUMPBUG_YAML_INDENT);
    JumpBug_OutS(f,key,-1);
    f(':');f(' ');
    JumpBug_OutN(f,val);
    f(' ');f('\n');
    return JB_PASS;
}
#endif

/***************************************
 * print out fail message to console
*/
int JumpBug_FailMsg (const char *c, int n) {
    OUTS("Unit TestCase: ");
    OUTN(n);
    OUTS(" FAIL: ");
    OUTS(c);
    return JB_PASS;
}
/***************************************
 *  initialize stats for global ctr
 */ 
int JumpBug_InitGlobal(char *moduleName, int (*f)(char), int (*flog)(char)) {
    return JumpBug_InitStats(&gTestData, moduleName, f, flog);
}
/***************************************
 * init stats data structure, set up serialized output function ptrs
*/
int JumpBug_InitStats(JB_UnitTestData *x, char *moduleName, int (*f)(char), int (*flog)(char)) 
{
    int i   = sizeof(JB_UnitTestData);
 	char *p = (char *) x;  
	
    if (x) {
        while (i--)
	    	*p++=0;

        x->mModuleName   = moduleName;   /* global test suite name     */
        x->mpfPutChar    = f;            /* console output fn          */
#ifdef JUMPBUG_LOGGING_SUPPORT    
        x->mpfPutCharLog = flog;         /* log output fn, OK if NULL  */

        if (x->mpfPutCharLog) {
            JumpBug_YAML_Cmt(x->mpfPutCharLog,"JumpBug YAML File output begin");
            JumpBug_YAML_Block(x->mpfPutCharLog ,"Global Test Parameters",1);
            JumpBug_YAML_SN(x->mpfPutCharLog,"JumpBug Framework Version",JUMPBUG_VERSION,2);
            JumpBug_YAML_SS(x->mpfPutCharLog,"Test Module Name",moduleName,2);
        }
#endif
        return JB_PASS;
    }
    return JB_FAIL;
}

/***************************************

*/
int JumpBug_InitUnit() {
    gTestData.curCases=0;
    gTestData.curCasesPassed=0;
    gTestData.totalUnitsTested++;

    return JB_PASS;
}

/***************************************

*/
int JumpBug_RunUnit( int (*f)(), char *unitName) {
    int result;

    /* if logging ... */
#ifdef JUMPBUG_LOGGING_SUPPORT
    JumpBug_YAML_Block(gTestData.mpfPutCharLog,unitName,1);
#endif

    JumpBug_InitUnit();
    result = f();
    if (JB_NOTFAIL(result)) {
        gTestData.totalUnitsPassed++;
        if ((result != JB_PASS) || (gTestData.curCases < 1)) {
            gTestData.totalUnitsPassedWarn++;
        }

        if (gTestData.curCases < 1) {/* unit was run but no tests were encountered..*/
            OUTS("Warning ==> Unit: ");
            OUTS(unitName);
            OUTS(" no test cases run\n");
        }
    }
    else {
        OUTS("Failed Unit Test: ");
        OUTS(unitName); 
#ifdef JUMPBUG_LOGGING_SUPPORT
            JumpBug_YAML_SS (gTestData.mpfPutCharLog,"UNIT_FAIL","true",2);
#endif           
    }
#ifdef JUMPBUG_LOGGING_SUPPORT
    JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Tests Run",gTestData.curCases,2);
    JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Tests Passed",gTestData.curCasesPassed,2);
    JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Tests Passed with Warning(s)",gTestData.curCasesPassedWarn,2);
    JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Tests Failed",gTestData.curCases-gTestData.curCasesPassed,2);        
#endif

    return result;

}

/***************************************
 * log test result
*/
int JumpBug_LogTest (int result, char *msg) {

#ifdef JUMPBUG_LOGGING_SUPPORT
            JumpBug_YAML_BlockN (gTestData.mpfPutCharLog,"test_case",gTestData.curCases,2);
#endif            

    gTestData.totalCases++; // global individual test cases count
    gTestData.curCases++;   // global unit count...
    
    if (JB_NOTFAIL(result)) { // pass

        gTestData.totalPassed++;
        gTestData.curCasesPassed++;
        if (result != JB_PASS) {
            gTestData.totalPassedWarn++;
            gTestData.curCasesPassedWarn++;
#ifdef JUMPBUG_LOGGING_SUPPORT
            JumpBug_YAML_SS (gTestData.mpfPutCharLog,msg,"PASSWARN",3);
#endif            
        }
#ifdef JUMPBUG_LOGGING_SUPPORT
            JumpBug_YAML_SS (gTestData.mpfPutCharLog,msg,"PASS",3);
#endif                    

    }
    else {
#ifdef JUMPBUG_LOGGING_SUPPORT
            JumpBug_YAML_SS (gTestData.mpfPutCharLog,msg,"FAIL",4);
#endif         
        JumpBug_FailMsg(msg,gTestData.curCases);
    }
    return result;
}
/***************************************
    JumpBug_LogTestF 
    log a test result and print out file and line number if available
*/
int JumpBug_LogTestF(int result, char *msg, char *fname, int lineno ) {

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

int JumpBug_BuildPass() {
    int r = JB_FAIL;
    int x = 2;
#ifdef JUMPBUG_LOGGING_SUPPORT   
        if (gTestData.mpfPutCharLog) {
            JumpBug_YAML_Block(gTestData.mpfPutCharLog ,"Global Test Results",x-1);

            JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Units Run",gTestData.totalUnitsTested,x);
            JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Units Passed",gTestData.totalUnitsPassed,x);
            JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Units Passed with Warning(s)",gTestData.totalUnitsPassedWarn,x);
            JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Units Failed",gTestData.totalUnitsTested-gTestData.totalUnitsPassed,x);

            JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Test Cases Run",gTestData.totalCases,x);
            JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Test Cases Passed",gTestData.totalPassed ,x);
            JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total test Cases Passed with Warning(s)",gTestData.totalPassedWarn,x);
            JumpBug_YAML_SN(gTestData.mpfPutCharLog,"Total Test Cases Failed",gTestData.totalCases-gTestData.totalPassed,x);            
        }
#endif
    if ((gTestData.totalCases == gTestData.totalPassed) && (gTestData.totalUnitsPassed == gTestData.totalUnitsTested)) {
        r = JB_PASS;
    }

    return r;
}
/***************************************

*/
int JumpBug_PrintResults() {
    OUTS("\n");
    OUTS("Test Cases Run\n");
    OUTS("Total Units Passed ")
    OUTN(gTestData.totalUnitsPassed); 
    OUTS(" of ");
    OUTN(gTestData.totalUnitsTested);
    OUTS(" with (");
    OUTN(gTestData.totalUnitsPassedWarn); 
    OUTS(") warnings.\n"); 
    OUTS("Total Cases Passed ");
    OUTN(gTestData.totalPassed);
    OUTS(" of ");
    OUTN(gTestData.totalCases);
    OUTS(" with (");
    OUTN(gTestData.totalPassedWarn);
    OUTS(") warnings\n\n"); 
    
    
    return JB_PASS;
}

//end of test loging section
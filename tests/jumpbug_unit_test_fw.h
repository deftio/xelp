/*

  @jumpbug.h - simple test file unit test framework
   genericish for most basic unit test tasks on posix & embedded systems.

  requires <stdio.h>
    supports most older compilers as long as stdio.h is present and double slash (//) comments are supported.

    if using a modern OS suggest usting a modern test framework with ASSERT style stats

    use with a coverage tool (such as gcov, lcov etc,) to insure full coverage.

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

#ifndef __JUMPBUG__
#define __JUMPBUG__


#ifdef __cplusplus
extern "C"
{
#endif

#define JUMPBUG_VERSION  0x0100001  

/*  This is used as the result param from each function or unit 
    0 is used as pass because when using CI / build / coverage tools the exit code of the 
    unit test program should be 0 to be considered passing.
 */
typedef int JB_RESULT; 	

#define JB_PASS       (0)
#define JB_PASSWARN   (1)
#define JB_FAIL       (-1)

/* returns true (1) if result is PASS or WARN, else false (0) */
#define JB_NOTFAIL(x) ((x>=0)?1:0)


/* when to stop testing   */
#define JB_STOPLEVEL_FIRST_ERR 0
#define JB_STOPLEVEL_FIRST_WRN 1
#define JB_STOPLEVEL_RUN_ALL   2

/* The UnitTestData structure holds all the stats counters about what is happening in the unit test */
typedef struct {
    /* stats section **********************************/

    /* total individual cases run across all units */
    int totalCases;         
    int totalPassed;
    int totalPassedWarn;

    /* total number of units run                   */
    int totalUnitsTested;   
    int totalUnitsPassed;
    int totalUnitsPassedWarn;

    /* stats with in currently tested unit         */
    int curCases;           
    int curCasesPassed;
    int curCasesPassedWarn;

    /* other settings */
    int stopLevel;                /* don't stop, stop on 1st warn, stop on 1st error                               */
    int consoleVerboseLevel;      /* how detailed to print stats at end of run                                     */
    int loggingFormat;            /* print loggin as text or JSON or YAML                                          */

    /* platform abstraction layer */
    int (*mpfPutChar)(char);      /* default stream for writing out results (e.g. console)                         */
    int (*mpfPutCharLog)(char);   /* if full logging, then this needs to be set.  Can be set equal to mpfPutChar   */

}JB_UnitTestData;


/*************************************************************
 test set
 */
typedef struct {
    char *name;
    int (*mpfTest)();
} JB_Tests;

#define JUMPBUG_SET_FN_OUT(ths,pfOut)       (ths.mpfPutChar=pfOut)       /* print chars to console                 */
#define JUMPBUG_SET_FN_LOGOUT(ths,pfOut)    (ths.mpfPutCharLog=pfOut)    /* print chars to logging stream          */


/*************************************************************
 JumpBug API

 */
JB_RESULT JumpBug_InitGlobal(int (*f)(char), int (*flog)(char));                     // init test case statistics
JB_RESULT JumpBug_InitStats(JB_UnitTestData *x,int (*f)(char), int (*flog)(char)) ; // init stats structure for unit tests
JB_RESULT JumpBug_InitUnit();                            // init the test stats before running each unit
JB_RESULT JumpBug_RunUnit(int (*f)(), char *unitName);   // run a unit test (cases for a function or module)
JB_RESULT JumpBug_LogTest(int result, char *msg);        // log result of an individual testcase
JB_RESULT JumpBug_BuildPass();                           // test whether the build passed.  modifiy this to change passing criteria
JB_RESULT JumpBug_PrintResults();                        // print final results 

/**************************************************************
 * Support Fns (used internally but can be used anywhere)
 */
int JumpBug_StrLen (const char* c);                        /* find length of null term stri  */
JB_RESULT JumpBug_OutS( int (*f)(char), char *s, int max); /* output a string using the PAL  */
JB_RESULT JumpBug_OutN( int (*f)(char), int n );           /* output a decimal num using PAL */
JB_RESULT JumpBug_OutH( int (*f)(char), int n );           /* output a hex num using PAL     */

//JB_RESULT JumpBug_TestRunner(JB_Tests *t);

	
#define LOGTEST JumpBug_LogTest                                // short hand for JumpBug_LogTest()

#ifdef __cplusplus
}
#endif

#endif /* __JUMPBUG__ */


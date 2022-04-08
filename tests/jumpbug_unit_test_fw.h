/*

  @jumpbug.h - simple test file unit test framework
   genericish for most basic unit test tasks on posix & embedded systems.

  requires <stdio.h>
    supports most older compilers as long as appropriate platfrom abstraction function is provided

    if using a modern OS suggest usting a modern test framework with ASSERT style stats such at gtest, CUnit, etc

    suggest use with a coverage tool (such as gcov, lcov etc,) to insure full coverage

    YAML compatible logging is also provide (must provide appropriate platform support see docs) along with
    corresponding HTML view for report.

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

#define JUMPBUG_VERSION  0x00010001  
#define JUMPBUG_LOGGING_SUPPORT (1)

/********************************************************
    JUMPBUG Result codes
    The codes is used as the result param from each function or unit 
    0 is a PASS
    >0 is a PASS with Warning (useful for signalling possible issues)
    <0 is FAIL 


    Note: 0 is used as pass because when using CI / build / coverage tools the exit code of the 
    unit test program should be 0 to be considered passing.
 */
#define JB_PASS       (0)
#define JB_PASSWARN   (1)
#define JB_FAIL       (-1)

/* returns true (1) if result is PASS or WARN, else false (0) */
#define JB_NOTFAIL(x) ((x>=0)?1:0)

/******************************************************************* 
  platform support macros (JumpBug runs on old compilers with sporadic 
  support for some now std features) 
 */
#ifndef __LINE__  /* in C99 and later this returns the line number */
#define JUMPBUG_DBG_LINE    (-1) 
#else
#define JUMPBUG_DBG_LINE    (__LINE__)
#endif

#ifndef __FILE__ /* in C99 and later this returns the current filename */
#define JUMPBUG_DBG_FILE    ("filename unknown") 
#else
#define JUMPBUG_DBG_FILE    (__FILE__)
#endif
/* end platform debugging support macros */


/* when to stop testing   */
#define JB_STOPLEVEL_FIRST_ERR 0   /* stop at first error              */
#define JB_STOPLEVEL_FIRST_WRN 1   /* stop at first warning            */
#define JB_STOPLEVEL_RUN_ALL   2   /* don't stop, try to run all tests */

/************************************************ 
  The UnitTestData structure holds all the stats counters about what is happening in the unit test 
  */
typedef struct {
    char *mModuleName;          /* name of this module, printed out in results, log files (if used)  */
    
    /* stats section **********************************/
    /* total individual cases run across all units */
    int totalCases;         
    int totalPassed;
    int totalPassedWarn;

    /* total number of units run                   */
    int totalUnitsTested;   
    int totalUnitsPassed;
    int totalUnitsPassedWarn;

    /* stats in currently tested unit         */
    int curCases;           
    int curCasesPassed;
    int curCasesPassedWarn;

    /* other settings */
    int stopLevel;                /* don't stop || stop on 1st warn || stop on 1st error                           */
    int consoleVerboseLevel;      /* how detailed to print stats at end of run                                     */
    
    /* platform abstraction layer */
    int (*mpfPutChar)(const char);      /* default stream for writing out results (e.g. console)                         */
#ifdef JUMPBUG_LOGGING_SUPPORT    
    int (*mpfPutCharLog)(const char);   /* if full logging, then this needs to be set.  Can be set equal to mpfPutChar   */
    int gYAMLIndent;
#endif

}JB_UnitTestData;

#define JUMPBUG_SET_FN_OUT(ths,pfOut)       (ths.mpfPutChar=pfOut)       /* print chars to console                 */
#define JUMPBUG_SET_FN_LOGOUT(ths,pfOut)    (ths.mpfPutCharLog=pfOut)    /* print chars to logging stream          */

/*************************************************************
 test set TBD
 */
typedef struct {
    char *name;
    int (*mpfTest)();
} JB_Tests;




/*************************************************************
 JumpBug API

 */
int JumpBug_InitGlobal(char *moduleName, int (*f)(char), int (*flog)(char));                     // init test case statistics
int JumpBug_InitStats(JB_UnitTestData *x, char *moduleName, int (*f)(char), int (*flog)(char)) ; // init stats structure for unit tests
int JumpBug_InitUnit();                            // init the test stats before running each unit
int JumpBug_RunUnit(int (*f)(), char *unitName);   // run a unit test (cases for a function or module)
int JumpBug_LogTest(int result, char *msg);        // log result of an individual testcase
#ifdef JUMPBUG_LOGGING_SUPPORT
int JumpBug_LogTestF(int result, char *msg, char *fname, int lineno );        // log result of an individual testcase
#endif 
int JumpBug_BuildPass();                           // test whether the build passed.  modifiy this to change passing criteria
int JumpBug_PrintResults();                        // print final results 

/**************************************************************
 * Support Functions (used internally but can be used anywhere)
 * Out series of functions write formatted output to a stream function using 
 * the platform abstraction
 */
int JumpBug_StrLen (const char* c);                        /* find length of null term stri                               */
int JumpBug_OutS( int (*f)(char), const char *s, int max); /* output a string using the PAL                               */
int JumpBug_OutSQ( int (*f)(char), const char *s, int max);/* output a string w quotes, escapes with using the PAL        */
int JumpBug_OutN( int (*f)(char), int n );                 /* output a decimal num using PAL                              */
int JumpBug_OutNd(int (*f)(char), int n, int pad );        /* prints an decimal num with up to pad # of spaces (e.g. %8d) */
int JumpBug_OutH( int (*f)(char), int n );                 /* output a hex num using PAL                                  */

/**************************************************************
 * The following are used by the logger to output YAML compatable constructs & key-value pairs
 * While results are pretty-ish no support for cleaner formatting is provided (e.g. num padding alignment)
 * since those would presumable come from whatever is reading the YAML output.
 * 
 * You must add this line in your test code:
 * #define JUMPBUG_LOGGING_SUPPORT
 * 
 * 
 */ 
#ifdef JUMPBUG_LOGGING_SUPPORT 

#ifndef JUMPBUG_YAML_INDENT
#define JUMPBUG_YAML_INDENT ' '                                           /* YAML indent character                       */
#endif 

int JumpBug_YAML_GlobalReset(int (*f)(char));                             /* YAML set stream f, reset indent level       */
int JumpBug_YAML_Cmt(int   (*f)(char), char *comment);                    /* YAML comment and \n e.g.'#my comment     \n */
int JumpBug_YAML_Block(int (*f)(char), char *string, int indent);         /* YAML block begin e.g.   '  myblock:      \n'*/
int JumpBug_YAML_BlockEnd  (int numBlocks);                               /* YAML block end (updates global indent state)*/
int JumpBug_YAML_SS(int    (*f)(char), char *key, char *val, int indent); /* YAML string : string eg '  key : "value" \n'*/
int JumpBug_YAML_SN(int    (*f)(char), char *key, int val, int indent);   /* YAML string : num    eg '   key :123     \n'*/

#endif

int JumpBug_TestRunner(JB_Tests *t);

/* this wrapper allows dissolving macro use (e.g. in your makefile just #define JB_ASSRT(r,m) ()) */
#ifndef JB_ASSERT
#define JB_ASSERT(result,msg)   JumpBug_LogTestF((result),msg,JUMPBUG_DBG_FILE,JUMPBUG_DBG_LINE)
#endif

#ifndef JB_ASSERTX
#define JB_ASSERTX(result,msg)   JumpBug_LogTestF((result),msg,JUMPBUG_DBG_FILE,JUMPBUG_DBG_LINE)
#endif


#define LOGTEST JB_ASSERT
#ifdef __cplusplus
}
#endif

#endif /* __JUMPBUG__ */


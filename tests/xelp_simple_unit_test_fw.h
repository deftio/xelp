/*

  @xelp_simple_unit_test_fw.h - simple test file unit test framework
  used for xelp but genericish for most basic unit test tasks on posix systems.

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

#ifndef __XELP_UNIT__
#define __XELP_UNIT__


#ifdef __cplusplus
extern "C"
{
#endif

typedef int XELPUNIT_RESULT; 	


typedef struct {
    int totalCases;
    int totalPassed;
    int totalUnitsTested;
    int totalUnitsPassed;
    int curCases;
    int curCasesPassed;

}XELP_UnitTestData;

#define XELPUNIT_PASS       (0)
#define XELPUNIT_FAIL       (-1)
#define XELPUNIT_PASSWARN   (1)

#define XELPUNIT_NOTFAIL(x) ((x>=0)?0:1)


XELPUNIT_RESULT XelpUnit_InitGlobal();                          // init test case statistics
XELPUNIT_RESULT XelpUnit_InitUnit();                            // init the test stats before running each unit
XELPUNIT_RESULT XelpUnit_RunUnit(int (*f)(), char *unitName);   // run a unit test (cases for a function or module)
XELPUNIT_RESULT XelpUnit_LogTest(int result, char *msg);        // log result of an individual testcase
XELPUNIT_RESULT XelpUnit_BuildPass();                           // test whether the build passed.  modifiy this to change passing criteria
XELPUNIT_RESULT XelpUnit_PrintResults();                        // print final results to console

#define LOGTEST XelpUnit_LogTest                                // short hand for XelpUnit_LogTest()

#ifdef __cplusplus
}
#endif

#endif /* __XELP_UNIT__ */


# Unit tests

This is a simple unit test framework which should run on most embedded systems.

## Features 
* Simple assert style test logging
* counts passing, warning & fails
* prints stats at end (opt see verbose)
* can be used with coverage tool such as gcov etc
* low overhead
* can be used cross platform for embedded projects (e.g. same on arduino vs ubuntu linux) using Platform Abstraction Layer
    * no dependancies on C std library
    * string / number / YAML output are all serialized internally
* instance based (can run mult at same time)

## TODO
* verbose - add support for summary section at console / stdout
    * (flag:slient)   - nothing printed only return code is available (e.g. returns 0)
    * (flag:one-line) - Name & result only ==> "Running tests for 'MyProject' ... passed"
    * (flag:stats )   - Name & result along with summary stats
* stop levels - add support for
    * stop on first error
    * stop on first warning
    * don't stop (try to go as long as possible)
* logging - support for logging all test cases ==> logs all test cases to this stream (on posix show file w flush example)
    * setup
        * programmer (user) must provide setup, teardown. 
            * SET_FN_PUTC_LOG((int*)(f(char c))) 
            * set flush()?
    *(done) logging format 
        * YAML (?) - advantage of YAML is that doesn't need close brace.  so if the execution stops the file is still valid.
    * report (JumpBug_Report.html)
        * Add support for url load eg JumpBug_Report?yamldata=filename
        * Add summary sectiosn for units
        * Add summary section at top for overall
        * Add support for multiple modules (e.g. run several modules one after the other) Have YAML support concat
        * Add support for single table view (e.g. single-table ?singleTable=true)
* (done) get rid of printf() dependancy --> make SET_FN_PUTC((int *)(f(char c))
    * (done) note to do this we need to hande the %s and %d in the current printf() functions
    * note for posix also add
        consider adding puts  # faster writing to log
        consider showing example where each write is flushed and each write opens, closes, appends file.
* naming
    * jetbug
    * blubug
    * jumpbug ** currenty chosen
    * redbug



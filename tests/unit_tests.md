# Unit tests

This is a simple unit test framework which should run on most embedded systems.

## Features 
* Simple assert style test logging
* counts passing, warning & fails
* prints stats at end (opt see verbose)
* can be used with coverage tool such as gcov etc
* low overhead
* can be used cross platform for embedded projects (e.g. same on arduino vs ubuntu linux) using HAL
* instance based (can run mult at same time)

## TODO
* add support for verbose levels
    none
    stats only (at end of run)
    print stats as JSON only
* add support for
    stop on first error
    stop on first warning
    don't stop (try to go as long as possible)
* add support for logging all test cases ==> logs all test cases to this stream (on posix show file w flush example)
    * SET_FN_PUTC_LOG((int*)(f(char c))) 
* get rid of printf() dependancy --> make SET_FN_PUTC((int *)(f(char c))
    * note to do this we need to hande the %s and %d in the current printf() functions
    * note for posix also add
        consider adding puts  # faster writing to log
        consider showing example where each write is flushed and each write opens, closes, appends file.
* naming
    * jetbug
    * blubug
    * jumpbug ** currenty chosen
    * redbug



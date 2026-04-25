# Testing Guide

How to use the JumpBug test framework included with xelp.

## What is JumpBug?

JumpBug is a minimal unit-test framework designed for embedded C projects.
It compiles under C89 and later, uses no dynamic memory, and needs only a
single function-pointer for character output -- making it portable to bare-metal
targets, RTOSes, and POSIX hosts alike.

Key properties:

- **No dependencies** beyond a `putchar`-style callback.
- **Callback-based I/O** -- console and optional YAML log output are driven
  through user-supplied `int (*f)(char)` function pointers.
- **Multi-instance stats** -- a `JB_UnitTestData` struct tracks pass/fail/warn
  counts for both individual test cases and test units.
- **Optional YAML logging** -- when `JUMPBUG_LOGGING_SUPPORT` is defined (the
  default), test results are also emitted as machine-readable YAML.

## Result Codes

| Constant | Value | Meaning |
|----------|-------|---------|
| `JB_PASS` | 0 | Test passed |
| `JB_PASSWARN` | 1 | Passed with warning |
| `JB_FAIL` | -1 | Test failed |

`JB_NOTFAIL(x)` returns 1 when `x >= 0` (pass or warning), 0 otherwise.

## Initialization

```c
#include "jumpbug_unit_test_fw.h"

int putcharc(char x) { return putchar(x); }

JumpBug_InitGlobal("ModuleName", putcharc, flogout);
```

`JumpBug_InitGlobal` takes a module name, a console output function, and an
optional YAML log output function (may be `NULL`).

## Running Test Units

A *unit* groups related test cases for a single function or module.
Register each unit with `JumpBug_RunUnit`:

```c
JumpBug_RunUnit(test_MyFunction, "MyFunction");
```

`JumpBug_RunUnit` calls `JumpBug_InitUnit()` internally, runs the test
function, and updates global pass/fail statistics.

## Assertions

### Basic assertion

```c
JB_ASSERT(result_expr, "message")
```

Returns `JB_PASS` (0) on success, `JB_FAIL` (-1) on failure.  Logs the result
(including file name and line number) via `JumpBug_LogTestF`.

The traditional two-line pattern used throughout the xelp test suite:

```c
if (JB_ASSERT(actual != expected, "description"))
    return XELP_E_ERR;
```

### Convenience macros

These macros reduce boilerplate for the most common patterns.

#### `JB_ASSERT_RET(result, msg, retval)`

Assert and return on failure in a single statement:

```c
JB_ASSERT_RET(r != XELP_S_OK, "init failed", XELP_E_ERR);
```

Equivalent to:

```c
if (JB_ASSERT(r != XELP_S_OK, "init failed"))
    return XELP_E_ERR;
```

#### `JB_ASSERT_EQ(actual, expected, msg)`

Equality check for `int` values.  On failure the framework prints both the
expected and actual values:

```c
JB_ASSERT_EQ(result, 42, "should return 42");
```

On failure output:

```
Unit TestCase: 5 FAIL: should return 42
  expected: 42, actual: 7
```

#### `JB_ASSERT_EQ_RET(actual, expected, msg, retval)`

Combines `JB_ASSERT_EQ` with return-on-fail:

```c
JB_ASSERT_EQ_RET(count, 3, "expected 3 items", XELP_E_ERR);
```

### Aliases

`JB_ASSERTX` and `LOGTEST` are aliases for `JB_ASSERT`.

## Printing Results and Build Status

After all units have run:

```c
JumpBug_PrintResults();   /* print summary to console */
int ok = JumpBug_BuildPass();  /* JB_PASS if all tests passed */
return ok;                /* exit code 0 = CI pass */
```

## YAML Logging

When `JUMPBUG_LOGGING_SUPPORT` is defined (default), JumpBug emits
structured YAML to the log output function.  The xelp test suite writes
this to `xelp-test-log.yaml` and the companion `jumpbug-report.html`
page renders it in a browser.

## Adding a New Test

1. Write a test function in `tests/xelp_unit_tests.c`:

```c
XELPRESULT test_MyFeature() {
    /* setup */
    XELP x;
    XelpInit(&x, "MyFeature");

    /* test cases */
    JB_ASSERT_EQ_RET(XelpStrLen("hi"), 2, "strlen hi", XELP_E_ERR);

    return XELP_S_OK;
}
```

2. Register it in `run_tests()`:

```c
JumpBug_RunUnit(test_MyFeature, "MyFeature");
```

3. Build and run:

```
make clean && make tests
```

## API Quick Reference

| Function / Macro | Description |
|------------------|-------------|
| `JumpBug_InitGlobal(name, fout, flog)` | Initialize global test state |
| `JumpBug_InitStats(data, name, fout, flog)` | Initialize a `JB_UnitTestData` struct |
| `JumpBug_RunUnit(fn, name)` | Run a test unit function |
| `JB_ASSERT(result, msg)` | Log a test case result |
| `JB_ASSERT_RET(result, msg, retval)` | Assert + return on fail |
| `JB_ASSERT_EQ(actual, expected, msg)` | Equality assertion with value printing |
| `JB_ASSERT_EQ_RET(actual, expected, msg, retval)` | Equality + return on fail |
| `JumpBug_PrintResults()` | Print summary to console |
| `JumpBug_BuildPass()` | Return `JB_PASS` if all tests passed |

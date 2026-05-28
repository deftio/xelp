# XelpScript Reference

XelpScript is an optional, no-malloc script engine for xelp. It is not
Lua, MicroPython, Tcl, or Forth. It does not replace C firmware. It adds
lightweight programmable control flow over xelp's native C command table:
variables, conditionals, nested command values, math, and script
functions -- all using a fixed per-instance arena with no dynamic memory.

Enable it with `XELP_ENABLE_SCRIPT` (requires `XELP_ENABLE_CLI`).
Compiled cost: ~6-7 KB over the HIST profile on 32-bit targets (ARM
Thumb: 9,200 bytes total, rv32: 10,970 bytes, ESP32-S3: 9,542 bytes).

## Terminology: two kinds of scripting

xelp has always supported **command sequences** -- flat ROM-safe strings
that run multiple commands separated by semicolons or newlines:

```c
/* Command sequences: available with XELP_ENABLE_CLI, no script engine needed */
XelpParse(&cli, "led 1; wait 100; led 0", 22);
```

**XelpScript** is the optional programmable layer on top of that. When
`XELP_ENABLE_SCRIPT` is defined, the CLI parser routes through the
script engine, enabling variables, math, conditionals, functions, and
all the builtins documented below.

```
Command sequences:  "led 1; wait 100; led 0"       -- always available with CLI
XelpScript:         "_set t ($sensor read); _if (_gt $t 80) _then fan 1"  -- requires XELP_ENABLE_SCRIPT
```

## Quick start

### Enable

In your build flags or `xelp_ovr.h`:

```c
#define XELP_ENABLE_CLI     1
#define XELP_ENABLE_SCRIPT  1
```

### Minimal example

```c
#include "xelp.h"

XELP cli;

void setup(void) {
    XelpInit(&cli, "My Device");
    XELP_SET_FN_OUT(cli, &uart_putc);
    XELP_SET_FN_CLI(cli, commands);

    /* Run a script */
    const char *script =
        "_set count 0\n"
        ":loop\n"
        "_print $count\n"
        "_inc count\n"
        "_if (_lt $count 5) _then _goto :loop\n"
        ":_end\n";
    XelpParse(&cli, script, XelpStrLen(script));
}
```

### Interactive use

At the prompt, XelpScript builtins work alongside your C commands:

```
xelp> _set x 42
xelp> _print $x
42
xelp> _set y (_add $x 8)
xelp> _print $y
50
xelp> _if (_gt $y 40) _then echo "y is big"
y is big
```

## Variables

`_set` creates or overwrites a variable in the arena. `$name` expands
the variable's value inline.

```
_set x 42          # INT variable (numeric value detected)
_set msg hello     # STR variable (non-numeric)
_set y $x          # y gets the current value of x
echo $msg          # expands to: echo hello
```

### Type inference

When the value string parses as a number (decimal or hex), the variable
is stored as `INT`. Otherwise it is stored as `STR`. This matters for
math and comparison builtins -- math operations require INT values.

### In-place modification

`_inc` and `_dec` increment or decrement an INT variable by 1. They
return the new value.

```
_set i 0
_inc i       # i is now 1
_dec i       # i is now 0
```

Both return `XELP_E_UNDEF_VAR` if the variable doesn't exist and
`XELP_E_TYPE_ERR` if it isn't INT.

## Parenthesized subexpressions

Parentheses capture a command's result as text and substitute it inline.
Evaluation proceeds innermost-first:

```
_set area (_mul (_add $w 1) $h)
```

The engine evaluates `(_add $w 1)` first, substitutes the result (say
`5`), then evaluates `(_mul 5 $h)`. The final result is stored in
`area`.

Paren evaluation uses a 32-byte result buffer, so returned text is
truncated to 31 characters. This is intentional -- the engine is for
small control values, not string processing.

## Math builtins

All math builtins operate on integers and push their result onto the
result stack.

| Builtin | Arguments | Description |
|---------|-----------|-------------|
| `_add` | `a b [c ...]` | Sum of all arguments (variadic) |
| `_sub` | `a b` | `a - b` |
| `_mul` | `a b [c ...]` | Product of all arguments (variadic) |
| `_div` | `a b` | `a / b` (integer division, returns `XELP_E_ERR` on divide-by-zero) |
| `_mod` | `a b` | `a % b` (returns `XELP_E_ERR` on divide-by-zero) |

```
_set r (_add 10 20 30)    # r = 60
_set r (_mul 2 3 4)       # r = 24
_set r (_div 17 5)        # r = 3
_set r (_mod 17 5)        # r = 2
```

Non-numeric arguments return `XELP_E_TYPE_ERR`.

## Bitwise builtins

| Builtin | Arguments | Description |
|---------|-----------|-------------|
| `_band` | `a b` | Bitwise AND |
| `_bor` | `a b` | Bitwise OR |
| `_bxor` | `a b` | Bitwise XOR |
| `_bnot` | `a` | Bitwise NOT (one's complement) |
| `_shl` | `a n` | Left shift `a` by `n` bits |
| `_shr` | `a n` | Right shift `a` by `n` bits (unsigned) |

Shift amounts must be 0-31 inclusive; out-of-range returns `XELP_E_ERR`.

```
_set r (_band 0xFF 0x0F)   # r = 15
_set r (_shl 1 4)          # r = 16
_set r (_bxor 0xAA 0xFF)   # r = 85
```

## Comparison builtins

All comparison builtins return 1 (true) or 0 (false) as an INT result.

| Builtin | Arguments | Description |
|---------|-----------|-------------|
| `_eq` | `a b` | Equal (numeric-first, string fallback) |
| `_neq` | `a b` | Not equal (numeric-first, string fallback) |
| `_lt` | `a b` | Less than (numeric only) |
| `_gt` | `a b` | Greater than (numeric only) |
| `_le` | `a b` | Less than or equal (numeric only) |
| `_ge` | `a b` | Greater than or equal (numeric only) |

`_eq` and `_neq` try numeric comparison first. If either argument is
non-numeric, they fall back to byte-for-byte string comparison. The
other four (`_lt`, `_gt`, `_le`, `_ge`) require both arguments to be
numeric and return `XELP_E_TYPE_ERR` otherwise.

```
_set r (_eq 10 10)       # r = 1
_set r (_eq hello hello) # r = 1 (string comparison)
_set r (_lt 3 10)        # r = 1
```

## Logic builtins

| Builtin | Arguments | Description |
|---------|-----------|-------------|
| `_and` | `a b` | Logical AND |
| `_or` | `a b` | Logical OR |
| `_not` | `a` | Logical NOT |

**Truthiness rules:** `0` and empty string `""` are false. Any non-zero
number or non-empty non-numeric string is true.

```
_set r (_and 1 1)     # r = 1
_set r (_or 0 1)      # r = 1
_set r (_not 0)       # r = 1
_set r (_not hello)   # r = 0  (non-empty string is truthy)
```

## Conditionals

```
_if <condition> _then <command> [_else <command>]
```

`_if` evaluates the condition using truthiness rules, then executes the
`_then` command or the `_else` command. Each branch is a **single
command** (one command name with its arguments).

```
_if (_lt $t 100) _then echo OK _else echo OVER
_if $flag _then led 1
_if (_eq $mode auto) _then _goto :auto_loop
```

For multi-step branches, call a function or use `_goto`:

```
_if $ready _then _goto :startup
```

## Switch (multi-way branch)

```
_switch <value> <case1> <cmd1> [<case2> <cmd2> ...] [_default <cmdN>]
```

`_switch` compares `value` against each case and executes the matching
command. Cases are checked in order; the first match wins. `_default`
matches anything and should appear last.

Each case command is a **single token**. Quote multi-word commands:

```
_switch $mode idle "_print standby" run "_print active" _default "_print unknown"
```

Case matching uses numeric comparison first. If the value is numeric and
the case parses as a number, they are compared as integers. Otherwise
the comparison falls back to byte-for-byte string matching.

```
_set v 2
_switch $v 1 "_print one" 2 "_print two" 3 "_print three"
# prints: two

_switch $color red "_set r 255" green "_set g 255" _default "_print ?"
```

If no case matches and there is no `_default`, `_switch` returns
`XELP_S_OK` silently. Fewer than 3 arguments (value + at least one
case/cmd pair) returns `XELP_E_ERR`.

## Labels and jumps

Labels are lines starting with `:` followed by a name. They define jump
targets within a script.

```
:loop
_print $i
_inc i
_if (_lt $i 5) _then _goto :loop
echo done
:_end
```

| Command | Description |
|---------|-------------|
| `:label` | Define a label (not executed, just a marker) |
| `_goto :label` | Jump to label, searching from **start** of current script |
| `_next :label` | Jump to label, searching **forward** from current position |
| `_next <command>` | Execute a sub-command (not a jump) |
| `:_end` | Special label marking end of script (required when using `_goto`) |

`_goto` restarts scanning from the beginning of the script -- use it for
backward jumps (loops). `_next` only scans forward -- use it for
skip-ahead patterns. Both return `XELP_E_NO_LABEL` if the label is not
found.

`_goto :_end` and `_next :_end` both advance to the end of the current
script, effectively exiting.

## Functions

### Script-defined functions (`_func`)

Define a function at runtime. The body is stored in the arena:

```
_func square "_return (_mul @1 @1)"
_set r (square 7)
_print $r               # prints 49
```

### Positional parameters

Inside a function body, `@1`, `@2`, etc. refer to the arguments passed
by the caller:

```
_func greet "echo hello @1"
greet world             # prints: hello world
```

### `_return`

`_return` exits the current function and pushes a value onto the result
stack. If no value is given, it pushes NIL.

```
_func double "_return (_mul @1 2)"
_set r (double 7)       # r = 14
```

`_return` also mirrors its value to `mR[1]` when the value is numeric.
Calling `_return` outside a function returns `XELP_E_NO_FRAME`.

### C-registered ROM script functions

Functions can also be defined in a C table and stored in ROM:

```c
XELPScriptFuncEntry script_funcs[] = {
    { "double", "_return (_mul @1 2)",  "double <n>" },
    { "add2",   "_return (_add @1 @2)", "add2 <a> <b>" },
    { 0, 0, 0 }  /* terminator */
};

XELP_SET_FN_SCRIPT(cli, script_funcs);
```

These behave identically to `_func`-defined functions but live in
`.rodata` instead of the arena, saving RAM.

### Calling functions with captured return value

Use parentheses to capture a function's return value:

```
_set r (double 7)       # call double, capture result in r
echo (add2 3 4)         # call add2, pass result to echo
```

## C interop

XelpScript and C code interoperate naturally because XelpScript builtins
and user C commands share the same dispatch path.

### C commands from scripts

Any C command registered in the CLI function table is callable from
scripts with no extra work:

```
led 1                        # call C command
_set status (sensor read)    # call C command, capture result
```

### C calling script functions

From C code, use `XelpCallProc` to invoke a script function:

```c
XELPRESULT XelpCallProc(XELP *ths, const char *cmdline);
```

```c
XelpCallProc(&cli, "double 21");
/* Result is on the stack or mirrored in mR[1] */
```

### Pushing results from C commands

C command handlers can push values onto the script result stack so
parenthesized calls can capture them:

```c
XELPRESULT XelpSetResultInt(XELP *ths, int val);
XELPRESULT XelpSetResultStr(XELP *ths, const char *s, int slen);
```

### Reading results from C

```c
XELPRESULT XelpGetResult(XELP *ths, XelpResult *result);
```

`XelpResult` is a tagged value:

```c
typedef struct {
    unsigned char kind;   /* XELP_VAL_NIL, XELP_VAL_INT, or XELP_VAL_STR */
    int           intVal; /* valid when kind == XELP_VAL_INT               */
    const char   *strVal; /* valid when kind == XELP_VAL_STR               */
    int           strLen; /* length of strVal (no null-term guarantee)      */
} XelpResult;
```

## Registers

`_mr` reads and writes the instance's `mR[]` registers, bridging script
and C handler return values:

```
_mr 1 42             # write 42 to mR[1]
_set v (_mr 1)       # read mR[1] into variable v
_mr 2 99             # write 99 to mR[2]
```

The index must be 0 to `XELP_REGS_SZ - 1` (default 0-3). Both read and
write modes push the value onto the result stack.

## Introspection

| Command | Description |
|---------|-------------|
| `_list` | List all arena variables and script functions |
| `_list vars` | List variables only |
| `_list funcs` | List functions only |
| `_print <value>` | Output a value (variable, literal, or expanded expression) |
| `_lpad <str> <width>` | Right-align `str` in a field of `width` characters |

```
xelp> _set x 42
xelp> _set msg hello
xelp> _func sq "_return (_mul @1 @1)"
xelp> _list
  $x = 42  (INT)
  $msg = hello  (STR)
  sq() = "_return (_mul @1 @1)"
```

## Breakpoint callback

Register a callback that fires after every script statement. This is the
mechanism for step budgets, watchdog resets, and safe execution:

```c
XELPRESULT my_breakpoint(XELP *ths) {
    (void)ths;
    if (--step_budget <= 0)
        return XELP_E_BREAK;   /* halt execution */
    return XELP_S_OK;          /* continue */
}

XELP_SET_FN_BREAKPOINT(cli, &my_breakpoint);
```

If the callback returns anything other than `XELP_S_OK`, script
execution halts immediately with `XELP_E_BREAK`. Use this to:

- Limit loop iterations (step budget)
- Service a hardware watchdog timer
- Implement single-step debugging
- Abort on external signals

## Arena memory model

The script engine uses a single fixed-size buffer per instance -- the
**arena**. No malloc. No heap fragmentation across instances.

```
Arena layout (XELP_SCRIPT_ARENA_SZ bytes):

 Low address                                    High address
 |                                                         |
 [  Stack (grows up -->)  |  free  |  (<-- grows down) Heap ]
 ^                        ^        ^                        ^
 mArena                  mSP      mHP        mArena + ARENA_SZ
```

**Stack** (grows up from start): result entries, call frames.

**Heap** (grows down from end): variables (INT and STR), PROC entries
(inline function bodies).

When `mSP >= mHP`, the arena is full and further allocations return
`XELP_E_ARENA_FULL`.

### Sizing

The default size scales with pointer width:

| Architecture | `sizeof(int)` | Default arena |
|-------------|---------------|---------------|
| 16-bit | 2 | 512 bytes |
| 32-bit | 4 | 1024 bytes |
| 64-bit | 8 | 2048 bytes |

Override with a compiler flag:

```c
-DXELP_SCRIPT_ARENA_SZ=2048
```

Or in `xelp_ovr.h`:

```c
#undef  XELP_SCRIPT_ARENA_SZ
#define XELP_SCRIPT_ARENA_SZ  2048
```

The arena is per-instance RAM. Each `XELP` struct includes an arena of
this size when `XELP_ENABLE_SCRIPT` is defined.

## Error codes

Script-specific error codes (all negative, defined when
`XELP_ENABLE_SCRIPT` is active):

| Code | Value | Meaning |
|------|------:|---------|
| `XELP_E_ARENA_FULL` | -4 | Arena stack and heap have collided |
| `XELP_E_UNDEF_VAR` | -5 | Referenced variable not found |
| `XELP_E_TYPE_ERR` | -6 | Type mismatch (e.g. math on a string) |
| `XELP_E_NO_LABEL` | -7 | `_goto` or `_next` target label not found |
| `XELP_E_NO_FRAME` | -8 | `_return` called outside a function |
| `XELP_E_BUDGET` | -9 | Step budget exceeded |
| `XELP_E_BREAK` | -10 | Breakpoint callback halted execution |

These supplement the core xelp error codes (`XELP_S_OK` = 0,
`XELP_E_ERR` = -1, `XELP_E_CMDBUFFULL` = -2,
`XELP_E_CMDNOTFOUND` = -3).

## Builtin reference

Complete table of all 33 XelpScript builtins:

| Builtin | Arguments | Returns | Description |
|---------|-----------|---------|-------------|
| `_set` | `name value` | OK | Set variable (INT if numeric, STR otherwise) |
| `_print` | `value [...]` | OK | Output values to console |
| `_mr` | `index [value]` | INT | Read or write `mR[]` register |
| `_inc` | `name` | INT | Increment INT variable by 1 |
| `_dec` | `name` | INT | Decrement INT variable by 1 |
| `_add` | `a b [c ...]` | INT | Sum (variadic) |
| `_sub` | `a b` | INT | Subtract |
| `_mul` | `a b [c ...]` | INT | Product (variadic) |
| `_div` | `a b` | INT | Integer division |
| `_mod` | `a b` | INT | Modulo |
| `_band` | `a b` | INT | Bitwise AND |
| `_bor` | `a b` | INT | Bitwise OR |
| `_bxor` | `a b` | INT | Bitwise XOR |
| `_bnot` | `a` | INT | Bitwise NOT |
| `_shl` | `a n` | INT | Left shift (n: 0-31) |
| `_shr` | `a n` | INT | Right shift unsigned (n: 0-31) |
| `_eq` | `a b` | INT | Equal (numeric-first, string fallback) |
| `_neq` | `a b` | INT | Not equal (numeric-first, string fallback) |
| `_lt` | `a b` | INT | Less than (numeric) |
| `_gt` | `a b` | INT | Greater than (numeric) |
| `_le` | `a b` | INT | Less than or equal (numeric) |
| `_ge` | `a b` | INT | Greater than or equal (numeric) |
| `_and` | `a b` | INT | Logical AND |
| `_or` | `a b` | INT | Logical OR |
| `_not` | `a` | INT | Logical NOT |
| `_if` | `cond _then cmd [_else cmd]` | varies | Conditional execution |
| `_switch` | `val case1 cmd1 [... _default cmdN]` | varies | Multi-way branch |
| `_goto` | `:label` | OK | Jump to label (from script start) |
| `_next` | `:label` or `cmd` | OK | Forward jump or execute sub-command |
| `_func` | `name "body"` | OK | Define arena-stored function |
| `_return` | `[value]` | special | Return from function, push value |
| `_list` | `[vars\|funcs]` | OK | List arena variables and/or functions |
| `_lpad` | `str width` | OK | Left-pad output for alignment |

## Design boundaries

These are intentional tradeoffs for code size:

- **Single-command `_if` branches.** Each `_then` or `_else` branch
  executes one command (with arguments). For multi-step branches, call
  a function or use `_goto`.

- **Paren result buffer is 32 bytes.** Parenthesized subexpression
  results are truncated to 31 characters. The engine handles control
  values, not large strings.

- **No string operations.** No substring, concatenation, or string
  length builtins. Use C commands for string processing if needed.

- **No floating point.** All math is integer. Use C commands for
  floating-point calculations.

- **Arena is not defragmented.** Variable overwrites may shift heap
  entries. Arena usage is bounded but not compacted.

- **No nested `_if`.** `_if` does not nest directly. Use functions or
  labels to express complex branching.

- **No local variables.** All variables share a single namespace in
  the arena. Function calls use positional parameters (`@1`, `@2`)
  for argument passing.

## Build configuration

| Item | Value |
|------|-------|
| Feature flag | `XELP_ENABLE_SCRIPT` |
| Requires | `XELP_ENABLE_CLI` |
| Code size (ARM Thumb) | ~9,200 bytes total / ~6,100 bytes delta over HIST |
| Code size (rv32) | ~10,970 bytes total / ~7,300 bytes delta over HIST |
| Code size (ESP32-S3) | ~9,542 bytes total / ~6,400 bytes delta over HIST |
| RAM per instance | arena size + struct fields (~20 bytes overhead) |
| Arena default | `sizeof(int) * 256` (512 / 1024 / 2048 bytes) |
| Arena override | `-DXELP_SCRIPT_ARENA_SZ=<n>` |

## See also

- [Tutorial](tutorial.md) -- step-by-step xelp introduction (core CLI)
- [API Reference](api-reference.md) -- all public functions and macros
- [Configuration Guide](configuration.md) -- compile-time flags
- [Examples](examples.md) -- annotated code for various platforms

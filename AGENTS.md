# AGENTS.md -- AI Coding Agent Reference for xelp

This file provides the essential context an AI coding agent needs to
generate correct xelp code. Read this instead of the full source when
integrating xelp into a project.

## What xelp is

A command line interpreter and script engine for embedded systems. Pure C,
zero dynamic memory, no OS, no stdlib. Three files: `xelp.c`, `xelp.h`,
`xelpcfg.h`. Compiles on 8-bit through 64-bit targets with any C89+
compiler.

## Critical constraints

- **No malloc, no heap.** Never suggest `malloc`, `calloc`, `strdup`,
  `asprintf`, or any dynamic allocation in xelp-related code.
- **No stdlib string functions.** xelp provides its own: `XELPStrLen`,
  `XELPStrEq`, `XELPStr2Int`, `XELPParseNum`. Do not use `strlen`,
  `strcmp`, `atoi`, `strtol`, or `sprintf` unless the user's platform
  already links them.
- **No printf.** xelp outputs one character at a time through a
  user-supplied `void (*)(char)` function. Use `XELPOut()` to print
  strings.
- **Strings stored by pointer.** Prompt strings and about messages passed
  to `XELP_SET_VAL_CLI_PROMPT()` and `XELP_SET_ABOUT()` are stored by
  pointer, not copied. They must be null-terminated and must remain valid
  for the life of the instance (string literals, static buffers, or
  globals -- never stack-local buffers that go out of scope).

## Function signatures (v0.3.0+)

### CLI command functions

```c
XELPRESULT my_command(XELP *ths, const char *args, int len) {
    /* ths  -- the invoking xelp instance (use for XELPOut, registers, etc.)
       args -- raw argument string (not null-terminated, use len)
       len  -- length of args in bytes */
    XELPOut(ths, "Hello\n", 0);
    return XELP_S_OK;
}
```

### KEY command functions

```c
XELPRESULT my_key_handler(XELP *ths, int key) {
    /* ths -- the invoking xelp instance
       key -- the ASCII key that was pressed */
    (void)key;
    XELPOut(ths, "Key pressed\n", 0);
    return XELP_S_OK;
}
```

### Command tables

```c
XELPCLIFuncMapEntry cli_commands[] = {
    { &my_command,  "mycmd",  "description for help" },
    { &other_cmd,   "other",  "another command"      },
    XELP_FUNC_ENTRY_LAST   /* required terminator */
};

XELPKeyFuncMapEntry key_commands[] = {
    { &my_key_handler, '?', "show help"  },
    { &toggle_led,     'l', "toggle LED" },
    XELP_FUNC_ENTRY_LAST
};
```

**Note:** v0.2.x used different signatures without the `XELP *ths`
parameter. All current code must use the v0.3.0+ signatures shown above.

## Setup pattern

Every xelp integration follows this pattern:

```c
#include "xelp.h"

void uart_putc(char c)  { /* write one char to your hardware */ }
void uart_bksp(void)    { uart_putc('\b'); uart_putc(' '); uart_putc('\b'); }

XELP cli;

void init(void) {
    XELPInit(&cli, "My Device v1.0");     /* 1. init instance           */
    XELP_SET_FN_OUT(cli, &uart_putc);     /* 2. set output function     */
    XELP_SET_FN_BKSP(cli, &uart_bksp);   /* 3. set backspace handler   */
    XELP_SET_FN_CLI(cli, cli_commands);   /* 4. register command table  */
    XELP_SET_FN_KEY(cli, key_commands);   /* 5. register key table      */
}

void loop(void) {
    if (char_available())
        XELPParseKey(&cli, read_char());  /* 6. feed chars one at a time */
}
```

## Parsing arguments inside commands

```c
XELPRESULT cmd_set(XELP *ths, const char *args, int len) {
    XelpBuf b, tok;
    XELP_XB_INIT(b, args, len);

    /* Token 0 = command name ("set"), token 1 = first arg, etc. */
    XELPTokN(&b, 1, &tok);
    int value = XELPStr2Int(tok.s, tok.p - tok.s);

    /* Count tokens */
    int n;
    XELPNumToks(&b, &n);  /* n includes the command name */

    return XELP_S_OK;
}
```

## Running scripts

Scripts are const strings parsed without modification (ROM-safe):

```c
const char *script = "hello; set mode 1; echo done";
XELPParse(&cli, script, XELPStrLen(script));
```

Separators: `;` and `\n`. Comments: `#` to end of line.

## Output from commands

Always use `XELPOut(ths, ...)` -- never hardcode a global instance:

```c
/* CORRECT: works with any instance */
XELPOut(ths, "Status: OK\n", 0);

/* WRONG: breaks multi-instance */
XELPOut(&my_global_cli, "Status: OK\n", 0);
```

`XELPOut(ths, msg, maxlen)`: if `maxlen > 0`, prints at most that many
characters. If `maxlen <= 0`, prints until null terminator.

## Multiple instances

xelp uses no global state. Each instance is independent:

```c
XELP cli_serial;
XELP cli_ble;

XELPInit(&cli_serial, "Serial Console");
XELPInit(&cli_ble,    "BLE Console");

XELP_SET_FN_OUT(cli_serial, &serial_putc);
XELP_SET_FN_OUT(cli_ble,    &ble_putc);

/* Same command table works for both -- ths routes output correctly */
XELP_SET_FN_CLI(cli_serial, commands);
XELP_SET_FN_CLI(cli_ble,    commands);
```

## Return codes

| Constant             | Value | Meaning                       |
|----------------------|-------|-------------------------------|
| `XELP_S_OK`         | 0     | Success                       |
| `XELP_W_WARN`       | 1     | Warning (still success)       |
| `XELP_S_NOTFOUND`   | 2     | Token or command not found    |
| `XELP_E_ERR`        | -1    | General error                 |
| `XELP_E_CMDBUFFULL` | -2    | Command buffer full           |
| `XELP_E_CMDNOTFOUND`| -3    | Command not found in dispatch |

`XELP_T_OK(r)` returns true if `r >= 0`.

## Compile-time configuration

Edit `src/xelpcfg.h` to enable/disable features:

| Flag               | Purpose                          | Size impact    |
|--------------------|----------------------------------|----------------|
| `XELP_ENABLE_CLI`  | CLI mode + scripting             | Core (~2 KB)   |
| `XELP_ENABLE_KEY`  | Single keypress mode             | ~200-500 bytes |
| `XELP_ENABLE_THR`  | Pass-through mode                | ~50-125 bytes  |
| `XELP_ENABLE_HELP` | Built-in help command            | ~180-350 bytes |
| `XELP_ENABLE_FULL` | All of the above                 | All combined   |

Buffer size: `XELP_CMDBUFSZ` (default 64 bytes).

## Three modes

- **CLI** (default): Line-buffered input with prompt. Type commands, press ENTER.
- **KEY**: Each keypress triggers a command immediately. For menus.
- **THR**: All keys pass through to another peripheral.

Default mode switch keys: ESC (KEY), CTRL-P (CLI), CTRL-T (THR).

## Common mistakes to avoid

1. Forgetting `XELP *ths` as the first parameter of command functions.
2. Using `printf` or `puts` instead of `XELPOut(ths, ...)`.
3. Passing stack-local strings to `XELP_SET_VAL_CLI_PROMPT()`.
4. Forgetting `XELP_FUNC_ENTRY_LAST` at the end of command tables.
5. Treating `args` as null-terminated (it is not -- use `len`).
6. Hardcoding `&global_instance` instead of using `ths` in commands.
7. Calling `malloc` or stdlib functions in embedded contexts.

## Registers (v0.3.1+)

Each XELP instance has 4 return registers (`mR[0..3]`), accessed via
macros. Convention: **callee-clobbers-all** -- any command call may
overwrite all registers. These are a return-value mailbox, NOT
computational registers (the VM has its own register file).

```c
/* Read after a command call */
XELPREG status = XELP_R0(cli);   /* engine writes: XELP_S_OK, etc. */
XELPREG val1   = XELP_R1(cli);   /* command-specific return value 1 */
XELPREG val2   = XELP_R2(cli);   /* command-specific return value 2 */
XELPREG val3   = XELP_R3(cli);   /* command-specific return value 3 */

/* Inside a command handler (pointer access) */
XELPRESULT cmd_divmod(XELP *ths, const char *args, int len) {
    /* ... parse a and b ... */
    ths->mR[1] = a / b;  /* quotient  */
    ths->mR[2] = a % b;  /* remainder */
    return XELP_S_OK;
}
```

C++ wrapper: `cli.r0()` (read-only), `cli.r1()`-`cli.r3()` (read/write).

## File structure

```
src/xelp.c       -- implementation (~700 lines)
src/xelp.h       -- public API (types, macros, function declarations)
src/xelpcfg.h    -- compile-time feature flags and settings
```

Add these three files to your build. No other dependencies.

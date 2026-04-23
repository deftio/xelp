# Tutorial

A step-by-step introduction to xelp. By the end you will have a working
CLI with custom commands, KEY mode shortcuts, scripting, and token parsing.

## Prerequisites

- A C compiler (GCC, Clang, or any C89-compatible toolchain)
- The three xelp source files: `xelp.c`, `xelp.h`, `xelpcfg.h`

## 1. Hello World -- minimal CLI

The smallest useful xelp program: one command, one output function.

```c
#include "xelp.h"

/* Platform: write one char. Replace with your UART TX. */
void my_putc(char c) { putchar(c); }
void my_bksp(void)   { my_putc('\b'); my_putc(' '); my_putc('\b'); }

/* Your first command */
XELPRESULT cmd_hello(XELP *ths, const char *args, int len) {
    XELPOut(ths, "Hello, world!\n", 0);
    return XELP_S_OK;
}

/* Command table -- NULL-terminated with XELP_FUNC_ENTRY_LAST */
XELPCLIFuncMapEntry commands[] = {
    { &cmd_hello, "hello", "say hello" },
    XELP_FUNC_ENTRY_LAST
};

XELP cli;

int main(void) {
    XELPInit(&cli, "Tutorial v1.0");
    XELP_SET_FN_OUT(cli, &my_putc);
    XELP_SET_FN_BKSP(cli, &my_bksp);
    XELP_SET_FN_CLI(cli, commands);

    /* Feed characters from stdin (or UART, BLE, etc.) */
    int c;
    while ((c = getchar()) != EOF)
        XELPParseKey(&cli, (char)c);
    return 0;
}
```

Compile and run:

```bash
gcc -Isrc src/xelp.c tutorial.c -o tutorial
./tutorial
```

Type `hello` and press ENTER. You should see `Hello, world!`.

### What just happened

1. `XELPInit` zeroes all internal state and stores the about message.
2. `XELP_SET_FN_OUT` tells xelp how to emit characters on this platform.
3. `XELP_SET_FN_BKSP` tells xelp how to handle destructive backspace.
4. `XELP_SET_FN_CLI` registers your command table.
5. `XELPParseKey` feeds one character at a time into xelp's state machine.
   When ENTER is received, xelp tokenizes the line and dispatches to the
   matching command function.

## 2. Commands with arguments

Command functions receive the raw argument string and its length. Use
`XelpArgs` to iterate tokens left-to-right:

```c
XELPRESULT cmd_add(XELP *ths, const char *args, int len) {
    XelpArgs a;
    int x, y;

    (void)ths;
    XelpArgsInit(&a, args, len);
    XelpNextTok(&a, 0);              /* skip command name ("add") */
    XelpNextInt(&a, &x);
    XelpNextInt(&a, &y);

    /* Output the result (xelp has no printf -- format manually or
       use your platform's sprintf into a buffer, then XELPOut) */
    /* ... x + y ... */
    return XELP_S_OK;
}
```

Register it:

```c
{ &cmd_add, "add", "add <a> <b>" },
```

Usage: `add 10 25`

### Tokenizer rules

- Tokens are separated by whitespace (spaces, tabs)
- Quoted strings are a single token: `echo "hello world"`
- `#` starts a comment (rest of line ignored)
- `;` separates statements on one line: `hello; add 1 2`
- Backtick (`` ` ``) escapes the next character at the CLI
- Backslash (`\`) escapes inside quoted strings

## 3. Counting and iterating tokens

Using `XelpArgs` to walk all tokens sequentially:

```c
XELPRESULT cmd_args(XELP *ths, const char *args, int len) {
    XelpArgs a;
    XelpBuf tok;
    int n;

    XelpArgsInit(&a, args, len);
    XelpArgCount(&a, &n);            /* n includes the command name */

    while (XelpNextTok(&a, &tok) == XELP_S_OK) {
        XELPOut(ths, tok.s, tok.p - tok.s);
        XELPOut(ths, "\n", 0);
    }
    return XELP_S_OK;
}
```

For random access by index (e.g. "get the 3rd argument"), use `XELPTokN`:

```c
XelpBuf b, tok;
XELP_XB_INIT(b, (char*)args, len);  /* cast required: XELP_XB_INIT takes char* */
XELPTokN(&b, 2, &tok);              /* 0-indexed: token 2 = third token */
```

## 4. KEY mode -- single keypress actions

KEY mode is for menus and quick toggles. Each keypress fires a function
immediately (no ENTER needed):

```c
XELPRESULT key_help(XELP *ths, XELPKEYCODE c) {
    (void)c;
    return XELPHelp(ths);
}

XELPRESULT key_toggle_led(XELP *ths, XELPKEYCODE c) {
    (void)c;
    /* toggle your LED here */
    XELPOut(ths, "LED toggled\n", 0);
    return XELP_S_OK;
}

XELPKeyFuncMapEntry key_commands[] = {
    { &key_help,       '?', "show help"    },
    { &key_toggle_led, 'l', "toggle LED"   },
    XELP_FUNC_ENTRY_LAST
};
```

Register with `XELP_SET_FN_KEY(cli, key_commands)`.

### Switching modes

By default:
- **ESC** switches to KEY mode
- **CTRL-P** switches to CLI mode
- **CTRL-T** switches to THR (pass-through) mode

These keys are configurable in `xelpcfg.h`.

## 5. Scripting

Any sequence of commands can be run as a script. Scripts are const
strings -- they can live in ROM on embedded targets:

```c
const char *startup = "hello; add 10 20; echo done";
XELPParse(&cli, startup, XELPStrLen(startup));
```

Multi-line scripts work too:

```c
const char *script =
    "# configuration script\n"
    "set mode 1\n"
    "set gain 50\n"
    "echo config complete\n";
XELPParse(&cli, script, XELPStrLen(script));
```

### XelpBuf variant

For scripts already in an `XelpBuf`:

```c
XelpBuf xb;
XELP_XB_INIT(xb, script, XELPStrLen(script));
XELPParseXB(&cli, &xb);
```

## 6. Help system

If `XELP_ENABLE_HELP` is defined in `xelpcfg.h`, calling `XELPHelp(&cli)`
prints a listing of all registered KEY and CLI commands with their help
strings. This is why the third field in every command table entry matters:

```c
{ &cmd_hello, "hello", "say hello" },
/*                      ^^^^^^^^^^^ shown in help output */
```

## 7. Multiple instances

xelp uses no global state. You can run independent CLIs on different
serial ports:

```c
XELP debug_cli;
XELP field_cli;

XELPInit(&debug_cli, "Debug Console");
XELPInit(&field_cli, "Field Service");

XELP_SET_FN_OUT(debug_cli, &uart0_putc);
XELP_SET_FN_OUT(field_cli, &uart1_putc);

XELP_SET_FN_CLI(debug_cli, debug_commands);
XELP_SET_FN_CLI(field_cli, field_commands);

XELP_SET_VAL_CLI_PROMPT(debug_cli, "dbg>");
XELP_SET_VAL_CLI_PROMPT(field_cli, "svc>");
```

Each instance has its own command buffer, mode state, registers, and
output function. See `examples/multi-instance/` for a complete example.

## 8. Numeric parsing

xelp supports decimal and hexadecimal:

```c
int val;

/* Quick conversion (no error checking) */
val = XELPStr2Int("255", 3);       /* decimal: 255 */
val = XELPStr2Int("FFh", 3);       /* hex suffix: 255 */
val = XELPStr2Int("0xFF", 4);      /* hex prefix: 255 */
val = XELPStr2Int("0x1A", 4);      /* uppercase hex: 26 */

/* Safer -- returns XELP_S_OK on success */
XELPRESULT r = XELPParseNum("0xFF", 4, &val);
if (XELP_T_OK(r)) {
    /* val == 255 */
}
```

## 9. THR (pass-through) mode

THR mode redirects all keystrokes to another peripheral -- useful for
interacting with a modem or a second MCU through the same terminal:

```c
void modem_send(char c) {
    /* forward to modem UART */
}

XELP_SET_FN_THR(cli, &modem_send);
```

When the user presses CTRL-T, all subsequent keystrokes go to
`modem_send()` instead of the xelp parser. Press CTRL-P to return to
CLI mode.

## 10. Line editing

When `XELP_ENABLE_LINE_EDIT` is defined in `xelpcfg.h`, the CLI prompt
supports cursor movement and mid-line editing:

- **Left/Right arrow** -- move cursor within the line
- **Home/End** -- jump to beginning/end of line
- **Delete** -- delete character at cursor
- **Insert** -- insert characters at cursor position (shift model)

Multi-byte key sequences (arrow keys, Home/End, Delete) are automatically
recognized by xelp's key accumulator. No terminal configuration needed
beyond standard VT100/ANSI support.

Without `XELP_ENABLE_LINE_EDIT`, CLI uses append-only input with
backspace via the `mpfBksp` callback.

## 11. Mode change callback

Get notified when the user switches modes:

```c
void on_mode_change(int mode) {
    if (mode == XELP_MODE_CLI)
        XELPOut(&cli, "[CLI]\n", 0);
    else if (mode == XELP_MODE_KEY)
        XELPOut(&cli, "[KEY]\n", 0);
    else if (mode == XELP_MODE_THR)
        XELPOut(&cli, "[THR]\n", 0);
}

XELP_SET_FN_EMCHG(cli, &on_mode_change);
```

## 12. Registers -- returning values from commands

Every XELP instance has 4 integer registers (`mR[0]`..`mR[3]`).
The engine writes R0 with the command's return code after every dispatch.
R1-R3 are available for commands to return extra values to the caller.

**Convention: callee-clobbers-all** -- any command call may overwrite all
registers, so read them immediately after dispatch.

### Reading R0 after a command

```c
XELPParse(&cli, "hello", 5);
if (XELP_R0(cli) == XELP_S_OK) {
    /* command succeeded */
}
```

### Writing R1-R3 in a command handler

```c
XELPRESULT cmd_divmod(XELP *ths, const char *args, int len) {
    int a = 17, b = 5;
    ths->mR[1] = a / b;  /* quotient  -> R1 */
    ths->mR[2] = a % b;  /* remainder -> R2 */
    return XELP_S_OK;    /* engine writes R0 */
}
```

### Reading results

```c
XELPParse(&cli, "divmod 17 5", 11);
int quot = XELP_R1(cli);   /* 3 */
int rem  = XELP_R2(cli);   /* 2 */
```

## Next steps

- [API Reference](api-reference.md) -- complete function and macro documentation
- [Configuration Guide](configuration.md) -- all compile-time options
- [Porting Guide](porting.md) -- platform-specific integration notes
- [Examples](examples.md) -- annotated example code for various platforms

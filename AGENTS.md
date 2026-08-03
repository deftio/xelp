# AGENTS.md -- AI Coding Agent Reference for xelp

This file provides the essential context an AI coding agent needs to
generate correct xelp code, write tests, port to new platforms, and
understand the internal architecture. Read this instead of the full
source when integrating xelp into a project.

For the llmstxt.org-format project index, see [llms.txt](llms.txt).

## What xelp is

A command line interpreter, script engine, and single-key menu system
for embedded systems. Pure C, zero dynamic memory, no OS, no stdlib.
Three files: `xelp.c`, `xelp.h`, `xelpcfg.h`. Compiles on 8-bit
through 64-bit targets with any C89+ compiler. Version 0.4.1.

## Critical constraints

- **No malloc, no heap.** Never suggest `malloc`, `calloc`, `strdup`,
  `asprintf`, or any dynamic allocation in xelp-related code.
- **No stdlib string functions.** xelp provides its own: `XelpStrLen`,
  `XelpStrEq`, `XelpStrEq2`, `XelpStr2Int`, `XelpParseNum`,
  `XelpBufCmp`. Do not use `strlen`, `strcmp`, `atoi`, `strtol`, or
  `sprintf` unless the user's platform already links them.
- **No printf.** xelp outputs one character at a time through a
  user-supplied `void (*)(char)` function. Use `XelpOut()` to print
  strings. Use `XelpPutc()` for single characters.
- **Strings stored by pointer.** Prompt strings and about messages
  passed to `XELP_SET_VAL_CLI_PROMPT()` and `XELP_SET_ABOUT()` are
  stored by pointer, not copied. They must be null-terminated and must
  remain valid for the life of the instance (string literals, static
  buffers, or globals -- never stack-local buffers that go out of scope).
- **C89/C90 compatibility.** No `//` comments in `src/` files. No
  mixed declarations and code. No VLAs. No GCC extensions.
- **No globals.** All state goes through the `XELP *ths` instance
  pointer. Multiple instances must run independently.

---

## Architecture overview

```
                          xelp architecture
                          =================

  Hardware byte stream (UART RX, BLE, USB CDC, etc.)
        |
        v
  XelpParseKey(ths, char)        <-- interactive: one byte at a time
        |
        +-- _xelpKeyAccum()      <-- assembles multi-byte ANSI sequences
        |       (ESC [ A -> XELP_KEYCODE_UP, etc.)
        |
        +-- mode switch check    <-- ESC/CTRL-P/CTRL-T
        |
        +-- dispatch by mode:
            |
            +-- KEY mode: XelpExecKC() -> key table lookup -> handler(ths, keycode)
            |
            +-- THR mode: mpfPassThru(char)
            |
            +-- CLI mode: line buffer -> on ENTER:
                    |
                    +-- _xelpHistSave()     (optional, if XELP_ENABLE_HISTORY)
                    |
                    +-- XelpParseXB()       <-- tokenize + dispatch
                            |
                            +-- XelpTokLineXB()  <-- PSM state machine tokenizer
                            |
                            +-- command table linear search
                            |
                            +-- _xelpBuf2Argv()  <-- tokenize into argc/argv
                            |
                            +-- handler(ths, argc, argv)

  XelpParse(ths, buf, len)       <-- scripting: parse entire buffer at once
        |
        +-- wraps XelpParseXB()  <-- same tokenizer + dispatch pipeline
```

## Three modes

| Mode | Constant | Description | Compile flag |
|------|----------|-------------|-------------|
| CLI | `XELP_MODE_CLI` (0x00) | Line-buffered input with prompt. Type commands, press ENTER. | `XELP_ENABLE_CLI` |
| KEY | `XELP_MODE_KEY` (0x01) | Each keypress triggers a command immediately. For menus. | `XELP_ENABLE_KEY` |
| THR | `XELP_MODE_THR` (0x02) | All keys pass through to another peripheral. | `XELP_ENABLE_THR` |

Default mode switch keys: **ESC** -> KEY, **CTRL-P** -> CLI,
**CTRL-T** -> THR. Configurable in `xelpcfg.h`. If a mode is not
compiled in, its switch key is silently ignored.

---

## Function signatures (v0.4.0+)

### CLI command functions

```c
XELPRESULT my_command(XELP *ths, int argc, const char **argv) {
    /* ths  -- the invoking xelp instance (use for XelpOut, registers, etc.)
       argc -- number of arguments (including command name)
       argv -- null-terminated argument strings (argv[0] = command name) */
    XelpOut(ths, "Hello\n", 0);
    return XELP_S_OK;
}
```

### KEY command functions

```c
XELPRESULT my_key_handler(XELP *ths, XELPKEYCODE key) {
    /* ths -- the invoking xelp instance
       key -- XELPKEYCODE (unsigned long): single-char key or packed
              multi-byte ANSI sequence (e.g. XELP_KEYCODE_UP) */
    (void)key;
    XelpOut(ths, "Key pressed\n", 0);
    return XELP_S_OK;
}
```

Multi-byte key constants: `XELP_KEYCODE_UP`, `XELP_KEYCODE_DOWN`,
`XELP_KEYCODE_LEFT`, `XELP_KEYCODE_RIGHT`, `XELP_KEYCODE_HOME`,
`XELP_KEYCODE_END`, `XELP_KEYCODE_INS`, `XELP_KEYCODE_KDEL`,
`XELP_KEYCODE_PGUP`, `XELP_KEYCODE_PGDN`.
Single-char keys are their natural value (`'?'` == 0x3F).
`XELP_KC_IS_MULTI(k)` returns true if the keycode is multi-byte (>= 0x100).

### Command tables

```c
XELPCLIFuncMapEntry cli_commands[] = {
    { &my_command,  "mycmd",  "description for help" },
    { &other_cmd,   "other",  "another command"      },
    XELP_FUNC_ENTRY_LAST   /* required terminator */
};

XELPKeyFuncMapEntry key_commands[] = {
    { &my_key_handler, '?',              "show help"  },
    { &toggle_led,     'l',              "toggle LED" },
    { &on_arrow_up,    XELP_KEYCODE_UP,  "scroll up"  },
    XELP_FUNC_ENTRY_LAST
};
```

**Note:** v0.3.x used `(XELP *ths, const char *args, int len)` signatures.
v0.4.0 changed to native `(XELP *ths, int argc, const char **argv)`.
The dispatch engine now tokenizes the command line before calling handlers.

### Default handlers

Called when no command/key matches. Optional.

```c
XELPRESULT default_cli(XELP *ths, int argc, const char **argv) {
    XelpOut(ths, "Unknown command\n", 0);
    return XELP_E_CMDNOTFOUND;
}

XELPRESULT default_key(XELP *ths, XELPKEYCODE key) {
    (void)key;
    return XELP_S_NOTFOUND;
}

XELP_SET_FN_DEF_CLI(cli, &default_cli);
XELP_SET_FN_DEF_KEY(cli, &default_key);
```

---

## Setup pattern

Every xelp integration follows this pattern:

```c
#include "xelp.h"

void uart_putc(char c)  { /* write one char to your hardware */ }
void uart_bksp(void)    { uart_putc('\b'); uart_putc(' '); uart_putc('\b'); }

XELP cli;

void init(void) {
    XelpInit(&cli, "My Device v1.0");     /* 1. init instance           */
    XELP_SET_FN_OUT(cli, &uart_putc);     /* 2. set output function     */
    XELP_SET_FN_BKSP(cli, &uart_bksp);   /* 3. set backspace handler   */
    XELP_SET_FN_CLI(cli, cli_commands);   /* 4. register command table  */
    XELP_SET_FN_KEY(cli, key_commands);   /* 5. register key table      */
}

void loop(void) {
    if (char_available())
        XelpParseKey(&cli, read_char());  /* 6. feed chars one at a time */
}
```

### All setup macros

| Macro | Purpose |
|-------|---------|
| `XELP_SET_FN_OUT(ths, fn)` | Set output function: `void fn(char)` **(required)** |
| `XELP_SET_FN_ERR(ths, fn)` | Set error output function (optional) |
| `XELP_SET_FN_BKSP(ths, fn)` | Set backspace handler: `void fn(void)` |
| `XELP_SET_FN_THR(ths, fn)` | Set pass-through function |
| `XELP_SET_FN_EMCHG(ths, fn)` | Set mode-change callback: `void fn(int)` |
| `XELP_SET_FN_CLI(ths, tbl)` | Set CLI command table |
| `XELP_SET_FN_KEY(ths, tbl)` | Set KEY command table |
| `XELP_SET_FN_DEF_CLI(ths, fn)` | Set default CLI handler |
| `XELP_SET_FN_DEF_KEY(ths, fn)` | Set default KEY handler |
| `XELP_SET_VAL_CLI_PROMPT(ths, str)` | Set prompt (stored by pointer) |
| `XELP_SET_ABOUT(ths, str)` | Set about/help message (stored by pointer) |

---

## Using arguments inside commands

Since v0.4.0, CLI handlers receive native `argc`/`argv` — the dispatch
engine tokenizes the command line before calling the handler. Tokens
are null-terminated strings in `argv[]`. `argv[0]` is the command name.

```c
XELPRESULT cmd_set(XELP *ths, int argc, const char **argv) {
    if (argc < 3) { XelpOut(ths, "usage: set key value\n", 0); return XELP_E_ERR; }
    const char *key = argv[1];
    int value;
    XelpArgvInt(argv, argc, 2, &value);
    /* ... use key and value ... */
    return XELP_S_OK;
}
```

### Argument helpers

| Function | Purpose |
|----------|---------|
| `XelpArgvInt(argv, argc, n, &val)` | Get argv[n] as int (bounds-checked) |
| `XelpArgvStr(argv, argc, n, &s, &slen)` | Get argv[n] as string pointer + length (bounds-checked) |

`XELP_ARGV_MAX` (default 16) limits the maximum number of argv entries.
Tokens are null-terminated. Quoted strings are unquoted and escape
sequences are processed by the dispatch tokenizer.

---

## Running scripts

Scripts are const strings parsed without modification (ROM-safe):

```c
const char *script = "hello; set mode 1; echo done";
XelpParse(&cli, script, XelpStrLen(script));
```

Separators: `;` and `\n`. Comments: `#` to end of line.
Quoted strings: `"hello world"` is a single token.
Escape chars: backtick (`` ` ``) at CLI, backslash (`\`) inside quotes.

Multi-line scripts:

```c
const char *script =
    "# startup configuration\n"
    "set mode 1\n"
    "set gain 50\n"
    "echo config complete\n";
XelpParse(&cli, script, XelpStrLen(script));
```

---

## Output from commands

Always use `XelpOut(ths, ...)` -- never hardcode a global instance:

```c
/* CORRECT: works with any instance */
XelpOut(ths, "Status: OK\n", 0);

/* WRONG: breaks multi-instance */
XelpOut(&my_global_cli, "Status: OK\n", 0);
```

`XelpOut(ths, msg, maxlen)`: if `maxlen > 0`, prints at most that many
characters. If `maxlen <= 0`, prints until null terminator.

`XelpPutc(ths, c)`: single-character output through the instance's output
function. Respects `mOutEnable`. Use instead of `XelpOut` for single chars.

---

## Multiple instances

xelp uses no global state. Each instance is independent:

```c
XELP cli_serial;
XELP cli_ble;

XelpInit(&cli_serial, "Serial Console");
XelpInit(&cli_ble,    "BLE Console");

XELP_SET_FN_OUT(cli_serial, &serial_putc);
XELP_SET_FN_OUT(cli_ble,    &ble_putc);

/* Same command table works for both -- ths routes output correctly */
XELP_SET_FN_CLI(cli_serial, commands);
XELP_SET_FN_CLI(cli_ble,    commands);
```

---

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

---

## Registers (v0.4.0+)

Each XELP instance has 4 return registers (`mR[0..3]`), accessed via
macros. Convention: **callee-clobbers-all** -- any command call may
overwrite all registers. These are a return-value mailbox.

```c
/* Read after a command call (struct access via macros) */
XELPREG status = XELP_R0(cli);   /* engine writes: XELP_S_OK, etc. */
XELPREG val1   = XELP_R1(cli);   /* command-specific return value 1 */
XELPREG val2   = XELP_R2(cli);   /* command-specific return value 2 */
XELPREG val3   = XELP_R3(cli);   /* command-specific return value 3 */

/* Inside a command handler (pointer access) */
XELPRESULT cmd_divmod(XELP *ths, int argc, const char **argv) {
    int a, b;
    XelpArgvInt(argv, argc, 1, &a);
    XelpArgvInt(argv, argc, 2, &b);
    ths->mR[1] = a / b;  /* quotient  */
    ths->mR[2] = a % b;  /* remainder */
    return XELP_S_OK;
}
```

C++ wrapper: `cli.r0()` (read-only), `cli.r1()`-`cli.r3()` (read/write).

---

## Output control

Two `char` fields in the XELP struct control output behavior:

### Output enable (`mOutEnable`)

Gates **ALL** output (XelpOut, echo, prompt, help, redraw).
Set via `XELP_SET_OUT_ENABLE(ths, val)`. Default: 1 (enabled).
Set to 0 for silent scripting / batch mode.

### Echo control (`mEchoChar`)

Controls how printable chars are echoed during interactive input.
Does NOT affect XelpOut, ENTER newline, cursor movement, or prompt.
- `XELP_ECHO_NORMAL` (`'\0'`) -- echo as typed (default)
- `XELP_ECHO_OFF` (`'\1'`) -- suppress echo
- Any other char (e.g. `'*'`) -- mask: echo that char instead

Password entry pattern:
```c
XELP_SET_ECHO(*ths, '*');          /* mask during input */
/* ... user types, sees ****** ... */
XELP_SET_ECHO(*ths, XELP_ECHO_NORMAL);  /* restore after ENTER */
```

---

## Compile-time configuration

Edit `src/xelpcfg.h` to enable/disable features:

| Flag | Purpose | Size impact |
|------|---------|-------------|
| `XELP_ENABLE_CLI` | CLI mode + scripting | Core (~1.5-2.6 KB) |
| `XELP_ENABLE_LINE_EDIT` | Cursor movement, insert, delete | ~800-1000 bytes |
| `XELP_ENABLE_HISTORY` | Command history (UP/DOWN recall) | ~550 bytes |
| `XELP_ENABLE_KEY` | Single keypress mode | ~200-500 bytes |
| `XELP_ENABLE_THR` | Pass-through mode | ~50-125 bytes |
| `XELP_ENABLE_HELP` | Built-in help command | ~180-350 bytes |
| `XELP_ENABLE_FULL` | All of the above (except history) | All combined |

### Buffer and register sizes

All of these are defined in `xelpcfg.h` and are overridable via compiler
flag (`-DXELP_CMDBUFSZ=128`) or `xelp_ovr.h`.

| Setting | Default | Purpose |
|---------|---------|---------|
| `XELP_CMDBUFSZ` | 64 | CLI input buffer size (bytes). In `xelpcfg.h`, `#ifndef`-guarded. |
| `XELP_ARGV_MAX` | 16 | Maximum arguments in argc/argv dispatch. In `xelpcfg.h`, `#ifndef`-guarded. |
| `XELP_ARGVBUFSZ` | `XELP_CMDBUFSZ` | Scratch buffer for argv tokenization (bytes per instance). |
| `XELP_REGS_SZ` | 4 | Return registers per instance (minimum 4) |
| `XELPREG` | `int` | Register element type |
| `XELP_HIST_DEPTH` | 4 | History ring buffer depth |

### Key mappings

| Setting | Default | Purpose |
|---------|---------|---------|
| `XELPKEY_CLI` | CTRL-P | Switch to CLI mode |
| `XELPKEY_KEY` | ESC | Switch to KEY mode |
| `XELPKEY_THR` | CTRL-T | Switch to THR mode |

### Escape characters

| Setting | Default | Purpose |
|---------|---------|---------|
| `XELP_CLI_ESC` | `` ` `` (backtick) | Escape next char in CLI/scripts |
| `XELP_QUO_ESC` | `\` (backslash) | Escape next char inside quoted strings |
| `XELP_ESC_MAP` | `"n\x0A" "t\x09" ""` | Packed key-value pairs for escape expansion in quoted strings (argv tokenizer). Each 2-byte entry maps the char after `XELP_QUO_ESC` to a replacement byte. Terminated by `'\0'`. Unmapped escapes pass through as identity. Set to `""` to disable. |

### ENTER key detection

| Setting | Default | Purpose |
|---------|---------|---------|
| `XELP_ENTER_LF` | 1 | Accept `\n` (0x0A) as ENTER |
| `XELP_ENTER_CR` | 1 | Accept `\r` (0x0D) as ENTER |

Both enabled by default for cross-platform use. Only affects interactive
input (`XelpParseKey`); script parsing always uses `\n`.

### Prompt configuration

```c
/* Fixed prompt (all instances share it) -- default */
#define XELP_CLI_PROMPT  "xelp>"

/* Per-instance prompt */
#define XELP_CLI_PROMPT  (ths->mpPrompt)
/* Then use: XELP_SET_VAL_CLI_PROMPT(cli, "mydev>"); */
```

### Help string customization

| Setting | Default | Purpose |
|---------|---------|---------|
| `XELP_HELP_KEY_STR` | `"\nKey functions\n"` | Section header for KEY commands |
| `XELP_HELP_CLI_STR` | `"\nCLI functions\n"` | Section header for CLI commands |
| `XELP_HELP_ABT_STR` | `(ths->mpAboutMsg)` | About message at top of help |

### Configuration override system

To customize without modifying source files:

1. Define `-DXELP_CONFIG_OVERRIDE` in compiler flags
2. Create `xelp_ovr.h` in your include path
3. Use `#undef` / `#define` to change values

```c
/* xelp_ovr.h example */
#undef  XELP_CLI_PROMPT
#define XELP_CLI_PROMPT   (ths->mpPrompt)

#undef  XELP_ENABLE_THR    /* disable THR mode */

#undef  XELP_HIST_DEPTH
#define XELP_HIST_DEPTH  8  /* 8 history entries instead of 4 */
```

---

## Parser internals

### PSM (Parser State Machine)

The tokenizer is a table-driven finite state machine in `xelp.c`. It
operates on the input buffer without modifying it (ROM-safe).

**8 states:**

| State | ID | Description |
|-------|----|-------------|
| `_PS_SEEK` | 0x00 | Seeking next token, skipping whitespace |
| `_PS_ESCA` | 0x01 | CLI escape sequence (backtick) |
| `_PS_TOK0` | 0x02 | Inside first token (command name) |
| `_PS_CMNT` | 0x03 | Inside single-line comment |
| `_PS_SEOL` | 0x04 | Seeking end-of-line after first token |
| `_PS_QUOT` | 0x05 | Inside quoted string |
| `_PS_QESC` | 0x06 | Escape char inside quoted string |
| `_PS_QEND` | 0x07 | Exiting quoted string |

**State table:** `gPSMStates[94]` -- each entry is 3 bytes:
`[char_to_match, exec_flags, next_state]`. Char 0 = default for state.

**Jump table:** `gPSMJumpTable[8]` -- maps state ID to offset in
`gPSMStates` for O(1) state lookup.

**Execution flags:**
- `_EF_TS` (0x01) -- mark token start
- `_EF_TE` (0x02) -- mark token end
- `_EF_LE` (0x04) -- mark line end

### Tokenizer function: `XelpTokLineXB()`

Two modes:
- `XELP_TOK_ONLY` -- returns next single token. Output: `tok.s` = start,
  `tok.p` = end of token.
- `XELP_TOK_LINE` -- returns entire line. Output: `tok.s` = command
  start, `tok.p` = command end, `tok.e` = end of line (including args).

### Key accumulator: `_xelpKeyAccum()`

Separate from the PSM. Assembles raw bytes into `XELPKEYCODE` values:
- Single chars complete immediately
- ESC triggers look-ahead: ESC + `[` enters CSI mode
- CSI sequences collect digits and terminate on letter or `~`
- Keycodes packed little-endian into `unsigned long`

### Command dispatch

`XelpParseXB()` loop:
1. Call `XelpTokLineXB()` with `XELP_TOK_LINE` to get next command line
2. Linear search through `mpCLIModeFuncs` table
3. First match (via `XelpStrEq2`) tokenizes line into argc/argv via `_xelpBuf2Argv`
4. Calls handler with `(ths, argc, argv)`
5. If no match and `mpfDefCLI` is set, call the default handler with argc/argv
5. Result stored in `ths->mR[0]`

---

## XelpBuf -- buffer wrapper

```c
typedef struct {
    char* s;  /* start of buffer */
    char* p;  /* current position / cursor */
    char* e;  /* end of buffer (s + length) */
} XelpBuf;
```

Invariant: `s <= p <= e`.

| Macro | Purpose |
|-------|---------|
| `XELP_XB_INIT(xb, buf, len)` | Init from pointer + length |
| `XELP_XB_INIT_PTRS(xb, s, p, e)` | Init from three pointers |
| `XELP_XB_INIT_BP(xb, buf, pos, len)` | Init with cursor offset |
| `XELP_XB_COPY(a, b)` | Copy a to b |
| `XELP_XB_PTR(xb)` | Get start pointer |
| `XELP_XB_LEN(xb)` | Total buffer length |
| `XELP_XB_POS(xb)` | Current position as int |
| `XELP_XB_PUTC(xb, ch)` | Write char with bounds check |
| `XELP_XB_PUTC_RAW(xb, ch)` | Write char without bounds check |
| `XELP_XB_GETC(xb, ch)` | Read char and advance |
| `XELP_XB_TOP(xb)` | Reset position to start |
| `XELP_XB_OUT(x, xb)` | Print XelpBuf via XelpOut |

There is also a `XelpBufC` (const variant) with `const char*` members.
`XelpBuf` is an alias for `XelpBufW` (writable).

---

## String utilities

| Function | Signature | Purpose |
|----------|-----------|---------|
| `XelpStrLen(c)` | `int (const char*)` | Length of null-terminated string |
| `XelpStrEq(buf, len, cmd)` | `XELPRESULT (const char*, int, const char*)` | Compare buffer to null-terminated string |
| `XelpStrEq2(buf, end, cmd)` | `XELPRESULT (const char*, const char*, const char*)` | Compare pointer-pair to null-terminated string |
| `XelpStr2Int(s, maxlen)` | `int (const char*, int)` | Parse string to int (returns 0 on error) |
| `XelpParseNum(s, maxlen, &n)` | `XELPRESULT (const char*, int, int*)` | Safer parse: returns result code |
| `XelpBufCmp(as, ae, bs, be, type)` | `XELPRESULT (...)` | Compare two buffers with null-term options |
| `XelpFindTok(x, t0s, t0e, type)` | `XELPRESULT (XelpBuf*, ...)` | Find matching token in buffer |

### Number parsing

`XelpParseNum` and `XelpStr2Int` accept:
- Decimal: `123`, `-45`, `+67`
- Hex prefix: `0xFF`, `0x1A`
- Hex suffix: `FFh`, `1Ah`
- Uppercase and lowercase hex digits

Overflow detection is included (returns `XELP_E_ERR` on overflow).

### Buffer comparison types

| Constant | Value | Behavior |
|----------|-------|----------|
| `XELP_CMP_TYPE_BUF` | 0x00 | Pure byte comparison, `\0` not special |
| `XELP_CMP_TYPE_A0` | 0x01 | Buffer A terminates on `\0` |
| `XELP_CMP_TYPE_A0B0` | 0x11 | Both buffers terminate on `\0` |

---

## XELP instance struct

Key fields (see `xelp.h` for full definition):

```c
typedef struct XELP_tag {
    int       mCurMode;              /* CLI / KEY / THR */
    char      mOutEnable;            /* 0=mute, nonzero=normal */
    char      mEchoChar;             /* '\0'=normal, '\1'=off, else mask */
    const char* mpAboutMsg;          /* help header string */
    XELPREG   mR[XELP_REGS_SZ];     /* return registers */

    /* CLI mode (if XELP_ENABLE_CLI) */
    XELPCLIFuncMapEntry *mpCLIModeFuncs;
    char      mCmdMsgBuf[XELP_CMDBUFSZ];
    XelpBuf   mCmdXB;

    /* KEY mode (if XELP_ENABLE_KEY) */
    XELPKeyFuncMapEntry *mpKeyModeFuncs;

    /* Key accumulator */
    XELPKEYCODE mKeyAccum;
    char      mKeyLen;

    /* Line editing (if XELP_ENABLE_LINE_EDIT) */
    char*     mCur;                  /* cursor in [mCmdXB.s .. mCmdXB.p] */

    /* History (if XELP_ENABLE_HISTORY) */
    char      mHistBuf[XELP_HIST_DEPTH][XELP_CMDBUFSZ];
    char      mHistWrite, mHistCount, mHistBrowse;
    char      mHistSaved[XELP_CMDBUFSZ];
    char      mHistSavedLen;

    /* Platform abstraction */
    void (*mpfOut)(char);            /* character output */
    void (*mpfErr)(char);            /* error output */
    void (*mpfEditModeChg)(int);     /* mode change callback */
    void (*mpfPassThru)(char);       /* THR mode forward */
    void (*mpfBksp)();               /* destructive backspace */
} XELP;
```

---

## Version information

```c
#define XELP_VERSION      (0x00000400UL)  /* 0x00MMmmpp */
#define XELP_VER_MAJOR(v) (((v) >> 16) & 0xFF)
#define XELP_VER_MINOR(v) (((v) >>  8) & 0xFF)
#define XELP_VER_PATCH(v) ( (v)        & 0xFF)
```

The version in `src/xelp.h` is the single source of truth.

---

## Common mistakes to avoid

1. Forgetting `XELP *ths` as the first parameter of command functions.
2. Using `printf` or `puts` instead of `XelpOut(ths, ...)`.
3. Passing stack-local strings to `XELP_SET_VAL_CLI_PROMPT()`.
4. Forgetting `XELP_FUNC_ENTRY_LAST` at the end of command tables.
5. Using old `(XELP*, const char*, int)` handler signatures instead of `(XELP*, int, const char**)`.
6. Hardcoding `&global_instance` instead of using `ths` in commands.
7. Calling `malloc` or stdlib functions in embedded contexts.
8. Using `//` comments in core source files (must use `/* */` for C89).
9. Mixed declarations and code in core source files.
10. Assuming `int` is a specific size (varies 16-bit to 64-bit).
11. Using `XELP_ENABLE_HISTORY` without `XELP_ENABLE_LINE_EDIT`.
12. Calling removed functions (XelpArgsInit, XelpNextTok, XelpTokN, XelpNumToks, XelpBuf2Argv) -- use argv[] directly.

---

## C++ wrapper (XelpArduino.h)

For Arduino and C++ projects, `src/XelpArduino.h` provides a `XelpCLI`
wrapper class with syntactic sugar:

```cpp
#include "XelpArduino.h"

XelpCLI cli;

void setup() {
    Serial.begin(115200);
    cli.begin("My Device", &Serial);
    cli.setCommands(commands);
}

void loop() {
    cli.poll(Serial);
}
```

Methods: `begin()`, `setCommands()`, `poll()`, `parse()`,
`r0()`-`r3()`, `help()`.

---

## Testing

### Test framework

xelp uses **JumpBug**, a minimal C89-compatible unit test framework with
no external dependencies. Tests are in `tests/xelp_unit_tests.c`.

**47 test units, 598 test cases, 100% line coverage of xelp.c.**

### Running tests

```bash
make validate     # tests + build all examples (the everyday check)
make tests        # unit tests + gcov coverage
make coverage     # coverage summary
make fuzz         # libFuzzer fuzz testing (requires clang)
```

### Writing a new test

```c
XELPRESULT test_my_feature(JumpBug *jb) {
    XELP cli;
    char out_buf[256];
    int out_pos = 0;

    /* capture output */
    void capture(char c) { out_buf[out_pos++] = c; }

    XelpInit(&cli, "test");
    XELP_SET_FN_OUT(cli, &capture);
    XELP_SET_FN_CLI(cli, test_commands);

    /* ... exercise the feature ... */

    JB_ASSERT(jb, out_pos > 0, "should produce output");
    JB_ASSERT_EQ(jb, XELP_R0(cli), XELP_S_OK, "should succeed");

    return JB_PASS;
}
```

Register in test runner:

```c
JumpBug_RunUnit(&jb, &test_my_feature, "my feature");
```

### Fuzz testing

Harnesses in `tests/fuzz/`:
- `fuzz_parse.c` -- fuzzes `XelpParse` (script buffer)
- `fuzz_parsekey.c` -- fuzzes `XelpParseKey` (byte-by-byte input)
Seed corpora in `tests/fuzz/corpus_parse/`.

---

## Build system

### Makefile targets

```bash
make validate       # tests + examples + zero warnings (everyday check)
make tests          # unit tests + coverage
make examples       # build all examples
make example        # build and run posix ncurses demo (interactive)
make coverage       # coverage summary
make fuzz           # libFuzzer fuzz testing
make funcsizes      # per-function compiled sizes
make sizes          # feature profile sizes
make version        # extract version as YAML
make prerelease     # validate + Docker cross-compile + size table update
make clean          # remove test artifacts
make clean-all      # remove all artifacts including examples
```

### CMake

```cmake
# As ESP-IDF component (auto-detected)
idf_component_register(SRCS "src/xelp.c" INCLUDE_DIRS "src")

# As plain CMake library
add_library(xelp src/xelp.c)
target_include_directories(xelp PUBLIC src)
```

### Package manifests

- `library.json` -- PlatformIO
- `library.properties` -- Arduino Library Manager
- `idf_component.yml` -- ESP-IDF Component Registry

---

## Platform-specific notes

### 8051 (SDCC)

SDCC for 8051 requires `__reentrant` on function pointers that may be
called reentrantly. xelp handles this with the `REENTRANT_SDCC` macro.
No user action needed.

### AVR / Arduino

Flash-resident strings (`PROGMEM`) are not directly supported by xelp's
string functions. For ROM strings, use standard AVR pgm_read techniques
to copy to RAM before passing to xelp.

### ARM bare metal

ARM Thumb (`-mthumb`) produces the smallest code. xelp's primary size
baseline is ARM Cortex-M0 Thumb with `-Os`.

### MSP430

16-bit `int` is fine -- xelp uses `int` intentionally for portability.
The `XELPREG` typedef can be overridden to `long` if wider registers
are needed.

### ESP32 / ESP-IDF

Install as component: `idf.py add-dependency "deftio/xelp"`. xelp
auto-registers via `CMakeLists.txt` when placed in `components/`.

---

## File structure

```
src/xelp.c         -- implementation (~1141 lines)
src/xelp.h         -- public API (types, macros, function declarations)
src/xelpcfg.h      -- compile-time feature flags and settings
src/XelpArduino.h   -- C++ wrapper class for Arduino
```

Add these files to your build. No other dependencies.

### Repository layout

```
xelp/
  src/              xelp.c, xelp.h, xelpcfg.h, XelpArduino.h
  tests/            unit tests (JumpBug framework), fuzz harnesses
  examples/         13 examples across POSIX, Arduino, ESP32, Pico, bare-metal
  docs/             API reference, tutorial, configuration, porting, testing guides
  pages/            GitHub Pages website (HTML)
  tools/            cross-build scripts, release pipeline, banner generator
  dev/              design notes, size profiling
  .github/          CI workflows (build, test, fuzz, PlatformIO)
```

### Examples directory

| Directory | Platform | Description |
|-----------|----------|-------------|
| `bare-metal/` | Any MCU | Minimal porting template |
| `multi-instance/` | Any MCU | Two independent CLIs on separate UARTs |
| `posix-simple/` | Linux/macOS | Full interactive CLI with ncurses |
| `posix-simple-cpp/` | Linux/macOS | Same as above with C++ wrapper |
| `scripting/` | Linux/macOS | Script execution vs interactive mode |
| `posix-argv/` | Linux/macOS | Native argc/argv with XelpArgvInt/XelpArgvStr |
| `arduino/` | Arduino | Raw C API with LED control |
| `arduino-cpp/` | Arduino | C++ `XelpCLI` wrapper |
| `arduino-live-cli/` | Arduino | Full hardware CLI: GPIO, ADC, PWM, tone, pulse, pin scan |
| `esp32-wifi/` | ESP32 | WiFi config, time/weather fetch |
| `esp32c6-wifi/` | ESP32-C6 | Dual-instance Serial + BLE with WiFi commands, Web Bluetooth terminal |
| `esp32-ble-cli/` | ESP32 (any with BLE) | Dual-instance Serial + BLE CLI, Web Bluetooth terminal, cross-instance messaging |
| `pico-cli/` | Raspberry Pi Pico | Pure C (GPIO, ADC, PWM) |
| `pico-cli-arduino/` | Raspberry Pi Pico | Arduino-Pico core + C++ wrapper |

---

## Compiled sizes (ARM Thumb -Os, representative)

| Configuration | .text (bytes) |
|--------------|-------------:|
| KEY only | ~580 |
| CLI only | ~1508 |
| CLI + line edit | ~1952 |
| CLI + line edit + help + key | ~2466 |
| Full (all features) | ~2506 |
| Full + history | ~2922 |

Full size tables for 20+ architectures in [README.md](README.md).

---

## Quick reference: all public functions

| Function | Purpose |
|----------|---------|
| `XelpInit(ths, aboutMsg)` | Initialize instance |
| `XelpParseKey(ths, char)` | Feed one interactive character |
| `XelpParse(ths, buf, len)` | Execute script buffer |
| `XelpParseXB(ths, &xb)` | Execute script from XelpBuf |
| `XelpExecKC(ths, keycode)` | Execute single-key command directly |
| `XelpOut(ths, msg, maxlen)` | Output string |
| `XelpPutc(ths, char)` | Output single character |
| `XelpHelp(ths)` | Print help listing |
| `XelpTokLineXB(&buf, &tok, type)` | Tokenize next token or line |
| `XelpArgvInt(argv, argc, n, &val)` | Get argv[n] as int |
| `XelpArgvStr(argv, argc, n, &s, &slen)` | Get argv[n] as string + length |
| `XelpStrLen(s)` | String length |
| `XelpStrEq(buf, len, cmd)` | Compare buffer to string |
| `XelpStrEq2(buf, end, cmd)` | Compare pointer-pair to string |
| `XelpStr2Int(s, maxlen)` | Parse string to int |
| `XelpParseNum(s, maxlen, &n)` | Safer parse with result code |
| `XelpBufCmp(as, ae, bs, be, type)` | Compare two buffers |
| `XelpFindTok(&xb, t0s, t0e, type)` | Find matching token |

---

## License

BSD 2-Clause. See [LICENSE.txt](LICENSE.txt).

Copyright (c) 2011-2026, M. A. Chatterjee.

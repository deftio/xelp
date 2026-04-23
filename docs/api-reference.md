# API Reference

All public types, functions, and macros defined in `xelp.h`. Version 0.3.1.

## Types

| Type | Description |
|------|-------------|
| `XELP` | Runtime instance of the interpreter. Holds all state. |
| `XELPRESULT` | Return type (`int`). 0 = OK, negative = error, positive = warning. |
| `XELPREG` | Register type (default `int`, overridable in xelpcfg.h). |
| `XELPKEYCODE` | Key code type (`unsigned long`). Single-char keys are their natural value; multi-byte ANSI sequences are packed little-endian. |
| `XelpBuf` | Buffer wrapper with start, position, and end pointers. |
| `XELPKeyFuncMapEntry` | Single-key command entry: `XELPRESULT fn(XELP *ths, XELPKEYCODE key)`, key code, help string. |
| `XELPCLIFuncMapEntry` | CLI command entry: `XELPRESULT fn(XELP *ths, const char *args, int len)`, command name, help string. |

## Return Codes

| Constant | Value | Meaning |
|----------|-------|---------|
| `XELP_S_OK` | 0 | Success |
| `XELP_W_WARN` | 1 | Warning (still success) |
| `XELP_S_NOTFOUND` | 2 | Token or command not found |
| `XELP_E_ERR` | -1 | General error |
| `XELP_E_CMDBUFFULL` | -2 | Command buffer full |
| `XELP_E_CMDNOTFOUND` | -3 | Command not found during dispatch |

`XELP_T_OK(r)` -- macro that returns true if `r >= 0` (OK or warning).

## Initialization

### `XELPInit`

```c
XELPRESULT XELPInit(XELP *ths, const char *pAboutMsg);
```

Initialize an XELP instance. Must be called before any other function.
`pAboutMsg` is displayed at the start of help output.

## Setup Macros

| Macro | Purpose |
|-------|---------|
| `XELP_SET_FN_OUT(ths, fn)` | Set output function: `void fn(char)` |
| `XELP_SET_FN_ERR(ths, fn)` | Set error output function |
| `XELP_SET_FN_BKSP(ths, fn)` | Set backspace handler: `void fn(void)` |
| `XELP_SET_FN_THR(ths, fn)` | Set pass-through function |
| `XELP_SET_FN_EMCHG(ths, fn)` | Set mode-change callback: `void fn(int)` |
| `XELP_SET_FN_CLI(ths, tbl)` | Set CLI command table |
| `XELP_SET_FN_KEY(ths, tbl)` | Set KEY command table |
| `XELP_SET_FN_DEF_CLI(ths, fn)` | Set default CLI handler: called when no command matches |
| `XELP_SET_FN_DEF_KEY(ths, fn)` | Set default KEY handler: called when no key matches |
| `XELP_SET_VAL_CLI_PROMPT(ths, str)` | Set CLI prompt string (stored by pointer, must be null-terminated and remain valid) |
| `XELP_SET_ABOUT(ths, str)` | Change the about/help message (stored by pointer, must be null-terminated and remain valid) |

## Core Functions

### `XELPParseKey`

```c
XELPRESULT XELPParseKey(XELP *ths, char key);
```

Feed a single character from the input stream. This is the main entry point
for interactive use. Call this for every character received from UART,
keyboard, BLE, etc.

### `XELPParse`

```c
XELPRESULT XELPParse(XELP *ths, const char *buf, int blen);
```

Parse and execute a command string. Used for scripting -- pass a complete
command or multi-statement script.

### `XELPParseXB`

```c
XELPRESULT XELPParseXB(XELP *ths, XelpBuf *script);
```

Same as `XELPParse` but takes a `XelpBuf`.

### `XELPExecKC`

```c
XELPRESULT XELPExecKC(XELP *ths, XELPKEYCODE key);
```

Execute a single-key command directly (bypasses mode checking).

### `XELPOut`

```c
XELPRESULT XELPOut(XELP *ths, const char *msg, int maxlen);
```

Output a string through the instance's output function. If `maxlen > 0`,
prints at most that many characters. If `maxlen <= 0`, prints until null
terminator.

### `XELPHelp`

```c
XELPRESULT XELPHelp(XELP *ths);
```

Print help listing all registered KEY and CLI commands. Only available when
`XELP_ENABLE_HELP` is defined.

## Tokenizer

### `XELPTokLineXB`

```c
XELPRESULT XELPTokLineXB(XelpBuf *buf, XelpBuf *tok, int srchType);
```

Get the next token or line from a buffer. `srchType` is `XELP_TOK_ONLY`
(token) or `XELP_TOK_LINE` (full line).

### `XELPTokN`

```c
XELPRESULT XELPTokN(XelpBuf *buf, int n, XelpBuf *tok);
```

Get the Nth token (0-indexed) from a buffer.

### `XELPNumToks`

```c
XELPRESULT XELPNumToks(XelpBuf *buf, int *n);
```

Count the number of tokens in a buffer.

## XelpArgs -- Sequential Argument Iterator

A left-to-right token iterator for CLI command handlers. Preferred over
`XELPTokN` when arguments are processed sequentially (O(1) per token
instead of O(n) re-scan).

### `XelpArgsInit`

```c
XELPRESULT XelpArgsInit(XelpArgs *a, const char *args, int len);
```

Initialize an argument iterator. `args` and `len` come directly from
the CLI command callback parameters.

### `XelpNextTok`

```c
XELPRESULT XelpNextTok(XelpArgs *a, XelpBuf *tok);
```

Get the next token. On success, `tok->s` points to the first character
and `tok->p` points one past the last. Pass `NULL` for `tok` to skip a
token (useful for skipping the command name).

Returns `XELP_S_OK` on success, non-OK when no more tokens remain.

### `XelpNextInt`

```c
XELPRESULT XelpNextInt(XelpArgs *a, int *val);
```

Get the next token and parse it as an integer (decimal or hex).

### `XelpArgCount`

```c
XELPRESULT XelpArgCount(XelpArgs *a, int *n);
```

Count the total number of tokens without consuming them. The iteration
position is preserved.

### Example

```c
XELPRESULT cmd_divmod(XELP *ths, const char *args, int len) {
    XelpArgs a;
    int dividend, divisor;
    XelpArgsInit(&a, args, len);
    XelpNextTok(&a, 0);              /* skip command name */
    XelpNextInt(&a, &dividend);
    XelpNextInt(&a, &divisor);
    if (divisor == 0) return XELP_E_ERR;
    ths->mR[1] = dividend / divisor;
    ths->mR[2] = dividend % divisor;
    return XELP_S_OK;
}
```

## String Utilities

### `XELPStrLen`

```c
int XELPStrLen(const char *c);
```

Compute length of a null-terminated string. No stdlib dependency.

### `XELPStrEq`

```c
XELPRESULT XELPStrEq(const char *pbuf, int blen, const char *cmd);
```

Compare a buffer of length `blen` against a null-terminated string `cmd`.
Returns `XELP_S_OK` if equal.

### `XELPStr2Int`

```c
int XELPStr2Int(const char *s, int maxlen);
```

Parse a string to integer. Accepts decimal and hex (`FFh` suffix or `0xFF`
prefix). Supports uppercase and lowercase hex digits.

### `XELPParseNum`

```c
XELPRESULT XELPParseNum(const char *s, int maxlen, int *n);
```

Safer string-to-integer: returns a result code and writes the parsed value
to `*n`.

### `XELPBufCmp`

```c
XELPRESULT XELPBufCmp(const char *as, const char *ae,
                      const char *bs, const char *be, int cmpType);
```

Compare two buffers. `cmpType` controls null-terminator handling:

- `XELP_CMP_TYPE_BUF` (0x00) -- pure byte comparison by length
- `XELP_CMP_TYPE_A0` (0x01) -- buffer A also terminates on `\0`
- `XELP_CMP_TYPE_A0B0` (0x11) -- both buffers terminate on `\0`

## XelpBuf Macros

| Macro | Purpose |
|-------|---------|
| `XELP_XB_INIT(xb, buf, len)` | Initialize from pointer + length |
| `XELP_XB_INIT_PTRS(xb, s, p, e)` | Initialize from three pointers |
| `XELP_XB_INIT_BP(xb, buf, pos, len)` | Initialize with cursor position |
| `XELP_XB_COPY(a, b)` | Copy XelpBuf a to b |
| `XELP_XB_LEN(xb)` | Total buffer length |
| `XELP_XB_POS(xb)` | Current position as int offset |
| `XELP_XB_PUTC(xb, ch)` | Write char with bounds check |
| `XELP_XB_GETC(xb, ch)` | Read char and advance position |
| `XELP_XB_TOP(xb)` | Reset position to start |
| `XELP_XB_OUT(x, xb)` | Print XelpBuf contents from current position |

## Registers

Each XELP instance contains an array of `XELPREG` (default `int`) registers
used to pass return values between the engine and command handlers.

- **`XELP_REGS_SZ`** -- register count (default 4, minimum 4).
- **`XELPREG`** -- register type (default `int`, overridable in xelpcfg.h).
- **Convention: callee-clobbers-all** -- any command call may overwrite all
  registers. Do not rely on values persisting across calls.

| Macro | Description |
|-------|-------------|
| `XELP_R0(ths)` | Command status (written by engine after dispatch). Read-only by convention. |
| `XELP_R1(ths)` | Command-specific return value 1 (engine never touches). |
| `XELP_R2(ths)` | Command-specific return value 2 (engine never touches). |
| `XELP_R3(ths)` | Command-specific return value 3 (engine never touches). |

`ths` is a struct (not a pointer). For pointer access inside command
handlers, use `ths->mR[1]` directly.

### Example: command returning multiple values

```c
XELPRESULT cmd_divmod(XELP *ths, const char *args, int len) {
    int a = 17, b = 5;
    ths->mR[1] = a / b;  /* quotient  -> R1 */
    ths->mR[2] = a % b;  /* remainder -> R2 */
    return XELP_S_OK;    /* engine writes R0 */
}

/* caller reads results via macros */
XELP_R1(cli)  /* 3 */
XELP_R2(cli)  /* 2 */
```

## Multi-byte Key Codes

`XELPKEYCODE` constants for ANSI escape sequences. Packed little-endian:
byte[0] in bits 0-7, byte[1] in 8-15, etc. Single-char keys are their
natural value (e.g. `'a'` == 0x61). Multi-byte keys are >= 0x100.

| Constant | Value | Sequence |
|----------|-------|----------|
| `XELP_KEYCODE_UP` | `0x00415B1BUL` | ESC [ A |
| `XELP_KEYCODE_DOWN` | `0x00425B1BUL` | ESC [ B |
| `XELP_KEYCODE_RIGHT` | `0x00435B1BUL` | ESC [ C |
| `XELP_KEYCODE_LEFT` | `0x00445B1BUL` | ESC [ D |
| `XELP_KEYCODE_HOME` | `0x00485B1BUL` | ESC [ H |
| `XELP_KEYCODE_END` | `0x00465B1BUL` | ESC [ F |
| `XELP_KEYCODE_INS` | `0x7E325B1BUL` | ESC [ 2 ~ |
| `XELP_KEYCODE_KDEL` | `0x7E335B1BUL` | ESC [ 3 ~ |
| `XELP_KEYCODE_PGUP` | `0x7E355B1BUL` | ESC [ 5 ~ |
| `XELP_KEYCODE_PGDN` | `0x7E365B1BUL` | ESC [ 6 ~ |

### Key code accessor macros

| Macro | Description |
|-------|-------------|
| `XELP_KC_B0(k)` | Extract byte 0 (bits 0-7) |
| `XELP_KC_B1(k)` | Extract byte 1 (bits 8-15) |
| `XELP_KC_B2(k)` | Extract byte 2 (bits 16-23) |
| `XELP_KC_B3(k)` | Extract byte 3 (bits 24-31) |
| `XELP_KC_IS_MULTI(k)` | True if key code is multi-byte (>= 0x100) |

## Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `XELP_VERSION` | 0x00000301 | Library version (32-bit hex: `0x00MMmmpp`) |
| `XELP_VER_MAJOR(v)` | | Extract major version byte |
| `XELP_VER_MINOR(v)` | | Extract minor version byte |
| `XELP_VER_PATCH(v)` | | Extract patch version byte |
| `XELP_CMDBUFSZ` | 64 | Default command buffer size |
| `XELP_MODE_CLI` | 0x00 | CLI mode identifier |
| `XELP_MODE_KEY` | 0x01 | KEY mode identifier |
| `XELP_MODE_THR` | 0x02 | THRU mode identifier |
| `XELP_ECHO_NORMAL` | `'\0'` | Echo typed character as-is (default) |
| `XELP_ECHO_OFF` | `'\1'` | Suppress character echo entirely |

## Output Control

Two independent mechanisms control what an XELP instance sends to its
output function:

### Output Enable (`mOutEnable`)

Gates **all** output: `XELPOut`, help, prompt, character echo, and
redraw. Useful for silent scripting or batch mode.

| Macro | Description |
|-------|-------------|
| `XELP_SET_OUT_ENABLE(ths, val)` | Set output enable: 0 = mute, nonzero = normal |
| `XELP_GET_OUT_ENABLE(ths)` | Read current output-enable state |

Default after `XELPInit`: **1** (enabled).

### Echo Control (`mEchoChar`)

Controls how printable characters are echoed during interactive input.
Does **not** affect `XELPOut` calls from commands, ENTER newline echo,
cursor movement, or prompt output.

| Macro | Description |
|-------|-------------|
| `XELP_SET_ECHO(ths, ch)` | Set echo mode: `XELP_ECHO_NORMAL`, `XELP_ECHO_OFF`, or a mask character |
| `XELP_GET_ECHO(ths)` | Read current echo character |

Values:
- `XELP_ECHO_NORMAL` (`'\0'`) -- echo the typed character (default)
- `XELP_ECHO_OFF` (`'\1'`) -- suppress echo entirely
- Any other character (e.g. `'*'`) -- echo that character instead

Default after `XELPInit`: `XELP_ECHO_NORMAL`.

### Password Entry Example

```c
XELPRESULT cmd_pass(XELP *ths, const char *args, int len) {
    XELP_SET_ECHO(*ths, '*');       /* mask input */
    /* ... user types password, sees ****** ... */
    /* on ENTER, read buffer, then: */
    XELP_SET_ECHO(*ths, XELP_ECHO_NORMAL);  /* restore */
    return XELP_S_OK;
}
```

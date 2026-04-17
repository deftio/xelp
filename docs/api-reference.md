# API Reference

All public types, functions, and macros defined in `xelp.h`. Version 0.2.1.

## Types

| Type | Description |
|------|-------------|
| `XELP` | Runtime instance of the interpreter. Holds all state. |
| `XELPRESULT` | Return type (`int`). 0 = OK, negative = error, positive = warning. |
| `XELPREG` | Register type (default `int`, overridable in xelpcfg.h). |
| `XelpBuf` | Buffer wrapper with start, position, and end pointers. |
| `XELPKeyFuncMapEntry` | Single-key command entry: function pointer, key, help string. |
| `XELPCLIFuncMapEntry` | CLI command entry: function pointer, command name, help string. |

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
| `XELP_SET_VAL_CLI_PROMPT(ths, str)` | Set CLI prompt string |
| `XELP_SET_ABOUT(ths, str)` | Change the about/help message |

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
XELPRESULT XELPExecKC(XELP *ths, char key);
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
| `XELP_XBInit(xb, buf, len)` | Initialize from pointer + length |
| `XELP_XBInitPtrs(xb, s, p, e)` | Initialize from three pointers |
| `XELP_XBInitBP(xb, buf, pos, len)` | Initialize with cursor position |
| `XELP_XBPCopy(a, b)` | Copy XelpBuf a to b |
| `XELP_XBBufLen(xb)` | Total buffer length |
| `XELP_XBGetPos(xb)` | Current position as int offset |
| `XELP_XBPUTC(xb, ch)` | Write char with bounds check |
| `XELP_XBGETC(xb, ch)` | Read char and advance position |
| `XELP_XBTOP(xb)` | Reset position to start |
| `XELPOutXB(x, xb)` | Print XelpBuf contents from current position |

## Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `XELP_VERSION` | 0x00000201 | Library version (32-bit hex: `0x00MMmmpp`) |
| `XELP_VER_MAJOR(v)` | | Extract major version byte |
| `XELP_VER_MINOR(v)` | | Extract minor version byte |
| `XELP_VER_PATCH(v)` | | Extract patch version byte |
| `XELP_CMDBUFSZ` | 64 | Default command buffer size |
| `XELP_MODE_CLI` | 0x00 | CLI mode identifier |
| `XELP_MODE_KEY` | 0x01 | KEY mode identifier |
| `XELP_MODE_THR` | 0x02 | THRU mode identifier |

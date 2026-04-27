# Argument Parsing Ergonomics for xelp

Design notes for improving how command handlers access their arguments.
Goal: reduce per-command boilerplate while keeping zero-malloc, C89, and
small code size.

## The Problem

Every command handler that takes arguments currently looks like this:

```c
XELPRESULT cmd_led(XELP *ths, const char *args, int len) {
    XelpBuf b, tok;
    int val;
    XELP_XB_INIT(b, (char*)args, len);
    if (XelpTokN(&b, 1, &tok) == XELP_S_OK) {
        XelpParseNum(tok.s, (int)(tok.p - tok.s), &val);
        /* finally do something with val */
    }
    return XELP_S_OK;
}
```

That's 4 lines of parsing machinery for one integer argument. The XelpArgs
iterator (added in 0.3.1) is better but still verbose:

```c
XELPRESULT cmd_led(XELP *ths, const char *args, int len) {
    XelpArgs a;
    int val;
    XelpArgsInit(&a, args, len);
    XelpNextTok(&a, NULL);   /* skip command name */
    XelpNextInt(&a, &val);
    /* finally do something with val */
    return XELP_S_OK;
}
```

Compare with what the JS/Python world expects:

```js
cli.addCommand("led", (args) => { led(parseInt(args[1])); });
```

We can't match that in C89, but we can get closer.

## What Other Embedded CLI Libraries Do

### funbiscuit/embedded-cli (~2KB code, 1.5KB RAM)

Handler receives `(EmbeddedCli *cli, char *args, void *context)`.
Arguments pre-tokenized if flag set during registration. Access via:

```c
const char *arg = embeddedCliGetToken(args, 1);   /* 1-indexed */
uint8_t count = embeddedCliGetTokenCount(args);
```

**Trade-off**: Modifies the input buffer in-place (inserts nulls between
tokens). Tokens are null-terminated C strings. Simple to use, but
destructive -- can't re-parse or use const input. xelp deliberately
avoids this (scripts are const/ROM-able).

### Helius/microrl

Handler receives `(int argc, char **argv)` -- classic main() style.
Library tokenizes into a pre-allocated argv array.

**Trade-off**: Requires a fixed-size `char *argv[N]` array. Each token
is null-terminated (destructive). Simple and familiar, but the argv
array costs RAM (N * sizeof(char*)) and limits max arguments.

### AndreRenaud/EmbeddedCLI (~1KB code, 200B RAM minimal)

Also parses into argc/argv. Supports quoted strings and escapes.
Suggests pairing with a separate "Simple Options" library for
`-flag value` style parsing.

### MicroShell (marcinbor85)

Filesystem-like command tree (ls, cat, pwd). Not really comparable --
different problem domain. No dynamic allocation, callback-based.

### Summary: Industry Patterns

| Library | Arg Interface | Destructive? | Null-terminated? | Extra RAM |
|---------|-------------|-------------|-----------------|-----------|
| embedded-cli | getToken(args, N) | Yes | Yes | N/A (in-place) |
| microrl | argc/argv | Yes | Yes | argv array |
| EmbeddedCLI | argc/argv | Yes | Yes | argv array |
| xelp (current) | XelpTokN / XelpArgs | No | No | XelpBuf on stack |

**Key observation**: Every other library modifies the input buffer to
null-terminate tokens. xelp is the only one that preserves const input
(needed for ROM-able scripts). This is a genuine differentiator but
it costs ergonomics -- tokens come as (pointer, length) pairs instead
of C strings.

## Options Evaluated

### Option A: Direct-access convenience functions (CHOSEN)

Add `XelpArgInt` and `XelpArgStr` as functions (not macros) that
combine "get Nth argument" into one call. No new types, no new
concepts -- just fewer lines per command.

Functions are the right choice over macros: the linker includes each
function body once regardless of how many commands call it. A macro
would expand the full XelpTokN + XelpParseNum sequence at every call
site -- ~50 bytes per invocation instead of once. Five commands using
XelpArgInt as a function: ~50 bytes total. As a macro: ~250 bytes.

```c
/* Get argument N as an integer.  Arg 0 is the command name. */
XELPRESULT XelpArgInt(const char *args, int len, int n, int *val);

/* Get argument N as a string span.  Sets *s and *slen. */
XELPRESULT XelpArgStr(const char *args, int len, int n,
                      const char **s, int *slen);
```

**Pros**:
- Dead simple, self-documenting
- No new types or state
- Works with existing const/non-destructive parsing
- Function body included once by linker, called from many sites

**Cons**:
- O(N) per call (re-scans from start each time). Fine for commands
  with 1-3 arguments. Bad if someone calls it in a loop for 20 args.
- Doesn't cover the "iterate all args" case (use existing XelpArgs)

### Option B: Enhanced XelpArgs iterator (NOT CHOSEN)

Would have added `XelpArgsBegin` (auto-skip command name) and typed
accessors (`XelpArgsInt`, `XelpArgsStr`).

**Rejected** because:
- Auto-skipping arg 0 violates the argc/argv convention. Arg 0 is the
  command name in every C program and every other CLI library. A command
  registered under two names (`"help"` and `"?"`) may need to know which
  name invoked it. Silently skipping it would be surprising.
- The existing XelpArgs iterator (XelpArgsInit + XelpNextTok +
  XelpNextInt) already covers the stateful iteration case adequately.
- Adding more functions to the iterator increases API surface for
  marginal benefit.

### Option C: argc/argv with pre-allocated array (NOT CHOSEN)

Tokenize into a fixed-size XelpBuf array.

**Rejected** because:
- Stack cost per call (N * 12 bytes on 32-bit)
- Tokens are still (ptr, len), not null-terminated C strings --
  so you can't pass them to printf("%s") or strcmp() directly.
  This reduces the ergonomic win vs. other libraries.
- XELP_MAX_ARGS is a footgun (silent truncation if exceeded)

### Option D: Destructive argc/argv (NOT CHOSEN)

Insert null bytes into the args buffer to give real C strings.

**Rejected** because:
- **Breaks xelp's const-input guarantee.** Scripts can't live in ROM.
  This is a fundamental design principle of xelp.
- Requires user to copy input to a mutable buffer first

## Final Proposal

Two new functions added to the base CLI API. No new compile flag --
compiled whenever `XELP_ENABLE_CLI` is on (these are useless without
the CLI tokenizer anyway). Implemented as functions, not macros.

```c
XELPRESULT XelpArgInt(const char *args, int len, int n, int *val);
XELPRESULT XelpArgStr(const char *args, int len, int n,
                      const char **s, int *slen);
```

Arg 0 is the command name, arg 1 is the first real argument. Follows
the argc/argv convention exactly.

Internally: thin wrappers over existing `XelpTokN` + `XelpParseNum`.
No new types, no new state, no behavior changes to existing functions.

### Code Size

| Function | Est. ARM Thumb | Notes |
|----------|---------------|-------|
| `XelpArgInt` | ~50 bytes | XelpTokN + XelpParseNum wrapper |
| `XelpArgStr` | ~40 bytes | XelpTokN wrapper, returns span |
| **Total** | **~90 bytes** | Part of base CLI, no extra flag |

For context, the full CLI build is ~2500 bytes on ARM Thumb. This adds
~3.5%. Two functions that improve every command handler in the project.

## Before/After Comparison

### Simple command (1 int arg)

**Before** (4 lines of boilerplate):
```c
XELPRESULT cmd_led(XELP *ths, const char *args, int len) {
    XelpBuf b, tok;
    int val;
    XELP_XB_INIT(b, (char*)args, len);
    if (XelpTokN(&b, 1, &tok) == XELP_S_OK) {
        XelpParseNum(tok.s, (int)(tok.p - tok.s), &val);
        set_led(val);
    }
    return XELP_S_OK;
}
```

**After** (1 line):
```c
XELPRESULT cmd_led(XELP *ths, const char *args, int len) {
    int val;
    if (XelpArgInt(args, len, 1, &val) == XELP_S_OK)
        set_led(val);
    return XELP_S_OK;
}
```

### Multi-arg command (2 ints)

**Before**:
```c
XELPRESULT cmd_divmod(XELP *ths, const char *args, int len) {
    XelpBuf b, tok;
    int a, d;
    XELP_XB_INIT(b, (char*)args, len);
    if (XelpTokN(&b, 1, &tok) != XELP_S_OK) goto usage;
    a = XelpStr2Int(tok.s, (int)(tok.p - tok.s));
    XELP_XB_TOP(b);
    if (XelpTokN(&b, 2, &tok) != XELP_S_OK) goto usage;
    d = XelpStr2Int(tok.s, (int)(tok.p - tok.s));
    /* ... */
}
```

**After**:
```c
XELPRESULT cmd_divmod(XELP *ths, const char *args, int len) {
    int a, d;
    if (XelpArgInt(args, len, 1, &a) != XELP_S_OK) goto usage;
    if (XelpArgInt(args, len, 2, &d) != XELP_S_OK) goto usage;
    /* ... */
}
```

### String argument

**Before**:
```c
XELPRESULT cmd_ssid(XELP *ths, const char *args, int len) {
    XelpBuf b, tok;
    XELP_XB_INIT(b, (char*)args, len);
    if (XelpTokN(&b, 1, &tok) == XELP_S_OK) {
        int slen = (int)(tok.p - tok.s);
        memcpy(gSsid, tok.s, slen);
        gSsid[slen] = '\0';
    }
    return XELP_S_OK;
}
```

**After**:
```c
XELPRESULT cmd_ssid(XELP *ths, const char *args, int len) {
    const char *s; int slen;
    if (XelpArgStr(args, len, 1, &s, &slen) == XELP_S_OK) {
        memcpy(gSsid, s, slen);
        gSsid[slen] = '\0';
    }
    return XELP_S_OK;
}
```

## Design Decisions

1. **Arg 0 is the command name.** Follows argc/argv convention. The
   handler picks which index it wants. No implicit skipping.

2. **Functions, not macros.** Linker includes the body once. Macros
   would duplicate ~50 bytes of XelpTokN + XelpParseNum at every call
   site. For something called from every command handler, this matters.

3. **No new compile flag.** These live in the base CLI API, gated only
   by `XELP_ENABLE_CLI`. Users who enable CLI want arg parsing. Users
   on KEY-only don't have arguments to parse.

4. **No changes to existing API.** XelpArgs, XelpTokN, XelpNextTok,
   XelpNextInt all stay as-is. The new functions are additive.

5. **Hex auto-detection.** XelpArgInt calls XelpParseNum internally,
   which already handles `0x1A` and `1Ah` formats. No decision needed.

6. **Error semantics.** If arg N doesn't exist, return XELP_E_ERR and
   leave *val unchanged. Caller can set a default before calling.
   Matches existing XelpParseNum behavior.

7. **O(N) is acceptable.** XelpArgInt re-scans from the start each
   call. For commands with 1-3 args (the vast majority), this is
   negligible. Commands with many args should use the XelpArgs
   iterator, which is O(1) per token.

## Existing API (unchanged)

For reference, the existing argument APIs remain available:

```c
/* Random access (O(N) per call) */
XELPRESULT XelpTokN(XelpBuf *buf, int n, XelpBuf *tok);
XELPRESULT XelpNumToks(XelpBuf *b, int *n);

/* Sequential iterator (O(1) per call) */
XELPRESULT XelpArgsInit(XelpArgs *a, const char *args, int len);
XELPRESULT XelpNextTok(XelpArgs *a, XelpBuf *tok);
XELPRESULT XelpNextInt(XelpArgs *a, int *val);
XELPRESULT XelpArgCount(XelpArgs *a, int *n);

/* Low-level */
int        XelpStr2Int(const char *s, int maxlen);
XELPRESULT XelpParseNum(const char *s, int maxlen, int *n);
```

The new `XelpArgInt` and `XelpArgStr` are sugar over these for the
common case. Power users retain full access to the tokenizer.

## Implementation Priority

Developer-ergonomics improvement, not a functional change. Ship when
convenient -- fully backward-compatible, no API breaks. Good candidate
for the next release (0.3.2 or 0.4.0).

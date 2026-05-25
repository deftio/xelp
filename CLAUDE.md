# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

xelp is a tiny extensible command interpreter and script engine for embedded systems. Pure C89, no malloc, no OS, no stdlib. Three source files: `src/xelp.c`, `src/xelp.h`, `src/xelpcfg.h`. Targets 8-bit (ATtiny85, 8051) through 64-bit (ARM64, x86-64). Under 3-5 KB fully featured on 32-bit targets.

## Build Commands

```bash
make tests          # Build + run unit tests with gcov coverage report
make validate       # Lint + tests + coverage gate (100% line, 97% branch) + build examples
make coverage       # Tests + coverage summary
make lint           # cppcheck static analysis
make fuzz           # All fuzz harnesses (requires clang + libFuzzer)
make fuzz-script    # Script engine fuzz only
make examples       # Build all POSIX examples (no interactive launch)
make clean          # Remove build artifacts
```

Coverage gate: `make validate` fails if line coverage < 100% or branch coverage < 97%.

Fuzz testing uses Homebrew LLVM by default (`/usr/local/opt/llvm/bin/clang`). Override with `FUZZ_CC=` and `FUZZ_SAN=`.

## Critical Constraints

- **No malloc.** Never use `malloc`, `calloc`, `strdup`, or any dynamic allocation.
- **No stdlib string functions.** Use xelp's own: `XelpStrLen`, `XelpStrEq`, `XelpStr2Int`, `XelpParseNum`, `XelpBufCmp`. Not `strlen`, `strcmp`, `atoi`, `sprintf`.
- **C89/C90 in `src/`.** No `//` comments, no mixed declarations and code, no VLAs, no GCC extensions.
- **No globals.** All state goes through the `XELP *ths` instance pointer. Multiple instances must work independently.
- **Compile with `-Wall -Wextra -Werror`.** Zero warnings allowed.
- **Don't write to /tmp — use ./tmp/.**

## Architecture

All code is in a single `src/xelp.c` file, feature-gated with `#ifdef` blocks:

- `XELP_ENABLE_KEY` — single-keypress dispatch mode
- `XELP_ENABLE_CLI` — line-buffered CLI with tokenizer and command dispatch
- `XELP_ENABLE_LINE_EDIT` — cursor movement, insert, delete (requires CLI)
- `XELP_ENABLE_HISTORY` — UP/DOWN command recall (requires LINE_EDIT)
- `XELP_ENABLE_THR` — pass-through mode
- `XELP_ENABLE_HELP` — built-in help listing
- `XELP_ENABLE_SCRIPT` — script engine with variables, control flow, functions (requires CLI)

Feature dependencies auto-disable at the bottom of `xelpcfg.h` if prerequisites are missing.

### Data Flow

```
XelpParseKey(ths, char)  →  key accumulator  →  mode dispatch:
  KEY mode: XelpExecKC() → key table → handler(ths, keycode)
  THR mode: mpfPassThru(char)
  CLI mode: line buffer → on ENTER → XelpParseXB() → tokenize → dispatch

XelpParse(ths, buf, len)  →  XelpParseXB()  →  same tokenizer pipeline
```

When `XELP_ENABLE_SCRIPT` is defined, CLI's `_xelpHandleEnter` routes through `_xelpEvalScript` instead of `XelpParseXB`, enabling `_set`, `_if`, `_goto`, `_func`, math builtins, etc.

### Script Engine Arena

The script engine uses a fixed-size arena (`XELP_SCRIPT_ARENA_SZ`, default 512 bytes):
- Stack (grows up from start): result entries, call frames
- Heap (grows down from end): variables (INT/STR), PROC entries (inline body)
- Overflow: `SP >= HP → XELP_E_ARENA_FULL`

### Parser State Machine (PSM)

The tokenizer is a table-driven FSM (`gPSMStates[94]`, `gPSMJumpTable[8]`) that operates read-only on input buffers. Two tokenization modes: `XELP_TOK_ONLY` (single token) and `XELP_TOK_LINE` (full line with command boundaries).

## Test Framework

Tests use "jumpbug" — a minimal unit test framework in `tests/jumpbug_unit_test_fw.c`. Test pattern:

```c
XELPRESULT test_MyFeature() {
    /* setup */
    if (JB_ASSERT(condition_that_should_be_false, "description"))
        return XELP_E_ERR;
    return XELP_S_OK;
}
```

Note: `JB_ASSERT` returns non-zero on FAILURE (when condition is TRUE). Typical idiom: `JB_ASSERT(actual != expected, "msg")`.

Tests are registered in `run_tests()` at the bottom of `xelp_unit_tests.c`:
```c
JumpBug_RunUnit(test_MyFeature, "MyFeature");
```

Script engine tests are gated with `#ifdef XELP_ENABLE_SCRIPT`.

## Key Conventions

- Command handler signature: `XELPRESULT fn(XELP *ths, int argc, const char **argv)`
- Key handler signature: `XELPRESULT fn(XELP *ths, XELPKEYCODE key)`
- Command tables terminated with `XELP_FUNC_ENTRY_LAST`
- Output via `XelpOut(ths, str, maxlen)` — maxlen 0 means null-terminated
- String lengths passed explicitly (no null-termination assumed internally)
- `XelpStrLen()` used instead of `strlen()` for length computation
- Build flags passed via `-D` on command line (e.g., `-DXELP_ENABLE_SCRIPT`)
- The test build always passes `-DXELP_ENABLE_SCRIPT` (see makefile `C_FLAGS`)

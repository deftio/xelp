# Changelog

All notable changes to xelp will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

The version source of truth is `XELP_VERSION` in `src/xelp.h` (hex format).
Versions always use three-component semver (e.g. `0.3.0`, never `0.3`).

## [Unreleased]

## [0.3.2] - 2026-04-26

### Added
- **Command history** (`XELP_ENABLE_HISTORY`): UP/DOWN arrow recall of
  previously entered commands using a fixed-slot ring buffer. Requires
  `XELP_ENABLE_CLI` and `XELP_ENABLE_LINE_EDIT`. Configurable depth via
  `XELP_HIST_DEPTH` (default 4). Consecutive duplicate suppression, empty
  command filtering, in-progress line save/restore on browse.
  RAM cost: `XELP_HIST_DEPTH * XELP_CMDBUFSZ + XELP_CMDBUFSZ + 4` bytes
  per instance. Code cost: ~420 bytes on ARM Thumb (`-Os`).
- `XelpArgInt(args, len, n, &val)` -- get argument N as an integer in one
  call. Wraps `XelpTokN` + `XelpParseNum`. ~108 bytes ARM Thumb (combined
  with XelpArgStr).
- `XelpArgStr(args, len, n, &s, &slen)` -- get argument N as a string
  span (pointer + length) in one call. Wraps `XelpTokN`.

### Fixed
- `XELPKEY_BKSP` was defined as 0x07 (BEL), not 0x08 (ASCII BS). Added
  `XELPKEY_BS` (0x08) and both backspace paths in `XelpParseKey` now
  accept 0x07, 0x08, and 0x7F (DEL).

### Changed
- Test suite expanded to 47 units, 598 test cases (from 39/531).
- `dev/size_profiles.sh` updated with profile 10 (Full + history).

## [0.3.1] - 2026-04-26

### Changed
- **Function naming convention**: all public functions renamed from
  `XELP` prefix to `Xelp` prefix (e.g. `XELPInit` -> `XelpInit`,
  `XELPOut` -> `XelpOut`). Types, macros, and constants retain the
  `XELP` prefix.
- Replaced `int mEchoState` (unused) with `char mOutEnable` +
  `char mEchoChar` in `XELP` struct (saves 2 bytes).
- Internal macros `_PUTC`, `_ECHO`, `_CURSOR` converted to static
  functions for code size reduction (~28 bytes saved).
- `_xelpKeyAccum` rewritten as table-driven switch on `mKeyLen`.
- `_xelpPrintKeyName` rewritten to table-driven approach (~219 bytes
  saved).
- `XelpParseKey` refactored: cursor movement collapsed into
  `_xelpCursorMove` helper, ENTER handling extracted to
  `_xelpHandleEnter` helper.
- Test suite expanded to 39 units, 531 test cases (from 36/477).
- **BREAKING:** KEY command function signature changed from
  `XELPRESULT fn(XELP *ths, int key)` to
  `XELPRESULT fn(XELP *ths, XELPKEYCODE key)`.
  `XELPKEYCODE` is `unsigned long`, so single-char keys still work
  unchanged, but multi-byte ANSI sequences are now dispatched as a
  single packed value.
- **BREAKING:** `XELPKeyFuncMapEntry.mKey` changed from `char` to
  `XELPKEYCODE`. Use `XELP_KEYCODE_UP`, `XELP_KEYCODE_LEFT`, etc.
  for multi-byte keys. Single-char keys (`'?'`, `'h'`, etc.) are
  unchanged.
- Default `XELP_REGS_SZ` changed from 1 to 4 (4 callee-clobbers-all
  return registers per instance). Minimum clamped to 4.
- `tools/compactbuilds-docker.sh` rewritten: builds each target in
  three configurations (KEY / CLI / FULL), grouped by word size.

### Added
- **Single-line editing** (`XELP_ENABLE_LINE_EDIT`): left/right cursor
  movement, Home/End, insert-at-cursor, Delete. Eliminates garbage
  `[A` characters when users press arrow keys. ~800-1000 bytes on ARM
  Thumb.
- **Multi-byte key accumulator** in `XelpParseKey`: recognizes ANSI
  escape sequences (arrow keys, Home/End, Delete, Insert, PgUp/PgDn)
  and dispatches them as `XELPKEYCODE` values.
- `XELPKEYCODE` type (`unsigned long`) and named key macros:
  `XELP_KEYCODE_UP`, `XELP_KEYCODE_DOWN`, `XELP_KEYCODE_LEFT`,
  `XELP_KEYCODE_RIGHT`, `XELP_KEYCODE_HOME`, `XELP_KEYCODE_END`,
  `XELP_KEYCODE_INS`, `XELP_KEYCODE_KDEL`, `XELP_KEYCODE_PGUP`,
  `XELP_KEYCODE_PGDN`.
- Key code accessor macros: `XELP_KC_B0`-`XELP_KC_B3`,
  `XELP_KC_IS_MULTI`.
- `XELP_R0`-`XELP_R3` accessor macros for register access
- `r0()`-`r3()` methods on C++ `XelpCLI` wrapper (`XelpArduino.h`)
- `divmod` example command demonstrating multi-register returns
- Register tests (9 cases covering init, dispatch, macros, survival)
- Fuzz testing: libFuzzer harnesses for `XelpParseKey` and `XelpParse`
  (`tests/fuzz/`). Seed corpora included. `make fuzz` target.
- Multi-instance stress test verifying no shared state between
  interleaved XELP instances (5 test sections, 50-round stress loop).
- PlatformIO CI job in GitHub Actions (builds for Uno, Mega, ESP32).
- Cross-build Docker tooling: added `extract_size.py` for robust
  multi-format size extraction (ELF, SDCC .rel, SDCC .map, Intel HEX).
  18 targets now produce real sizes across KEY/CLI/FULL configurations.
- **Output control**: `mOutEnable` (char) gates all output; `mEchoChar`
  (char) controls echo during interactive input (normal, off, or mask
  character). Macros: `XELP_SET_OUT_ENABLE`, `XELP_GET_OUT_ENABLE`,
  `XELP_SET_ECHO`, `XELP_GET_ECHO`. Constants: `XELP_ECHO_NORMAL`,
  `XELP_ECHO_OFF`.
- `XelpPutc` function: single-character output respecting `mOutEnable`.
- `XelpArgs` sequential argument iterator: `XelpArgsInit`,
  `XelpNextTok`, `XelpNextInt`, `XelpArgCount`. O(1) per token
  alternative to `XelpTokN`.
- `dev/size_profiles.sh`: reusable build profile size reporting tool
  using Docker cross-compilation (ARM Cortex-M0 Thumb) with host GCC
  fallback.

### Fixed
- **SEGV in XelpTokLineXB**: buffer exhaustion in `_PS_ESCA` state
  (CLI escape char at end of input) returned `XELP_S_OK` with
  uninitialized `tok->s`, causing crash in `XelpStrEq` during command
  dispatch. Found by fuzz testing.
- **KEY-only build failure**: `XELP_XB_INIT(ths->mCmdXB,...)` in
  `XelpInit` was not guarded by `#ifdef XELP_ENABLE_CLI`, preventing
  KEY-only configurations from compiling.

## [0.3.0] - 2026-04-19

### Overview

v0.3.0 is a breaking API change that passes the `XELP *` instance pointer
to every user-defined CLI and KEY command function. Previously, command
handlers had no way to know which xelp instance invoked them, forcing
multi-instance designs to use hardcoded global pointers or separate
functions per instance. Now every command receives `ths` as its first
parameter, giving it direct access to `XELPOut()`, registers (`mR[]`),
and all other instance state.

**Why now:** v0.2.6 just shipped and the Arduino Library Manager hasn't
indexed it yet, making this the ideal window for a breaking change before
the user base grows further. This change is also a prerequisite for the
planned scripting extensions (subroutine calls, control flow) which
require commands to interact with interpreter state.

**Migration:** add `XELP *ths` as the first parameter to every CLI
command function and every KEY handler function. Replace any hardcoded
`&myGlobalInstance` pointers with `ths`. Functions that don't need
instance access can simply `(void)ths;`.

**Code size impact:** ~3-5% increase across all targets (48-189 bytes
depending on architecture) due to the extra pointer argument at 4
internal dispatch call sites. The `XELP` struct size is unchanged.

**Before:**

```c
XELPRESULT cmd_hello(const char *args, int len) {
    XELPOut(&cli, "Hello!\n", 0);        /* hardcoded global */
    return XELP_S_OK;
}

XELPRESULT key_help(int c) {
    return XELPHelp(&cli);               /* hardcoded global */
}
```

**After:**

```c
XELPRESULT cmd_hello(XELP *ths, const char *args, int len) {
    XELPOut(ths, "Hello!\n", 0);         /* works with any instance */
    return XELP_S_OK;
}

XELPRESULT key_help(XELP *ths, int c) {
    return XELPHelp(ths);                /* works with any instance */
}
```

### Changed
- **BREAKING:** CLI command function signature changed from
  `XELPRESULT fn(const char *args, int len)` to
  `XELPRESULT fn(XELP *ths, const char *args, int len)`.
- **BREAKING:** KEY command function signature changed from
  `XELPRESULT fn(int key)` to
  `XELPRESULT fn(XELP *ths, int key)`.
- **BREAKING:** Default handler signatures changed similarly:
  `mpfDefCLI` now takes `(XELP *, const char *, int)`,
  `mpfDefKey` now takes `(XELP *, int)`.
- `XELP` struct now uses a named tag (`struct XELP_tag`) to allow
  forward declaration in function pointer typedefs. The `XELP` typedef
  is unchanged -- no user code changes needed for the struct itself.
- Multi-instance example rewritten: eliminated per-instance command
  duplicates (`cmd_help_a`/`cmd_help_b`, `cmd_status_b`). A single
  `cmd_status()` now works correctly with any instance via `ths`.
- Updated all 7 example files, unit tests, and documentation.
- **BREAKING:** XelpBuf macro names normalized to SCREAMING_CASE for
  consistency with the rest of the API:
  `XELP_XBInit` -> `XELP_XB_INIT`,
  `XELP_XBInitPtrs` -> `XELP_XB_INIT_PTRS`,
  `XELP_XBInitBP` -> `XELP_XB_INIT_BP`,
  `XELP_XBPCopy` -> `XELP_XB_COPY`,
  `XELP_XBGetBufPtr` -> `XELP_XB_PTR`,
  `XELP_XBBufLen` -> `XELP_XB_LEN`,
  `XELP_XBGetPos` -> `XELP_XB_POS`,
  `XELP_XBPUTC` -> `XELP_XB_PUTC`,
  `XELP_XBPUTC_RAW` -> `XELP_XB_PUTC_RAW`,
  `XELP_XBGETC` -> `XELP_XB_GETC`,
  `XELP_XBTOP` -> `XELP_XB_TOP`,
  `XELPOutXB` -> `XELP_XB_OUT`.
  Removed unused `XELP_XBGetBuf` macro.

### Added
- Cross-compilation targets: m68k (Motorola 68000), RISC-V rv32,
  Xtensa LX106 (ESP8266/Tensilica) added to Docker crossbuild tooling
  and README size table.

## [0.2.3] - 2026-04-17

### Fixed
- Fixed 10 bugs across xelp.h, xelp.c, xelpcfg.h:
  - `XELPOutXB` macro parentheses
  - `XELP_XBGETC` macro read from wrong pointer
  - `_XOUTC` macro used GCC-only statement expressions
  - `XELPOut` with maxlen=0 printed nothing (now prints until null)
  - `XELPStr2Int` and `XELPParseNum` did not handle uppercase hex (A-F)
  - Duplicate `XELP_T_OK` macro definition
  - `XELP_STACK_OPS` / `XELP_STACK_MACHINE` name mismatch in xelpcfg.h
  - C89-incompatible `//` comment in xelpcfg.h
- Fixed `XELPTokLineXB` phantom line bug: tokenizer returned `XELP_S_OK`
  for trailing whitespace/newlines after last command, causing spurious
  command dispatch and incorrect default handler invocations
- Fixed 7 bugs in unit tests (unreachable code, wrong fields, stubs, etc.)
- Fixed all remaining `//` comments in src/ for C89 compliance
- Removed stale DEBUG code block and dead comments from xelp.c

### Changed
- **API naming consistency**: all public functions now use `XELP` prefix
  - `XelpNumToks` -> `XELPNumToks`
  - `XelpParseNum` -> `XELPParseNum`
  - `XelpBufCmp` -> `XELPBufCmp`
- **Status code naming**: all uppercase per C convention
  - `XELP_W_Warn` -> `XELP_W_WARN`
  - `XELP_E_Err` -> `XELP_E_ERR`
  - `XELP_E_CmdBufFull` -> `XELP_E_CMDBUFFULL`
  - `XELP_E_CmdNotFound` -> `XELP_E_CMDNOTFOUND`
- `XELP_VERSION` format changed from 16-bit (`0xMMmm`) to 32-bit (`0x00MMmmpp`)
  with accessor macros `XELP_VER_MAJOR`, `XELP_VER_MINOR`, `XELP_VER_PATCH`
- Reorganized repository structure (pages/, docs/, tools/, dev/)
- Replaced defunct Travis CI with GitHub Actions CI
- Updated `.gitignore` for build artifacts
- Rewrote README.md
- `XELP_VERSION` in xelp.h is now the sole version source of truth

### Added
- Default command handlers: `mpfDefKey` and `mpfDefCLI` function pointers
  called when no matching key/command is found, with setter macros
  `XELP_SET_FN_DEF_KEY` and `XELP_SET_FN_DEF_CLI`
- 100% line coverage of xelp.c (269 test cases across 21 units)
- Comprehensive buffer boundary tests (20 cases verifying buffer limits
  are never exceeded across ParseKey, ParseXB, Parse, and XelpBuf macros)
- Stress and hardening tests for malformed input
- Default handler tests (22 cases covering KEY and CLI handlers)
- Makefile `coverage` target
- Markdown documentation: API reference, configuration guide, porting guide
- Docker cross-compilation tooling (`tools/crossbuild.sh`)
- Banner generator tool (`tools/generate_banner.py`)
- Release script (`tools/make_release.sh`)
- Bare-metal and multi-instance examples
- `CONTRIBUTING.md`, `CHANGELOG.md`

## [0.2.1] - 2024-06-05

### Added
- `XelpParseNum` function for safer string-to-integer conversion
- Arduino C example (`examples/arduino/`)
- `XelpBufCmp` buffer comparison with multiple comparison modes
- `XELPFindTok` token search function

### Changed
- Expanded unit test coverage

## [0.2.0] - 2024-02-25

### Added
- Initial public release on GitHub
- Char-at-a-time CLI parser with KEY, CLI, and THRU modes
- Tokenizer with quoted strings, escape sequences, comments
- Command dispatch for both single-key and CLI modes
- Multi-instance support with no global state
- No dynamic memory allocation
- Platform abstraction layer (5 function pointers)
- Compile-time feature selection via `xelpcfg.h`
- Posix example with ncurses
- Unit tests with jumpbug framework
- Support for 8-bit through 64-bit architectures

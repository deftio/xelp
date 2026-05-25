# xelp Build Reference

> **Auto-generated** on 2026-05-25 (arena-based frames refactor)
> xelp 0.4.0 | Apple clang version 21.0.0 (clang-2100.1.1.101) | Darwin arm64
>
> Regenerate: `make build-ref`

---

## Build Profiles

Four ready-made profiles. Each is a set of `#define` flags that control which
features are compiled in. Unused features are stripped at compile time.

All flags are independent -- mix and match freely. These profiles are convenient
bundles, not hard requirements. For example, `XELP_ENABLE_THR` can be added to
any profile, or `XELP_ENABLE_SCRIPT` can be used without `XELP_ENABLE_HISTORY`.

| Profile | Flags | Description |
|---------|-------|-------------|
| **KEY** | `XELP_ENABLE_KEY` | Single-keypress dispatch only. Menus, debug jigs, minimal footprint. |
| **CLI** | `KEY` + `CLI` + `LINE_EDIT` + `HELP` | Interactive command line with cursor editing and help. |
| **HIST** | CLI + `HISTORY` + `THR` | CLI plus command history (UP/DOWN recall) and pass-through mode. |
| **SCRIPT** | HIST + `SCRIPT` | All features including the XelpScript engine. |

---

## Feature Matrix

Flags marked `*` have no dependencies and can be added to any profile.

| Feature | KEY | CLI | HIST | SCRIPT |
|---------|:---:|:---:|:----:|:------:|
| Single-key dispatch (menus) | Y | Y | Y | Y |
| Command line prompt + ENTER | -- | Y | Y | Y |
| Cursor movement (left/right, Home/End) | -- | Y | Y | Y |
| Insert-at-cursor, Delete | -- | Y | Y | Y |
| Multi-byte ANSI key recognition | -- | Y | Y | Y |
| Command dispatch (tokenizer, function tables) | -- | Y | Y | Y |
| Built-in help listing `*` | -- | Y | Y | Y |
| Script execution (semicolons, newlines, comments) | -- | Y | Y | Y |
| Quoted strings with escape sequences | -- | Y | Y | Y |
| Command history (UP/DOWN arrow) | -- | -- | Y | Y |
| THR pass-through mode `*` | -- | -- | Y | Y |
| Script variables (`_set`, `$var`) | -- | -- | -- | Y |
| Math builtins (`_add`, `_mul`, etc.) | -- | -- | -- | Y |
| Comparison + logic (`_eq`, `_lt`, `_not`) | -- | -- | -- | Y |
| Conditionals (`_if`/`_then`/`_else`) | -- | -- | -- | Y |
| Labels + jumps (`_goto`, `_next`) | -- | -- | -- | Y |
| Script functions (`_func`, `_return`, `@1`/`@2`) | -- | -- | -- | Y |
| C-registered ROM script functions | -- | -- | -- | Y |
| C interop (`XelpCallProc`) | -- | -- | -- | Y |
| Register read/write (`_mr`) | -- | -- | -- | Y |
| Parenthesized subexpressions | -- | -- | -- | Y |
| Breakpoint safety callback | -- | -- | -- | Y |

---

## Code Size (.text bytes, `-Os`)

| Platform | Bits | Compiler | KEY | CLI | HIST | SCRIPT |
|----------|-----:|----------|----:|----:|-----:|-------:|
| AVR (ATtiny85) | 8 | avr-gcc | 990 | 4069 | 4995 | 14054 |
| AVR (ATmega328P) | 8 | avr-gcc | 998 | 4155 | 5093 | 14546 |
| Z80 | 8 | SDCC | 1969 | 7205 | 8381 | 26733 |
| 6800 (HC08) | 8 | SDCC | 2096 | 8268 | 9785 | 31894 |
| MSP430 | 16 | msp430-gcc | 782 | 3240 | 4032 | 12578 |
| 68HC11 | 16 | m68hc11-gcc | 2169 | 6739 | 8616 | 30851 |
| ARM Thumb | 32 | arm-none-eabi-gcc | 600 | 2575 | 3071 | 9200 |
| Xtensa LX7 (ESP32-S3) | 32 | xtensa-esp-elf-gcc | 620 | 2677 | 3113 | 9542 |
| m68k | 32 | m68k-linux-gnu-gcc | 746 | 3167 | 3881 | 11098 |
| RISC-V (rv32) | 32 | riscv64-unknown-elf-gcc | 746 | 3078 | 3646 | 10970 |
| Xtensa LX106 (ESP8266) | 32 | xtensa-lx106-elf-gcc | 747 | 2956 | 3456 | 10469 |
| ARM32 | 32 | arm-none-eabi-gcc | 1008 | 3895 | 4607 | 13704 |
| x86-32 | 32 | GCC | 1099 | 4523 | 5238 | 15102 |
| MIPS32 | 32 | mipsel-linux-gnu-gcc | 1312 | 4864 | 5728 | 15472 |
| PowerPC | 32 | powerpc-linux-gnu-gcc | 1536 | 5591 | 6379 | 16832 |
| RISC-V (rv64) | 64 | riscv64-linux-gnu-gcc | 780 | 3292 | 3900 | 11682 |
| x86-64 | 64 | Clang | 1060 | 4661 | 5649 | 21711 |
| x86-64 | 64 | GCC | 1084 | 4556 | 5377 | 13986 |
| AArch64 (ARM64) | 64 | aarch64-linux-gnu-gcc | 1336 | 4931 | 5639 | 14928 |
| MIPS64 | 64 | mips64el-linux-gnuabi64-gcc | 1376 | 5392 | 6464 | 17360 |

All targets compiled with `-Os`. Sizes are `.text` section only (no data/BSS).
The x86-64 Clang row is measured on the host; all other rows are from the last
Docker cross-build and the SCRIPT column may be stale after major refactors.
Run `make prerelease` to regenerate all rows (requires Docker).

---

## RAM Per Instance (`sizeof(XELP)`, default buffer sizes)

With `XELP_CMDBUFSZ=64`, `XELP_HIST_DEPTH=4`, `XELP_SCRIPT_ARENA_SZ=sizeof(int)*256`.

| Platform class | KEY | CLI | HIST | SCRIPT |
|----------------|----:|----:|-----:|-------:|
| 16-bit (MSP430, AVR) | ~50 | ~192 | ~522 | ~1596 |
| 32-bit (ARM Thumb, ESP32, RISC-V) | ~64 | ~220 | ~552 | ~1636 |
| 64-bit (x86-64, AArch64) | 96 | 280 | 616 | 1720 |

64-bit row is measured on Darwin arm64. 32-bit and 16-bit rows are
estimated from the measured value by adjusting for pointer width (each pointer
field is 8 bytes on host vs 4 or 2 bytes on the target). Actual values
may vary slightly due to alignment differences.

Most of the RAM is fixed-size arrays (`mCmdMsgBuf`, `mHistBuf`, `mArena`) that
don't change with word size. The pointer-dependent portion is small relative
to the buffers.

### What Dominates RAM

| Component | Bytes | Present in |
|-----------|------:|-----------|
| `mArena[]` (script engine arena) | 512-2048 | SCRIPT only (scales with word size) |
| `mHistBuf[4][64]` + `mHistSaved[64]` (history ring) | 324 | HIST, SCRIPT |
| `mCmdMsgBuf[64]` + `mArgvBuf[64]` (CLI buffers) | 128 | CLI, HIST, SCRIPT |
| `mR[4]` (return registers) | 16 | All |
| Pointers (function tables, callbacks, script context) | 12-160 | varies by config and word size |
| Scalar state (mode, counters, flags) | ~10 | All |

---

## RAM Tuning Knobs

| Parameter | Default | Effect |
|-----------|--------:|--------|
| `XELP_CMDBUFSZ` | 64 | CLI input buffer + each history slot. Reducing to 32 saves 32 B x (1 + HIST_DEPTH + 1) buffers. |
| `XELP_HIST_DEPTH` | 4 | History ring slots. Reducing to 2 saves `2 * XELP_CMDBUFSZ` (~128 B). |
| `XELP_SCRIPT_ARENA_SZ` | `sizeof(int)*256` | Script engine arena. 512 on 16-bit, 1024 on 32-bit, 2048 on 64-bit. Override with `-DXELP_SCRIPT_ARENA_SZ=N`. |
| `XELP_ARGVBUFSZ` | 64 | Tokenization scratch buffer. Defaults to `XELP_CMDBUFSZ`. |
| `XELP_ARGV_MAX` | 8 | Max arguments per command. |
| `XELP_REGS_SZ` | 4 | Return registers (min 4). Each is `sizeof(XELPREG)` bytes. |
| `XELPREG` | `int` | Register type. Use `short` on 8-bit targets to save space. |

---

## Compile-Time Flags

### Feature Flags

| Flag | What it enables | Requires | Code cost |
|------|----------------|----------|----------|
| `XELP_ENABLE_KEY` | Single key press dispatch (menus) | -- | ~200-500 B |
| `XELP_ENABLE_CLI` | Command line prompt, tokenizer, command dispatch | -- | ~1.5-2.5 KB |
| `XELP_ENABLE_LINE_EDIT` | Cursor movement, insert-at-cursor, Delete, ANSI keys | `CLI` | ~800-1000 B |
| `XELP_ENABLE_HISTORY` | Command history (UP/DOWN arrow recall) | `CLI` + `LINE_EDIT` | ~420 B |
| `XELP_ENABLE_THR` | Pass-through mode (redirect keys to peripheral) | **none** | ~50-125 B |
| `XELP_ENABLE_HELP` | Built-in help command listing | **none** | ~180-350 B |
| `XELP_ENABLE_SCRIPT` | XelpScript: variables, math, conditionals, labels, functions | `CLI` | ~6-16 KB |
| `XELP_ENABLE_FULL` | Shorthand: KEY + CLI + THR + HELP (no LINE_EDIT, HISTORY, SCRIPT) | -- | combined |

### Dependency Rules

Unmet dependencies are silently disabled (no compile errors):

- `LINE_EDIT` requires `CLI`
- `HISTORY` requires `CLI` + `LINE_EDIT`
- `SCRIPT` requires `CLI`
- `KEY`, `THR`, `HELP` -- **no dependencies**, work alone or with any profile

### Key Mappings

| Define | Default | Purpose |
|--------|---------|---------|
| `XELPKEY_CLI` | CTRL-P (0x10) | Enter CLI mode |
| `XELPKEY_KEY` | ESC (0x1B) | Enter KEY mode |
| `XELPKEY_THR` | CTRL-T (0x14) | Enter THR mode |

### Escape Characters

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_CLI_ESC` | ` (backtick) | Escape at command line / in scripts |
| `XELP_QUO_ESC` | `\\` (backslash) | Escape inside quoted strings |
| `XELP_ESC_MAP` | `"n\x0A" "t\x09" ""` | Quoted string escape map |

### Enter Key Detection

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_ENTER_LF` | 1 | Accept LF (`\n`) as ENTER |
| `XELP_ENTER_CR` | 1 | Accept CR (`\r`) as ENTER |

Both enabled by default. CR+LF pairs are coalesced.

### Prompt and Help

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_CLI_PROMPT` | `"xelp>"` | CLI prompt string |
| `XELP_HELP_KEY_STR` | `"\nKey functions\n"` | Help section header for KEY commands |
| `XELP_HELP_CLI_STR` | `"\nCLI functions\n"` | Help section header for CLI commands |
| `XELP_HELP_ABT_STR` | `(ths->mpAboutMsg)` | About message at top of help |

For per-instance prompts: `#define XELP_CLI_PROMPT (ths->mpPrompt)` then
`XELP_SET_VAL_CLI_PROMPT(myXelp, "ser1>")` at runtime.

---

## Config Override

Customize the build without modifying source files:

1. Pass `-DXELP_CONFIG_OVERRIDE` in compiler flags
2. Create `xelp_ovr.h` in your include path
3. `#undef` to disable, `#undef` + `#define` to change

```c
/* xelp_ovr.h -- lean build for ATtiny (KEY only) */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP
#undef XELP_ENABLE_SCRIPT
```

```c
/* xelp_ovr.h -- CLI + THR, no history, no scripting */
#undef XELP_ENABLE_CLI
#undef XELP_ENABLE_LINE_EDIT
#undef XELP_ENABLE_HISTORY
#undef XELP_ENABLE_KEY
#undef XELP_ENABLE_THR
#undef XELP_ENABLE_HELP
#undef XELP_ENABLE_SCRIPT

#define XELP_ENABLE_KEY       1
#define XELP_ENABLE_CLI       1
#define XELP_ENABLE_LINE_EDIT 1
#define XELP_ENABLE_HELP      1
#define XELP_ENABLE_THR       1
```

---

*Generated by `tools/gen_build_reference.sh`*

# xelp 0.4.0 Resources

Build sizes and RAM estimates for the 0.4.0 development branch.

## Configuration Tiers

| Tier | Defines |
|------|---------|
| **KEY**  | `XELP_ENABLE_KEY` |
| **CLI**  | KEY + `XELP_ENABLE_CLI` + `XELP_ENABLE_LINE_EDIT` + `XELP_ENABLE_HELP` |
| **FULL** | CLI + `XELP_ENABLE_HISTORY` + `XELP_ENABLE_ARGV` + `XELP_ENABLE_THR` |

Default buffer sizes: `XELP_CMDBUFSZ=64`, `XELP_ARGVBUFSZ=64`,
`XELP_HIST_DEPTH=4`, `XELP_REGS_SZ=4`, `XELP_ARGV_MAX=8`.

---

## Unified Flash + RAM Table

All flash values are .text bytes compiled with `-Os`. RAM is approximate
`sizeof(XELP)` + peak dispatch stack for the tier (before alignment padding).
Stack counts because on a microcontroller it's the same SRAM pool.

### 8-bit targets (2-byte ptr, 2-byte int) — RAM: KEY ~29 B, CLI ~193 B, FULL ~519 B

| Target             | Compiler  | KEY flash | KEY RAM | CLI flash | CLI RAM | FULL flash | FULL RAM |
|--------------------|-----------|----------:|--------:|----------:|--------:|-----------:|---------:|
| AVR (ATmega328P)   | avr-gcc   |       998 |     ~29 |      4145 |    ~193 |       5083 |     ~519 |
| AVR (ATtiny85)     | avr-gcc   |       990 |     ~29 |      4059 |    ~193 |       4985 |     ~519 |
| Z80                | SDCC      |      1969 |     ~29 |      7164 |    ~193 |       8340 |     ~519 |
| 6800 (HC08)        | SDCC      |      2096 |     ~29 |      8260 |    ~193 |       9733 |     ~519 |

### 16-bit targets (2-byte ptr, 2-byte int) — RAM: KEY ~29 B, CLI ~193 B, FULL ~519 B

| Target             | Compiler     | KEY flash | KEY RAM | CLI flash | CLI RAM | FULL flash | FULL RAM |
|--------------------|--------------|----------:|--------:|----------:|--------:|-----------:|---------:|
| MSP430             | msp430-gcc   |       782 |     ~29 |      3234 |    ~193 |       4026 |     ~519 |
| 68HC11             | m68hc11-gcc  |      2169 |     ~29 |      6709 |    ~193 |       8586 |     ~519 |

### 32-bit targets (4-byte ptr, 4-byte int) — RAM: KEY ~51 B, CLI ~255 B, FULL ~583 B

| Target                  | Compiler                | KEY flash | KEY RAM | CLI flash | CLI RAM | FULL flash | FULL RAM |
|-------------------------|-------------------------|----------:|--------:|----------:|--------:|-----------:|---------:|
| x86-32                  | GCC                     |      1099 |     ~51 |      4510 |    ~255 |       5225 |     ~583 |
| ARM32                   | arm-none-eabi-gcc       |      1008 |     ~51 |      3891 |    ~255 |       4603 |     ~583 |
| ARM Thumb               | arm-none-eabi-gcc       |       600 |     ~51 |      2563 |    ~255 |       3059 |     ~583 |
| m68k                    | m68k-linux-gnu-gcc      |       746 |     ~51 |      3151 |    ~255 |       3865 |     ~583 |
| PowerPC                 | powerpc-linux-gnu-gcc   |      1536 |     ~51 |      5567 |    ~255 |       6355 |     ~583 |
| RISC-V (rv32)           | riscv64-unknown-elf-gcc |       746 |     ~51 |      3068 |    ~255 |       3636 |     ~583 |
| Xtensa LX106 (ESP8266)  | xtensa-lx106-elf-gcc   |       747 |     ~51 |      2940 |    ~255 |       3436 |     ~583 |
| Xtensa LX7 (ESP32-S3)  | xtensa-esp-elf-gcc      |       620 |     ~51 |      2661 |    ~255 |       3101 |     ~583 |
| MIPS32                  | mipsel-linux-gnu-gcc    |      1312 |     ~51 |      4864 |    ~255 |       5728 |     ~583 |

### 64-bit targets (8-byte ptr, 4/8-byte int) — RAM: KEY ~75 B, CLI ~367 B, FULL ~707 B

| Target                  | Compiler                    | KEY flash | KEY RAM | CLI flash | CLI RAM | FULL flash | FULL RAM |
|-------------------------|-----------------------------|----------:|--------:|----------:|--------:|-----------:|---------:|
| x86-64                  | GCC                         |      1084 |     ~75 |      4519 |    ~367 |       5340 |     ~707 |
| x86-64                  | Clang                       |      1069 |     ~75 |      4971 |    ~367 |       6045 |     ~707 |
| AArch64 (ARM64)         | aarch64-linux-gnu-gcc       |      1336 |     ~75 |      4915 |    ~367 |       5623 |     ~707 |
| RISC-V (rv64)           | riscv64-linux-gnu-gcc       |       780 |     ~75 |      3290 |    ~367 |       3898 |     ~707 |
| MIPS64                  | mips64el-linux-gnuabi64-gcc |      1376 |     ~75 |      5392 |    ~367 |       6480 |     ~707 |

Note: RAM totals include struct + peak stack during dispatch (stack is RAM
on a microcontroller). Stack figures are for `XelpParseXB` locals only.

---

## v0.3.2 vs 0.4.0 RAM Comparison

v0.3.2 had no multi-config split — all features always compiled in, so
v0.3.2 is comparable to 0.4.0 FULL. Struct changes in 0.4.0:
- Added `mArgvBuf[64]` (+64 B) for argc/argv scratch tokenization
- Added `mLastWasCR` (+1 B, only when both `XELP_ENTER_CR` and
  `XELP_ENTER_LF` defined)

Stack changes in 0.4.0: `XelpParseXB` now allocates `argv[XELP_ARGV_MAX]`
(8 pointers) and `int argc` on the stack. v0.3.2 only had `XelpBuf line`
+ `*f` (4 ptrs + 1 ptr).

### Total RAM (struct + peak dispatch stack)

v0.3.2 dispatch stack: `XelpBuf line` (3 ptr) + `*f` (1 ptr) = 4 ptrs
0.4.0  dispatch stack: `XelpBuf line` (3 ptr) + `*f` (1 ptr) + `argv[8]` (8 ptr) + `int argc` (1 int)

| Width    | v0.3.2 total | 0.4.0 FULL total | Delta   | 0.4.0 CLI total | 0.4.0 KEY total |
|----------|-------------:|-----------------:|--------:|----------------:|----------------:|
| 8/16-bit |       ~441 B |           ~519 B | +78 B   |          ~193 B |          ~29 B  |
| 32-bit   |       ~491 B |           ~583 B | +92 B   |          ~255 B |          ~51 B  |
| 64-bit   |       ~583 B |           ~707 B | +124 B  |          ~367 B |          ~75 B  |

Breakdown of the delta (0.4.0 FULL vs v0.3.2):
- `mArgvBuf[64]`: +64 B (all widths)
- `argv[8]` on stack: +16 B (8/16-bit) / +32 B (32-bit) / +64 B (64-bit)
- `int argc` on stack: +2 B (8/16-bit) / +4 B (32-bit) / +4 B (64-bit)
- Less `*f` already counted in v0.3.2 stack — net adjustment: -4 / -8 / -8

In exchange, commands get proper argc/argv parsing with quote and escape
handling instead of raw `(buf, len)`.

Users who don't need argc/argv can stay on CLI tier (no `mArgvBuf`) and
save 326+ bytes vs FULL (no history ring either), ending up well under
v0.3.2 despite the new line-editing features.

---

## .text Size Detail (bytes, -Os)

### 8-bit targets

| Target                | Compiler   | KEY  | CLI  | FULL |
|-----------------------|------------|-----:|-----:|-----:|
| AVR (ATmega328P)      | avr-gcc    |  998 | 4145 | 5083 |
| AVR (ATtiny85)        | avr-gcc    |  990 | 4059 | 4985 |
| Z80                   | SDCC       | 1969 | 7164 | 8340 |
| 6800 (HC08)           | SDCC       | 2096 | 8260 | 9733 |

### 16-bit targets

| Target                | Compiler      | KEY  | CLI  | FULL |
|-----------------------|---------------|-----:|-----:|-----:|
| MSP430                | msp430-gcc    |  782 | 3234 | 4026 |
| 68HC11                | m68hc11-gcc   | 2169 | 6709 | 8586 |

### 32-bit targets

| Target                    | Compiler                  | KEY  | CLI  | FULL |
|---------------------------|---------------------------|-----:|-----:|-----:|
| x86-32                    | GCC                       | 1099 | 4510 | 5225 |
| ARM32                     | arm-none-eabi-gcc         | 1008 | 3891 | 4603 |
| ARM Thumb                 | arm-none-eabi-gcc         |  600 | 2563 | 3059 |
| m68k                      | m68k-linux-gnu-gcc        |  746 | 3151 | 3865 |
| PowerPC                   | powerpc-linux-gnu-gcc     | 1536 | 5567 | 6355 |
| RISC-V (rv32)             | riscv64-unknown-elf-gcc   |  746 | 3068 | 3636 |
| Xtensa LX106 (ESP8266)   | xtensa-lx106-elf-gcc      |  747 | 2940 | 3436 |
| Xtensa LX7 (ESP32-S3)    | xtensa-esp-elf-gcc        |  620 | 2661 | 3101 |
| MIPS32                    | mipsel-linux-gnu-gcc      | 1312 | 4864 | 5728 |

### 64-bit targets

| Target                    | Compiler                      | KEY  | CLI  | FULL |
|---------------------------|-------------------------------|-----:|-----:|-----:|
| x86-64                    | GCC                           | 1084 | 4519 | 5340 |
| x86-64                    | Clang                         | 1069 | 4971 | 6045 |
| AArch64 (ARM64)           | aarch64-linux-gnu-gcc         | 1336 | 4915 | 5623 |
| RISC-V (rv64)             | riscv64-linux-gnu-gcc         |  780 | 3290 | 3898 |
| MIPS64                    | mips64el-linux-gnuabi64-gcc   | 1376 | 5392 | 6480 |

---

## v0.3.2 vs 0.4.0 FULL-config Comparison

v0.3.2 did not have multi-config builds (KEY/CLI/FULL all identical).
Not apples-to-apples: 0.4.0 adds argc/argv, history, line editing,
escape handling, and the multi-config `#ifdef` structure.

| Target              | v0.3.2 | 0.4.0 | Delta |
|---------------------|-------:|------:|------:|
| ARM Thumb           |  3,074 | 3,059 |   -15 |
| Xtensa LX7          |  3,060 | 3,101 |   +41 |
| RISC-V rv32         |  3,654 | 3,636 |   -18 |
| x86-64 (GCC)        |  5,935 | 5,340 |  -595 |
| AVR (ATtiny85)      |  5,174 | 4,985 |  -189 |
| MSP430              |  4,272 | 4,026 |  -246 |

---

## RAM Estimates (XELP struct, default config)

Approximate `sizeof(XELP)` per tier before struct alignment padding.
Stack usage during command dispatch adds `XELP_ARGV_MAX` pointers
(8 ptrs = 16-64 bytes depending on pointer width).

### Struct field breakdown

**Always present (all tiers):**

| Field           | Type          | 8/16-bit | 32-bit |
|-----------------|---------------|:--------:|:------:|
| mCurMode        | int           |    2     |    4   |
| mOutEnable      | char          |    1     |    1   |
| mEchoChar       | char          |    1     |    1   |
| mpAboutMsg      | ptr           |    2     |    4   |
| mR[4]           | int x 4       |    8     |   16   |
| mKeyAccum       | unsigned long |    4     |    4   |
| mKeyLen         | char          |    1     |    1   |
| mpfOut          | fn ptr        |    2     |    4   |
| mpfErr          | fn ptr        |    2     |    4   |
| mpfEditModeChg  | fn ptr        |    2     |    4   |
| **Subtotal**    |               |  **25**  | **43** |

**KEY adds:**

| Field           | Type          | 8/16-bit | 32-bit |
|-----------------|---------------|:--------:|:------:|
| mpKeyModeFuncs  | ptr           |    2     |    4   |
| mpfDefKey       | fn ptr        |    2     |    4   |
| **Subtotal**    |               |   **4**  |  **8** |

**CLI adds** (includes LINE_EDIT, HELP, ARGV):

| Field           | Type          | 8/16-bit | 32-bit |
|-----------------|---------------|:--------:|:------:|
| mpCLIModeFuncs  | ptr           |    2     |    4   |
| mpfDefCLI       | fn ptr        |    2     |    4   |
| mCmdMsgBuf[64]  | char[64]      |   64     |   64   |
| mCmdXB          | XelpBuf (3ptr)|    6     |   12   |
| mArgvBuf[64]    | char[64]      |   64     |   64   |
| mCur            | ptr           |    2     |    4   |
| mpfBksp         | fn ptr        |    2     |    4   |
| **Subtotal**    |               | **142**  |**156** |

**FULL adds** (HISTORY + THR):

| Field           | Type            | 8/16-bit | 32-bit |
|-----------------|-----------------|:--------:|:------:|
| mHistBuf[4][64] | char[4][64]     |  256     |  256   |
| mHistWrite      | char            |    1     |    1   |
| mHistCount      | char            |    1     |    1   |
| mHistBrowse     | char            |    1     |    1   |
| mHistSaved[64]  | char[64]        |   64     |   64   |
| mHistSavedLen   | char            |    1     |    1   |
| mpfPassThru     | fn ptr          |    2     |    4   |
| **Subtotal**    |                 | **326**  |**328** |

### Totals (struct + peak dispatch stack)

| Tier     | AVR / MSP430 (8/16-bit) | ARM / RISC-V (32-bit) | x86-64 (64-bit) |
|----------|:-----------------------:|:---------------------:|:----------------:|
| **KEY**  |  ~29 B                  |  ~51 B                |  ~75 B           |
| **CLI**  | ~193 B                  | ~255 B                | ~367 B           |
| **FULL** | ~519 B                  | ~583 B                | ~707 B           |

The history ring (`mHistBuf[4][64]` = 256 B) is the single largest RAM
consumer. On very constrained parts (ATtiny85 = 512 B SRAM), only KEY
or CLI-without-history is practical. Reducing `XELP_HIST_DEPTH` to 2
saves 128 bytes; reducing `XELP_CMDBUFSZ` to 32 cuts all buffer fields
in half.

### Dispatch stack breakdown

`XelpParseXB` stack locals (CLI and FULL tiers only):
- `argv[XELP_ARGV_MAX]`: 8 pointers (16 B on AVR, 32 B on ARM, 64 B on x86-64)
- `XelpBuf line`: 3 pointers (6 B / 12 B / 24 B)
- `*f`: 1 pointer (2 B / 4 B / 8 B)
- `int argc`: (2 B / 4 B / 4 B)

Peak dispatch stack: ~26 B (8/16-bit) / ~52 B (32-bit) / ~100 B (64-bit).

KEY tier has no CLI dispatch so no argv/line stack cost.

---

## Design Notes (from exp_argv_core session)

### Dispatch table unification (KEY vs CLI)

Considered making KEY and CLI use the same dispatch table type. Currently:
- KEY handlers: `fn(ths, keycode)` — integer match, no parsing
- CLI handlers: `fn(ths, argc, argv)` — string match, full tokenization

The function signatures are fundamentally different. Unifying would mean
either forcing KEY handlers to take argc/argv (overhead on every keypress)
or using void*/union (loses type safety). The match predicates also differ
(integer == vs string compare). Multiple commands can point to the same
function (e.g. `_eq`, `_add`, `_sub` → one math handler that switches on
`argv[0]`), which already works cleanly with both table types.

Verdict: not worth it. The two walker loops are small and the type safety
matters more than a few bytes of code dedup.

### argc/argv rationale

The `(buf, len)` raw interface from v0.3.2 works but pushes parsing burden
onto every command handler. argc/argv:
- Every C programmer knows it — zero learning curve for writing handlers
- Handlers are portable: testable as standalone `main(argc, argv)` wrappers
- Quoting/escaping solved once in `_xelpBuf2Argv`, not per-handler
- Xelpscript variable substitution has a clean injection point into argv
- Cost: +64 B struct (`mArgvBuf`) + stack-local pointer array during dispatch

XelpBuf remains the right internal representation (zero-copy, ROM-safe).
argc/argv is the handler-facing API. `_xelpBuf2Argv` bridges the two.

### Cache behavior on 32/64-bit

On 32-bit and 64-bit platforms with L1 caches (typically 32-128 KB):
- FULL .text: 3-6 KB → fits in L1 I-cache (~10-16% of 32 KB)
- FULL struct: ~583-707 B → fits in L1 D-cache (~2%)
- Total code + data: ~6 KB on 64-bit → under 10% of typical L1

Once warm, the entire interpreter — tokenizer, argv builder, dispatch
loop, command call — runs from L1 with no cache misses. Relevant for
real-time systems where a debug CLI shouldn't perturb timing.

### Bare-metal debug monitor use case

xelp works pre-OS with zero dependencies (no malloc, no stdio, no
string.h). A bare-metal boot debugger pattern:

```c
XELP dbg;
XelpInit(&dbg, "boot>");
XELP_SET_FN_OUT(dbg, uart_putc);
XELP_SET_FN_CLI(dbg, debug_cmds);  // peek, poke, dump, regs, go
while (!boot_ready)
    if (uart_has_byte()) XelpParseKey(&dbg, uart_getc());
```

Similar to classic MacsBug / ROM monitors but at ~6 KB vs ~40 KB.
Applicable to AArch64 EL3/EL2 firmware, RISC-V SBI, any pre-OS context
that has a UART but no interactive debug.

### Multi-instance peripheral bringup

Multiple xelp instances, each on its own UART/peripheral:

```c
XELP dbg;     // UART0 — main debug console
XELP modem;   // UART1 — cellular module
XELP wifi;    // UART2 — ESP-AT module
XELP ble;     // UART3 — BLE module
```

Each instance gets its own putc, command table, and mode config.
THRU mode on peripheral instances gives raw AT-command passthrough —
flip to THRU, type raw AT commands, flip back to CLI for scripted
sequences. Three phases during bringup:

1. **THRU** — raw passthrough, manual AT poking
2. **CLI** — command table wraps AT sequences (`signal` → `AT+CSQ`)
3. **KEY** — single-key shortcuts for frequent operations

At ~583 B RAM per instance (32-bit FULL), four instances = ~2.3 KB.
Trivial on Cortex-M4 with 64+ KB SRAM.

The debug console's command table can operate on other instances —
`modem thru`, `wifi reset`, `ble scan` — routing between xelp instances.

### CLI without history — the sweet spot for small platforms

CLI tier (no `XELP_ENABLE_HISTORY`) is the unlisted compelling config:
- ~193 B RAM on 8/16-bit, ~255 B on 32-bit
- Full line editing (cursor, insert, delete, home/end)
- argc/argv dispatch, help, command tables
- No history ring (saves ~324 B)

On ATmega328P (2 KB SRAM) or MSP430G2553 (512 B): a real interactive
shell with proper argument parsing at under 40% of available SRAM.

### Xelpscript design direction

Goal: script engine as "just another dispatch table" with minimal core
changes. Script keywords (goto, if, set, call) are a third dispatch
table using the same `XELPCLIFuncMapEntry` type.

Additions needed:
1. Third table pointer in XELP struct (`mpScriptFuncs`)
2. Larger scratch buffer (or separate script scratch)
3. Minimal script state: program counter (XelpBuf into script),
   small call/return stack, variable slots
4. Dispatch order: check script table first, fall through to CLI table

Script execution approach: `XelpRunScript` as a parallel entry point
to `XelpParseXB` — same tokenizer loop, different dispatch priority.
Interactive and script paths stay cleanly separated. Script-only
keywords never pollute the CLI table.

XelpBuf cursor walk for flow control: `goto` resets `buf.p`, loop
constructs manipulate the cursor. Zero-copy, ROM-safe execution
directly from the script buffer.

### Size targets for xelpscript

- Flash: < 10 KB (prefer < 8 KB) FULL + script on 32-bit
- RAM: ~1 KB for small scripts (struct + script state + scratch)
- Current FULL baseline on ARM Thumb: 3,059 B → ~5-7 KB budget for
  script engine code
- First target: bare-metal ESP32-S3 (Xtensa LX7) for micro synth project

### Branch plan

1. Clean up `exp_argv_core` branch (current)
2. Return to 0.3.3 branch, release it
3. Come back to 0.4.0 for xelpscript development

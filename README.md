<a href="https://deftio.github.io/xelp/pages/"><img src="https://deftio.github.io/xelp/img/xelp-prompt-med.png" width="30%"></img></a>

![Version](https://img.shields.io/badge/version-0.3.2-blue.svg)
[![License](https://img.shields.io/badge/License-BSD%202--Clause-blue.svg)](https://opensource.org/licenses/BSD-2-Clause)
[![CI](https://github.com/deftio/xelp/actions/workflows/ci.yml/badge.svg)](https://github.com/deftio/xelp/actions/workflows/ci.yml)
![Coverage](https://img.shields.io/badge/coverage-100%25-brightgreen.svg)  
[![PlatformIO](https://img.shields.io/badge/PlatformIO-library-teal.svg)](https://registry.platformio.org/libraries/deftio/xelp)
[![Arduino](https://img.shields.io/badge/Arduino-library-teal.svg)](https://github.com/deftio/xelp)
[![ESP Component](https://img.shields.io/badge/ESP--IDF-component-teal.svg)](https://components.espressif.com/components/deftio/xelp)

# xelp

A tiny extensible command interpreter with scripting support for embedded systems. 
Add an interactive serial CLI, single-key menus, or scripted command sequences to any microcontroller --
from an 8-bit ATtiny85 to a 64-bit ARM Cortex-A. Pure C, no malloc, no OS,
under 3-5 KB fully featured depending on archicture. 

Xelp is instance based so it's possible to run distinct copies on different ports and it can be run inside interrupts (pending your own functions are safe).

<img src="https://deftio.github.io/xelp/img/xelp-cli-demo.png" width="70%" alt="xelp CLI demo session">

## Why xelp

Many embedded projects end up with an ad-hoc `if (char == 'x')` debug
console. xelp replaces that with a proper CLI that:

- Compiles on 8-bit to 64-bit targets with any C89 or later compiler
- Fits in ~1 KB (key dispatch only) to ~5 KB (full CLI with line editing)
- Uses zero dynamic memory -- no malloc, no heap, safe for ISRs
- Supports multiple independent instances -- one per UART, no globals
- Multiple commands can be saved as strings (scripts) and run from C or from the CLI.
- Scripts are ROM-able const strings -- the parser never modifies its input (e.g. no strtok style processing)
- Function dispatch tables make any C/C++ function callable from the CLI
- Live help listing (optional, compile-time removable), commands can have run time assistance
- CLI commands can be called from C or C functions can be called from the CLI.
- A cpp wrapper is provided with identical functionality and some syntactic sugar for ease of use (still no memory allocation)

## History
Xelp was first built for some embedded projects in the late 90s (though under different names) and then made more uniform in the late 2000s.


## Build Profiles

xelp is modular -- enable only what you need. Three profiles cover most use cases.

### KEY only (~1 KB)

Single-keypress dispatch: each key triggers a function immediately, no
ENTER needed. Ideal for debug menus and hardware test jigs.

```c
#define XELP_ENABLE_KEY  1
```

### CLI Lean (~1.4 KB)

Append-only command line with tokenizer, scripting, and command dispatch.
No line editing, no cursor movement, no help listing. Ideal for the
most size-constrained targets that still need a CLI.

```c
#define XELP_ENABLE_CLI  1
```

### CLI (~3-5 KB)

Line-buffered command prompt with cursor movement, line editing, multi-byte
ANSI key recognition, tokenizer, scripting, and help. This is the typical
interactive configuration.

```c
#define XELP_ENABLE_CLI        1
#define XELP_ENABLE_LINE_EDIT  1
#define XELP_ENABLE_KEY        1
#define XELP_ENABLE_HELP       1
```

Optional: add `XELP_ENABLE_HISTORY` for UP/DOWN arrow command recall
(~420 bytes, requires `XELP_ENABLE_LINE_EDIT`).

### Full (~3-6 KB)

Adds THR pass-through mode (~50-125 bytes more) for forwarding all
keystrokes to another peripheral such as a modem or radio module. This enables 
the cli to flip in to a mode where one can send raw commands (such AT commands) to another device.  

```c
#define XELP_ENABLE_FULL  1
```

Every flag is independent -- mix and match. For the full reference see
[Build Profiles & Configuration Guide](docs/build-profiles.md).

## Quick Start (Pure C)

See examples for simpler C++ version.

Add these three files to your project: `xelp.c`, `xelp.h`, `xelpcfg.h`.

```c
#include "xelpcfg.h"
#include "xelp.h"

/* Your output function -- write one char to UART, LCD, etc. */
void uart_putc(char c) { UART_TX = c; }

/* Your destructive backspace function - customize for your OS */
void uart_bksp(void)   { uart_putc('\b'); uart_putc(' '); uart_putc('\b'); }

/* Commands -- any C function with this signature */
XELPRESULT cmd_hello(XELP *ths, const char *args, int len) {
    XelpOut(ths, "Hello!\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmd_led(XELP *ths, const char *args, int len) {
    XelpArgs a;
    int val;
    XelpArgsInit(&a, args, len);
    XelpNextTok(&a, 0);                      /* skip command name */
    XelpNextInt(&a, &val);
    LED_PORT = val;
    return XELP_S_OK;
}

/* Command table */
XELPCLIFuncMapEntry commands[] = {
    { &cmd_hello, "hello", "say hello"  },
    { &cmd_led,   "led",   "led <0|1>"  },
    XELP_FUNC_ENTRY_LAST
};

XELP cli;

void main(void) {
    XelpInit(&cli, "My Device v1.0");
    XELP_SET_FN_OUT(cli, &uart_putc);
    XELP_SET_FN_BKSP(cli, &uart_bksp);
    XELP_SET_FN_CLI(cli, commands);

    for (;;) {
        if (uart_rx_ready())
            XelpParseKey(&cli, uart_getc());
    }
}
```

At the prompt:

```
xelp> hello
Hello!
xelp> led 1
xelp>
```

The prompt string is settable per instance:

```c
XELP_SET_VAL_CLI_PROMPT(cli, "mydev>");
```

The prompt is stored by pointer, not copied. It must be a null-terminated
string that remains valid for the life of the instance (string literal,
static, or global -- not a stack buffer that goes out of scope).

Prompts can be global (all instances share a prompt), or per-instance (each peripheral gets its own prompt)

## Scripting

Anything typed at the CLI can be run as a script. Scripts are const strings
parsed without modification -- they can live in ROM:

```c
const char *startup_script = "hello; led 1";   /* cli functions that are available plus mode switch imperatives are allowed */
XelpParse(&cli, startup_script, XelpStrLen(startup_script));
```

Scripts support semicolons (`;`), newlines, `#` comments, quoted strings
(`"..."`), and escape characters (backtick at CLI, backslash in quotes).

## Three Modes

```
         CTRL-P           ESC            CTRL-T
CLI mode -------> KEY mode -----> THR mode
   ^                                |
   +--------------------------------+
              mode switch keys
```

- **CLI**: Line-buffered input with prompt. Type commands, press ENTER.
- **KEY**: Each keypress triggers a command immediately. For menus.
- **THR**: All keys pass through to another peripheral. For debugging modems, serial devices.

Mode switch keys are configurable in `xelpcfg.h`.  If any mode is not enabled then the mode switch skips that mode

## Building and Testing

### Local development (fast, no Docker)

```bash
make validate       # build + run tests + build all examples (the everyday check)
make tests          # unit tests + coverage only
make examples       # build all examples (no interactive launch)
make example        # build and run the posix ncurses demo (interactive)
make coverage       # tests + coverage summary
make fuzz           # fuzz testing with libFuzzer (requires clang)
make funcsizes      # per-function compiled sizes (x86-32, ARM32)
make sizes          # print compiled sizes for all feature profiles
make clean          # remove test build artifacts
make clean-all      # clean tests + all examples
```

47 test units, 598 test cases, 100% line coverage of `xelp.c`.

Feature profile sizes: `dev/size_profiles.sh` (uses Docker for ARM Cortex-M0, falls back to host GCC).

### Pre-release (validates + updates size tables)

```bash
make prerelease     # tests + examples + Docker cross-compile + update README size tables
```

Runs `make validate`, then `tools/crossbuild.sh` (Docker), then
`tools/update_sizes.sh` to patch the compiled-size tables in README.md
and pages/index.html. Does not tag, push, or publish.

### Full release

```bash
bash tools/make_release.sh             # full guided release (includes Docker cross-build)
bash tools/make_release.sh --validate  # local validation only (no git, no push)
```

The release script handles everything end-to-end: validation (tests +
examples), manifest sync, badge update, Docker cross-compilation, size
table update, push, PR, CI, merge, tag, and publish. The Docker
cross-build step is skipped if Docker is not installed. Day-to-day
development uses `make validate` which takes seconds.

## Compiled Sizes

Compiled `.text` section sizes with `-Os`. Three configurations: KEY
(single-key dispatch only), CLI (typical interactive use), FULL (all
features). Even the largest full build is under 7 KB.

<!-- Build Size Table -->
| CPU | Width | Compiler | KEY (bytes) | CLI (bytes) | FULL (bytes) |
|-----|------:|----------|------------:|------------:|-------------:|
| AVR (ATtiny85) | 8 | avr-gcc | 5174 | 5174 | 5174 |
| AVR (ATmega328P) | 8 | avr-gcc | 5284 | 5284 | 5284 |
| Z80 | 8 | SDCC | 8386 | 8386 | 8386 |
| 6800 (HC08) | 8 | SDCC | 10078 | 10078 | 10078 |
| MSP430 | 16 | msp430-gcc | 4272 | 4272 | 4272 |
| 68HC11 | 16 | m68hc11-gcc | 8701 | 8701 | 8701 |
| Xtensa LX7 (ESP32-S3) | 32 | xtensa-esp-elf-gcc | 3060 | 3060 | 3060 |
| ARM Thumb | 32 | arm-none-eabi-gcc | 3074 | 3074 | 3074 |
| Xtensa LX106 (ESP8266) | 32 | xtensa-lx106-elf-gcc | 3447 | 3447 | 3447 |
| RISC-V (rv32) | 32 | riscv64-unknown-elf-gcc | 3654 | 3654 | 3654 |
| m68k | 32 | m68k-linux-gnu-gcc | 4044 | 4044 | 4044 |
| ARM32 | 32 | arm-none-eabi-gcc | 4638 | 4638 | 4638 |
| x86-32 | 32 | GCC | 5628 | 5628 | 5628 |
| MIPS32 | 32 | mipsel-linux-gnu-gcc | 6056 | 6056 | 6056 |
| PowerPC | 32 | powerpc-linux-gnu-gcc | 6842 | 6842 | 6842 |
| RISC-V (rv64) | 64 | riscv64-linux-gnu-gcc | 4136 | 4136 | 4136 |
| x86-64 | 64 | GCC | 5935 | 5935 | 5935 |
| AArch64 (ARM64) | 64 | aarch64-linux-gnu-gcc | 6282 | 6282 | 6282 |
| x86-64 | 64 | Clang | 6495 | 6495 | 6495 |
| MIPS64 | 64 | mips64el-linux-gnuabi64-gcc | 6904 | 6904 | 6904 |
<!-- Build Size Table -->

x86-64 GCC row is measured directly; others from cross-compilation via
`tools/Dockerfile.crossbuild`.  This is for size tracking, older versions / cousins of xelp were compiled and run on several (but not all of the above platforms)

## Configuration

All compile-time options (buffer size, key mappings, prompt, escape
characters, register count) are controlled via `#define` flags in
`src/xelpcfg.h`. See the
[Build Profiles & Configuration Guide](docs/build-profiles.md).

## Porting

xelp compiles on anything with a C89 compiler. To port:

1. Add `xelp.c`, `xelp.h`, `xelpcfg.h` to your build
2. Write a `void putc(char c)` function for your output hardware
3. Call `XELP_SET_FN_OUT()` and `XelpParseKey()` -- that's it

No assembly. No platform `#ifdefs` (except optional SDCC `__reentrant`).
See [Porting Guide](docs/porting.md).

## Architecture Support

The following toolchains compile xelp with zero warnings. Verified via
Docker cross-compilation (`tools/Dockerfile.crossbuild`):

| Architecture | Compiler | Word Size |
|-------------|----------|-----------|
| x86-64 | GCC, Clang | 64-bit |
| x86-32 | GCC, Clang | 32-bit |
| ARM64 | aarch64-linux-gnu-gcc | 64-bit |
| ARM32 / Thumb | arm-none-eabi-gcc | 32-bit |
| RISC-V (rv64) | riscv64-linux-gnu-gcc | 64-bit |
| RISC-V (rv32) | riscv64-unknown-elf-gcc | 32-bit |
| Xtensa LX106 (ESP8266) | xtensa-lx106-elf-gcc | 32-bit |
| Xtensa LX7 (ESP32-S3) | xtensa-esp-elf-gcc | 32-bit |
| MSP430 | msp430-gcc | 16-bit |
| m68k (68000) | m68k-linux-gnu-gcc | 32-bit |
| AVR (ATmega, ATtiny) | avr-gcc | 8-bit |
| 8051 | SDCC | 8-bit |
| 68HC11/12 | m68hc11-gcc | 16-bit |
| PowerPC | powerpc-linux-gnu-gcc | 32-bit |

## Repository Structure

```
xelp/
  src/            xelp.c, xelp.h, xelpcfg.h (the library -- add these to your project)
  tests/          unit tests (jumpbug framework), fuzz harnesses, 100% coverage
  examples/       POSIX ncurses, Arduino, C++ wrapper, bare-metal, scripting, ESP32 Wi-Fi/BLE, Pico
  tools/          cross-build scripts, release script, banner generator
  docs/           API reference, configuration guide, porting guide
  pages/          GitHub Pages site
  dev/            design notes, planning docs, size profiling tools
  .github/        CI workflows (build, test, fuzz, PlatformIO)
```

## Contributing

PRs welcome. `master` is protected -- all changes go through pull requests.
CI must pass (zero warnings, all tests, coverage) before merge.  Please note that 
PRs which affect multi-instance support or require dynamic memory may not be accepted.

```bash
git checkout -b dev-my-feature master
# make changes...
make clean && make tests    # must pass, zero warnings
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for full guidelines: coding standards,
what we welcome, branch model, and the release process.

## Documentation

- [Tutorial](docs/tutorial.md) -- step-by-step introduction to xelp
- [Examples](docs/examples.md) -- annotated code for various platforms
- [API Reference](docs/api-reference.md) -- all public functions, macros, types
- [Build Profiles & Configuration Guide](docs/build-profiles.md) -- feature system and compile-time options
- [Configuration Quick Reference](docs/configuration.md) -- all `#define` flags at a glance
- [Porting Guide](docs/porting.md) -- bringing up xelp on a new platform
- [Release Management](release_management.md) -- versioning, CI, release workflow
- [Tools](tools/README_TOOLS.md) -- build utilities and code generators
- [Contributing](CONTRIBUTING.md) -- how to contribute

## AI / LLM Integration

If you use AI coding agents (Claude Code, Cursor, Copilot, etc.), xelp
provides machine-readable context files for accurate code generation:

- [AGENTS.md](AGENTS.md) -- comprehensive coding reference: architecture, function signatures, parser internals, setup patterns, testing, common mistakes
- [llms.txt](llms.txt) -- project overview and full documentation index ([llmstxt.org](https://llmstxt.org) format)

## License

BSD 2-Clause. See [LICENSE.txt](LICENSE.txt).

Copyright (c) 2011-2026, M. A. Chatterjee

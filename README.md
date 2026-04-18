
<a href="https://www.deftio.com/xelp"><img src="./img/xelp-prompt-med.png" width="30%"></img></a>

![Version](https://img.shields.io/badge/version-0.2.3-blue.svg)
[![License](https://img.shields.io/badge/License-BSD%202--Clause-blue.svg)](https://opensource.org/licenses/BSD-2-Clause)
[![CI](https://github.com/deftio/xelp/actions/workflows/ci.yml/badge.svg)](https://github.com/deftio/xelp/actions/workflows/ci.yml)
![Coverage](https://img.shields.io/badge/coverage-100%25-brightgreen.svg)
![RISC-V](https://img.shields.io/badge/RISC--V-1.9KB-informational.svg)

# xelp

A command line interpreter and script engine for embedded systems, written in
pure C. No dynamic memory. No OS required. No standard library dependencies.
900 bytes to 4 KB compiled depending on features enabled.

xelp gives bare-metal firmware a real interactive CLI, scriptable command
dispatch, and single-key menus -- in a package small enough for an 8051 or
ATtiny85.

## Why xelp

Most embedded projects end up with an ad-hoc `if (char == 'x')` debug
console. xelp replaces that with a proper CLI that:

- Compiles on **8-bit to 64-bit** targets with any C89 compiler
- Fits in **under 1 KB** in KEY-only mode, **under 4 KB** fully featured
- Uses **zero dynamic memory** -- no malloc, no heap, works in ISRs
- Supports **multiple independent instances** -- one per UART, no globals
- Scripts are **ROM-able** const strings -- the parser never modifies input
- Function dispatch tables let you **add C functions callable from the CLI**

## Features

| Feature | Description | Size impact |
|---------|-------------|-------------|
| **CLI mode** | Line-buffered command prompt with backspace, prompt string, command dispatch | Core (~2 KB) |
| **KEY mode** | Single keypress menus and actions, no ENTER needed | ~200-500 bytes |
| **THR mode** | Pass-through redirects all keys to another peripheral (modem, debug port) | ~50-125 bytes |
| **Scripting** | Parse multi-statement scripts from ROM or RAM strings | Included with CLI |
| **Help** | Built-in help listing all registered commands | ~180-350 bytes |
| **Tokenizer** | Quoted strings, escape sequences, comments, semicolons | Included with CLI |

All features are independently compilable via `#define` flags in
`src/xelpcfg.h`. Disable what you don't need to save space.

## Quick Start

Add three files to your project: `xelp.c`, `xelp.h`, `xelpcfg.h`.

```c
#include "xelp.h"

/* Your output function -- write one char to UART, LCD, etc. */
void uart_putc(char c) { UART_TX = c; }
void uart_bksp(void)   { uart_putc('\b'); uart_putc(' '); uart_putc('\b'); }

/* Commands -- any C function with this signature */
XELPRESULT cmd_hello(const char *args, int len) {
    XELPOut(&cli, "Hello!\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmd_led(const char *args, int len) {
    XelpBuf b, tok;
    XELP_XBInit(b, args, len);
    XELPTokN(&b, 1, &tok);                  /* get second token */
    int val = XELPStr2Int(tok.s, tok.p - tok.s);
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
    XELPInit(&cli, "My Device v1.0");
    XELP_SET_FN_OUT(cli, &uart_putc);
    XELP_SET_FN_BKSP(cli, &uart_bksp);
    XELP_SET_FN_CLI(cli, commands);

    for (;;) {
        if (uart_rx_ready())
            XELPParseKey(&cli, uart_getc());
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

## Scripting

Anything typed at the CLI can be run as a script. Scripts are const strings
parsed without modification -- they can live in ROM:

```c
const char *startup_script = "hello; led 1";
XELPParse(&cli, startup_script, XELPStrLen(startup_script));
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

Mode switch keys are configurable in `xelpcfg.h`.

## Building and Testing

```bash
make tests          # build + run unit tests + coverage report
make coverage       # tests + coverage summary
make example        # build + run the posix ncurses example
make clean          # remove all build artifacts
```

19 test units, 207 test cases, 100% line coverage of `xelp.c`.

## Cross-Platform Size Table

Compiled sizes with `-Os` (all features enabled):

| Target | Compiler | Code size (bytes) |
|--------|----------|-------------------|
| ARM32 Thumb | arm-none-eabi-gcc | 1488 |
| MSP430 | msp430-gcc | 1804 |
| RISC-V (rv64) | riscv64-linux-gnu-gcc | 1960 |
| ARM32 | arm-none-eabi-gcc | 2372 |
| AVR (ATtiny85) | avr-gcc | 2420 |
| AVR (ATmega328P) | avr-gcc | 2452 |
| x86-64 | GCC | 2775 |
| x86-32 | GCC | 2826 |
| x86-64 | Clang | 3003 |
| ARM64 | aarch64-linux-gnu-gcc | 3148 |
| PowerPC | powerpc-linux-gnu-gcc | 3778 |
| 68HC11 | m68hc11-gcc | 4247 |

Sizes measured via Docker cross-compilation. KEY-only mode: 900 bytes.
Run `bash tools/crossbuild.sh` to reproduce.

## Configuration

Compile-time options in `src/xelpcfg.h`:

| Flag | Purpose |
|------|---------|
| `XELP_ENABLE_CLI` | Command line mode (required for scripting) |
| `XELP_ENABLE_KEY` | Single keypress mode |
| `XELP_ENABLE_THR` | Pass-through mode |
| `XELP_ENABLE_HELP` | Built-in help command |
| `XELP_ENABLE_LCORE` | Language core (peek, poke, go) |
| `XELP_ENABLE_FULL` | Enable all of the above |

Buffer size, register count, prompt string, escape characters, and mode
switch keys are all configurable. See [Configuration Guide](docs/configuration.md).

## Porting

xelp compiles on anything with a C89 compiler. To port:

1. Add `xelp.c`, `xelp.h`, `xelpcfg.h` to your build
2. Write a `void putc(char c)` function for your output hardware
3. Call `XELP_SET_FN_OUT()` and `XELPParseKey()` -- that's it

No assembly. No platform `#ifdefs` (except optional SDCC `__reentrant`).
See [Porting Guide](docs/porting.md).

## Architecture Support

Tested with zero warnings on:

| Architecture | Compiler | Word Size |
|-------------|----------|-----------|
| x86-64 | GCC, Clang | 64-bit |
| x86-32 | GCC, Clang, TCC | 32-bit |
| ARM64 | aarch64-linux-gnu-gcc | 64-bit |
| ARM32 / Thumb | arm-none-eabi-gcc | 32-bit |
| RISC-V | riscv64-linux-gnu-gcc | 64-bit |
| MSP430 | msp430-gcc | 16-bit |
| AVR (ATmega, ATtiny) | avr-gcc | 8-bit |
| 8051 | SDCC | 8-bit |
| 68HC11/12 | m68hc11-gcc | 8-bit |
| PowerPC | powerpc-linux-gnu-gcc | 32-bit |
| 6502 | cc65 | 8-bit |

## Repository Structure

```
xelp/
  src/            xelp.c, xelp.h, xelpcfg.h (the library -- add these to your project)
  tests/          unit tests (jumpbug framework), 100% coverage
  examples/       posix ncurses demo, bare-metal template, multi-instance
  tools/          cross-build scripts, release script, banner generator
  docs/           API reference, configuration guide, porting guide
  pages/          GitHub Pages site
  dev/            design notes and planning
  .github/        CI workflows (build, test, release)
```

## Contributing

PRs welcome. `master` is protected -- all changes go through pull requests.
CI must pass (zero warnings, all tests, coverage) before merge.

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
- [Configuration Guide](docs/configuration.md) -- compile-time options
- [Porting Guide](docs/porting.md) -- bringing up xelp on a new platform
- [Release Management](release_management.md) -- versioning, CI, release workflow
- [Tools](tools/README_TOOLS.md) -- build utilities and code generators
- [Contributing](CONTRIBUTING.md) -- how to contribute

## License

BSD 2-Clause. See [LICENSE.txt](LICENSE.txt).

Copyright (c) 2011-2025, M. A. Chatterjee

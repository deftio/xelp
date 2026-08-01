# Porting Guide

xelp is designed to be trivially portable. The library has no dependencies
on standard headers, no assembly, and no dynamic memory. Porting requires
implementing a small number of function pointers and compiling two files.

## Files to Include

Add to your project:

- `src/xelp.c` -- implementation
- `src/xelp.h` -- public header
- `src/xelpcfg.h` -- configuration (edit for your platform)

## Required: Output Function

xelp needs a way to emit characters. Provide a function with the signature
`void fn(char c)` and register it:

```c
void uart_putc(char c) {
    while (!UART_TX_READY);
    UART_TX_REG = c;
}

XELP_SET_FN_OUT(myXelp, &uart_putc);
```

## Required: Character Feeding

Feed received characters one at a time via `XelpParseKey`:

```c
/* In your UART ISR or main loop */
void UART_RX_ISR(void) {
    char c = UART_RX_REG;
    XelpParseKey(&myXelp, c);
}
```

## Optional: Backspace Handler

If using CLI mode, provide a destructive backspace handler:

```c
void handle_bksp(void) {
    uart_putc('\b');
    uart_putc(' ');
    uart_putc('\b');
}

XELP_SET_FN_BKSP(myXelp, &handle_bksp);
```

## Optional: Command History

If you enable `XELP_ENABLE_HISTORY` (requires `XELP_ENABLE_CLI` + `XELP_ENABLE_LINE_EDIT`),
users can recall previous commands with UP/DOWN arrows. No extra setup needed --
it works automatically. Be aware of the RAM cost:

```
RAM = XELP_HIST_DEPTH * XELP_CMDBUFSZ + XELP_CMDBUFSZ + 4 ints
    = 4 * 64 + 64 + 16 = 336 bytes  (default settings, 32-bit target)
```

Reduce `XELP_HIST_DEPTH` (default 4) or `XELP_CMDBUFSZ` (default 64) if RAM is tight.
Override via compiler flag (`-DXELP_HIST_DEPTH=2`) or `xelp_ovr.h` when using
`XELP_CONFIG_OVERRIDE`.

## Optional: Other Callbacks

| Callback | Signature | Purpose |
|----------|-----------|---------|
| `XELP_SET_FN_ERR` | `void fn(char)` | Error output (separate channel) |
| `XELP_SET_FN_THR` | `void fn(char)` | Pass-through output (modem, etc.) |
| `XELP_SET_FN_EMCHG` | `void fn(int)` | Mode change notification |

All callbacks default to NULL and are safely skipped if not set.

## Char Signedness

Plain `char` is signed on x86 and Apple ARM64, but **unsigned** on most ARM
Linux and embedded MCU toolchains. xelp is built and tested both ways
(`make tests-unsigned-char`), so no action is needed to port it.

It matters for your own code: if you store a negative value or a `-1`
sentinel in a plain `char`, it silently becomes `255` on unsigned-`char`
targets and never compares equal to `-1`. This is what broke history recall
in [issue #18](https://github.com/deftio/xelp/issues/18). Use `int` for
state variables and sentinels, or compile with an explicit
`-fsigned-char` / `-funsigned-char` so behavior does not vary by toolchain.

## Platform-Specific Notes

### 8051 (SDCC)

```
sdcc -c xelp.c --model-small
```

xelp automatically adds `__reentrant` qualifiers when compiled with SDCC for
8051 targets. The `REENTRANT_SDCC` macro handles this.

### AVR / Arduino

```
avr-gcc -c xelp.c -Os -mmcu=atmega328p -Isrc
```

Use `-Os` for size optimization. Consider using `PROGMEM` for command name
and help strings to save RAM.

### ARM (bare metal)

```
arm-none-eabi-gcc -c xelp.c -Os -mthumb -mcpu=cortex-m0 -Isrc
```

Thumb mode gives the smallest code. No special considerations needed.

### MSP430

```
msp430-gcc -c xelp.c -Os -mmcu=msp430f2012 -Isrc
```

Watch `int` size (16-bit on MSP430). If you need 32-bit registers, override
`XELPREG` in `xelpcfg.h`:

```c
#define XELPREG long
```

## Cross-Compilation Test

The `tools/compactbuilds-docker.sh` script compiles xelp with various
cross-toolchains inside Docker and reports code sizes in three
configurations (KEY-only, CLI, FULL) grouped by word size. Run via
the crossbuild wrapper:

```bash
bash tools/crossbuild.sh            # builds Docker image (first time) and runs
bash tools/crossbuild.sh --build    # force rebuild the Docker image
```

## Minimal Example

```c
#include "xelp.h"

/* Platform HAL */
void my_putc(char c) { /* ... */ }
void my_bksp(void)   { /* ... */ }

/* Commands */
XELPRESULT cmd_hello(XELP *ths, const char *args, int len) {
    XelpOut(ths, "Hello!\n", 0);
    return XELP_S_OK;
}

XELPCLIFuncMapEntry cmds[] = {
    {&cmd_hello, "hello", "say hello"},
    XELP_FUNC_ENTRY_LAST
};

XELP x;

void main(void) {
    XelpInit(&x, "My Device v1.0");
    XELP_SET_FN_OUT(x, &my_putc);
    XELP_SET_FN_BKSP(x, &my_bksp);
    XELP_SET_FN_CLI(x, cmds);

    for (;;) {
        if (char_available())
            XelpParseKey(&x, get_char());
    }
}
```

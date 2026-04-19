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

Feed received characters one at a time via `XELPParseKey`:

```c
/* In your UART ISR or main loop */
void UART_RX_ISR(void) {
    char c = UART_RX_REG;
    XELPParseKey(&myXelp, c);
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

## Optional: Other Callbacks

| Callback | Signature | Purpose |
|----------|-----------|---------|
| `XELP_SET_FN_ERR` | `void fn(char)` | Error output (separate channel) |
| `XELP_SET_FN_THR` | `void fn(char)` | Pass-through output (modem, etc.) |
| `XELP_SET_FN_EMCHG` | `void fn(int)` | Mode change notification |

All callbacks default to NULL and are safely skipped if not set.

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

### 6502 (cc65)

```
cc65 -t none -Os xelp.c
```

Extremely constrained. Use KEY-only mode to minimize footprint.

## Cross-Compilation Test

The `tools/compactbuilds.sh` script compiles xelp with various
cross-toolchains and reports code sizes. Run it to verify your toolchain
produces a clean build:

```
bash tools/compactbuilds.sh
```

## Minimal Example

```c
#include "xelp.h"

/* Platform HAL */
void my_putc(char c) { /* ... */ }
void my_bksp(void)   { /* ... */ }

/* Commands */
XELPRESULT cmd_hello(XELP *ths, const char *args, int len) {
    XELPOut(ths, "Hello!\n", 0);
    return XELP_S_OK;
}

XELPCLIFuncMapEntry cmds[] = {
    {&cmd_hello, "hello", "say hello"},
    {0, 0, 0}
};

XELP x;

void main(void) {
    XELPInit(&x, "My Device v1.0");
    XELP_SET_FN_OUT(x, &my_putc);
    XELP_SET_FN_BKSP(x, &my_bksp);
    XELP_SET_FN_CLI(x, cmds);

    for (;;) {
        if (char_available())
            XELPParseKey(&x, get_char());
    }
}
```

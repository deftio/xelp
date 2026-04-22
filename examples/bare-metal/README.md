# Bare Metal

Minimal porting template for any microcontroller. Shows all three xelp
modes (CLI, KEY, THR), the mode-change callback, and the platform
abstraction stubs you need to fill in. No OS, no stdlib, no external
dependencies beyond a UART.

## How to Use

1. Copy `bare-metal-example.c` into your project.
2. Copy (or symlink) `xelp.c`, `xelp.h`, and `xelpcfg.h` from `src/`.
3. Replace the four UART stubs with your hardware-specific code:

```c
static void uart_putc(char c)   { /* write c to UART TX register */ }
static void uart_bksp(void)     { uart_putc('\b'); uart_putc(' '); uart_putc('\b'); }
static int  uart_rx_ready(void) { /* return nonzero if a byte is available */ }
static char uart_getc(void)     { /* return the received byte */ }
```

4. Add your own commands to the `cli_commands[]` and `key_commands[]`
   tables.
5. Build with your toolchain. xelp is pure C99 with no stdlib
   dependencies.

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `echo <args>` | Echo arguments back |
| `info` | Show system info |

Single-key mode (press ESC to enter): `?` = help, `b` = print banner.

## What It Demonstrates

- **Platform abstraction** -- the 5 function pointers (output, error,
  backspace, pass-through, mode-change) that form xelp's entire HAL.
  Only output is required; the rest are optional.
- **CLI + KEY + THR modes** -- all three entry modes wired up
- **Mode switching** -- ESC enters KEY mode, CTRL-P returns to CLI,
  CTRL-T enters THR (pass-through)
- **Zero dependencies** -- no malloc, no printf, no OS calls

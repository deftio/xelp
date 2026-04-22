# Multi-Instance

Two completely independent xelp CLIs running on separate serial ports.
Each has its own prompt, command table, output function, and internal
state. Demonstrates that xelp uses no global variables.

## How to Use

1. Copy `multi-instance-example.c` into your project.
2. Copy (or symlink) `xelp.c`, `xelp.h`, and `xelpcfg.h` from `src/`.
3. Replace the UART stubs (`uart0_putc`, `uart1_putc`, etc.) with your
   hardware-specific code.
4. Build with your toolchain.

## What It Demonstrates

- **Two independent XELP instances** (`cli_a` and `cli_b`) each with
  their own output function, backspace handler, command table, and prompt
- **Shared command functions** -- the same C function can serve both
  instances because it receives the `XELP*` pointer for the calling
  instance, so output is routed correctly
- **Per-instance prompts** -- `dbg>` on UART0, `svc>` on UART1

## Use Cases

- Debug console on one UART, production interface on another
- USB CDC + hardware UART running independent command sets
- Multiple BLE characteristics each with their own CLI
- WiFi telnet server + local serial port

## Architecture

```
UART0 rx  ──>  XELPParseKey(&cli_a, c)  ──>  uart0_putc()
UART1 rx  ──>  XELPParseKey(&cli_b, c)  ──>  uart1_putc()
```

Each instance maintains its own line buffer, mode state (CLI/KEY/THR),
and register file (R0-R3). No interaction between instances unless your
command functions explicitly reference each other.

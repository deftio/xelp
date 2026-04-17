# Examples

## posix-simple

Interactive CLI demo using ncurses for terminal handling on Linux/macOS.
Demonstrates CLI commands, KEY mode, THR mode, math operations, and
token parsing.

```bash
make example    # from repo root
```

Requires ncurses: `sudo apt-get install libncurses5-dev` (Linux).

## bare-metal

Minimal template for porting xelp to a new microcontroller. Shows all
three modes (CLI, KEY, THR), the mode change callback, and the platform
abstraction layer stubs you need to fill in. No dependencies beyond a
UART.

## multi-instance

Two independent xelp instances on separate UARTs, each with their own
prompt, command table, and state. Demonstrates that xelp uses no globals.

## arduino

ESP32/Arduino example with NeoPixel LED control via CLI commands. Uses
Arduino `Serial` for I/O.

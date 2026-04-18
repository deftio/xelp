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

## scripting

Demonstrates the difference between **scripting mode** (`XELPParse` /
`XELPParseXB` -- execute a buffer of commands at once) and **interactive
mode** (`XELPParseKey` -- character-by-character terminal input). Shows
startup scripts, one-liner macros with semicolons, and the `XelpBuf` API.

```bash
gcc -Wall -Isrc examples/scripting/scripting-example.c src/xelp.c -o scripting-example
./scripting-example
```

## arduino

ESP32/Arduino example with NeoPixel LED control via CLI commands. Uses
Arduino `Serial` for I/O.

## arduino-cpp

Same idea as the `arduino` example but uses the `XelpCLI` C++ wrapper
class from `src/XelpArduino.h`. Eliminates boilerplate: no manual
`XELP_SET_FN_*` macros, no `Serial.available()` loop -- just `begin()`,
`setCommands()`, and `poll(Serial)`. Also demonstrates `run()` for
executing a startup script.

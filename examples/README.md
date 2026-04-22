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

Basic xelp example using the raw C API. Works on any Arduino board
with a Serial port -- LED control, token listing, and built-in help.
No external library dependencies.

## arduino-cpp

Same idea as the `arduino` example but uses the `XelpCLI` C++ wrapper
class from `src/XelpArduino.h`. Eliminates boilerplate: no manual
`XELP_SET_FN_*` macros, no `Serial.available()` loop -- just `begin()`,
`setCommands()`, and `poll(Serial)`. Also demonstrates `run()` for
executing a startup script, and the `r0()`-`r3()` register accessors
for reading command return values (divmod example).

## esp32-wifi

ESP32 WiFi example using the C++ wrapper. Configure WiFi credentials
over the serial CLI, then fetch the current time and weather from free
APIs (worldtimeapi.org and open-meteo.com). No API keys needed.

Requires an ESP32 board and the ESP32 Arduino core.

## pico-cli

Pure C example for Raspberry Pi Pico / Pico W / Pico 2 using the
Pico SDK. Controls GPIO, ADC, and PWM over USB CDC serial. Automatically
handles Pico W LED (CYW43 driver). See `pico-cli/README.md` for build
instructions.

## pico-cli-arduino

Raspberry Pi Pico example using the **C++ Easy API**. Commands are
registered with `commands({...})` using inline lambdas -- no static
tables, no raw `XELP*` pointers. Uses the Arduino-Pico core by Earle
Philhower. See `pico-cli-arduino/README.md` for details.

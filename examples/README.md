# Examples

## posix-simple

Interactive CLI demo using ncurses for terminal handling on Linux/macOS.
Demonstrates CLI commands, KEY mode, THR mode, command history (UP/DOWN
arrow recall), line editing, math operations, and token parsing.

```bash
cd posix-simple && make
```

Requires ncurses: `sudo apt-get install libncurses5-dev` (Linux).

## posix-simple-cpp

Same functionality as `posix-simple` but written in C++ using the `XelpCLI`
wrapper and **Easy API** (`commands({...})` with inline lambdas). No static
function tables, no raw `XELP*` pointers. Requires C++17.

```bash
cd posix-simple-cpp && make
```

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
mode** (`XelpParseKey` -- character-by-character terminal input). Shows
startup scripts, one-liner macros with semicolons, and the `XelpBuf` API.

```bash
cd scripting && make
```

## arduino

Raw C API with XelpBuf2Argv for argc/argv-style argument parsing.
Demonstrates echo (argv iteration), LED control (`XelpArgvInt`), and
divmod with register returns (R1/R2). Includes a startup script via
`XelpParse`. Works on any Arduino board with a Serial port.

## arduino-cpp

C++ Easy API with inline lambda commands (`commands({...})`). Uses
`XelpCLI::argInt()` for const-correct argv parsing, `r1()`/`r2()`
register accessors for reading command return values, and `run()` for
startup scripts. No static function tables, no raw `XELP*` pointers
-- just `begin()`, `commands({...})`, and `poll(Serial)`.

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

# Arduino C++ Wrapper

Uses the `XelpCLI` C++ wrapper class from `src/XelpArduino.h` with the
C-style static command table. Eliminates boilerplate: no manual
`XELP_SET_FN_*` macros, no `Serial.available()` loop -- just `begin()`,
`setCommands()`, and `poll(Serial)`.

## Requirements

- Any Arduino board with a Serial port (Uno, Mega, ESP32, RP2040, etc.)
- xelp installed via Arduino Library Manager or symlinked from `src/`

## Setup

1. Open `arduino-cpp.ino` in the Arduino IDE.
2. Select your board and port.
3. Upload and open the Serial Monitor at **115200 baud**.

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `echo <args>` | Echo arguments back |
| `led <0\|1>` | Turn on-board LED on/off |
| `divmod <a> <b>` | Integer division (R1 = quotient, R2 = remainder) |
| `pr` | Print all registers (R0-R3) |

## What It Demonstrates

- **`begin()` + `setCommands()` + `setPrompt()`** -- the C-way setup
  using a static `XELPCLIFuncMapEntry[]` array
- **`poll(Serial)`** in `loop()` -- replaces the manual
  `Serial.available()` / `Serial.read()` / `XELPParseKey()` pattern
- **`run()`** -- execute a startup script at boot
- **Register accessors** -- `r0()` through `r3()` for reading command
  return values (shown with the `divmod` command)

For the Easy API approach (lambdas, no static tables), see the
[pico-cli-arduino](../pico-cli-arduino/) example.

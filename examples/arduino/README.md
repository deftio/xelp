# Arduino (Raw C API)

Basic xelp example using the raw C API directly (no C++ wrapper). Works
on any Arduino board with a Serial port -- LED control, token listing,
and built-in help. No external library dependencies.

## Requirements

- Any Arduino board (Uno, Mega, Nano, ESP32, RP2040, etc.)
- xelp installed via Arduino Library Manager or symlinked from `src/`

## Setup

### Arduino IDE

1. Open `arduino.ino` in the Arduino IDE.
2. Select your board and port.
3. Upload and open the Serial Monitor at **115200 baud**.
4. Type `help` and press ENTER.

### arduino-cli

```bash
# List connected boards to find your port and FQBN
arduino-cli board list

# Compile (replace FQBN with your board)
arduino-cli compile --fqbn arduino:avr:uno examples/arduino

# Upload
arduino-cli upload --fqbn arduino:avr:uno -p /dev/ttyACM0 examples/arduino

# Serial monitor
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
```

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `banner` | Print xelp ASCII art |
| `led <0\|1>` | Turn on-board LED on/off |
| `lt <args>` | List parsed tokens |

## What It Demonstrates

- Direct use of `XelpInit()`, `XELP_SET_FN_OUT()`, `XELP_SET_FN_CLI()`
- Static `XELPCLIFuncMapEntry[]` command table with sentinel
- Manual `Serial.available()` / `XelpParseKey()` loop
- Native `argc`/`argv` argument handling in command handlers

For the C++ wrapper approach, see [arduino-cpp](../arduino-cpp/).
For the Easy API with lambdas, see [pico-cli-arduino](../pico-cli-arduino/).

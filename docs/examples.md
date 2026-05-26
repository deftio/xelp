# Examples

Annotated examples showing xelp on different platforms and use cases. All
example source code is in the `examples/` directory of the repository.

## Overview

| Example | Platform | What it demonstrates |
|---------|----------|---------------------|
| [Bare metal](#bare-metal) | Any MCU | Minimal porting template with all three modes |
| [Multi-instance](#multi-instance) | Any MCU | Two independent CLIs on separate UARTs |
| [Posix simple](#posix-simple) | Linux / macOS | Full interactive demo with ncurses |
| [Arduino](#arduino) | Any Arduino board | LED control, token listing via CLI |
| [Arduino C++](#arduino-cpp) | Any Arduino board | C++ wrapper class for easy integration |
| [Arduino Live CLI](#arduino-live-cli) | Any Arduino board | Full hardware CLI: GPIO, ADC, PWM, tone, pulse |
| [ESP32 WiFi](#esp32-wifi) | ESP32 | WiFi config, time and weather fetch via CLI |
| [ESP32-C6 WiFi + BLE](#esp32c6-wifi) | ESP32-C6 | Dual-instance Serial + BLE with WiFi |
| [ESP32 BLE CLI](#esp32-ble-cli) | Any ESP32 with BLE | Dual-instance Serial + BLE, Web Bluetooth terminal |
| [Pico CLI (C)](#pico-cli) | Raspberry Pi Pico | GPIO, ADC, PWM over USB serial (Pico SDK) |
| [Pico CLI (Arduino)](#pico-cli-arduino) | Raspberry Pi Pico | C++ Easy API with lambdas |
| [Scripting](#scripting) | Linux / macOS | Batch scripting vs interactive mode |

## Bare metal

**File:** `examples/bare-metal/bare-metal-example.c`

The starting point for any new port. This example shows the complete
xelp integration pattern with no dependencies beyond a UART:

```c
#include "xelp.h"

/* 1. Platform stubs -- replace with your hardware */
static void uart_putc(char c)  { /* UART TX */ }
static int  uart_rx_ready(void) { return 0; }
static char uart_getc(void)     { return 0; }

/* 2. KEY mode commands */
XELPKeyFuncMapEntry key_commands[] = {
    { &key_help,   '?', "show help"    },
    { &key_banner, 'b', "print banner" },
    XELP_FUNC_ENTRY_LAST
};

/* 3. CLI mode commands */
XELPCLIFuncMapEntry cli_commands[] = {
    { &cmd_help, "help", "show help"        },
    { &cmd_echo, "echo", "echo args back"   },
    { &cmd_info, "info", "show system info" },
    XELP_FUNC_ENTRY_LAST
};

/* 4. Wire everything up */
XELP cli;

void main(void) {
    XelpInit(&cli, "My Device v1.0\n");

    XELP_SET_FN_OUT(cli, &uart_putc);
    XELP_SET_FN_KEY(cli, key_commands);
    XELP_SET_FN_CLI(cli, cli_commands);
    XELP_SET_FN_THR(cli, &thr_passthrough);      /* optional */
    XELP_SET_FN_EMCHG(cli, &on_mode_change);     /* optional */

    XelpHelp(&cli);
    XelpParseKey(&cli, '\n');   /* show initial prompt */

    for (;;) {
        if (uart_rx_ready())
            XelpParseKey(&cli, uart_getc());
    }
}
```

### Key points

- **4 function pointers** are the entire platform abstraction layer:
  output, error, pass-through, mode change. Only output is required.
- Command tables are NULL-terminated arrays. Use `XELP_FUNC_ENTRY_LAST`
  as the sentinel.
- The main loop just feeds characters. xelp handles line buffering,
  backspace, mode switching, and command dispatch internally.

## Multi-instance

**File:** `examples/multi-instance/multi-instance-example.c`

Demonstrates running two completely independent xelp CLIs on separate
serial ports. Each instance has its own:

- Output function (different UART)
- Command table (can be the same or different)
- Prompt string
- Internal state (mode, buffer, registers)

```c
XELP debug_cli;
XELP field_cli;

XelpInit(&debug_cli, "Debug Console (UART0)");
XelpInit(&field_cli, "Service Port (UART1)");

XELP_SET_FN_OUT(debug_cli, &uart0_putc);
XELP_SET_FN_OUT(field_cli, &uart1_putc);

XELP_SET_FN_CLI(debug_cli, debug_commands);
XELP_SET_FN_CLI(field_cli, field_commands);

XELP_SET_VAL_CLI_PROMPT(debug_cli, "dbg>");
XELP_SET_VAL_CLI_PROMPT(field_cli, "svc>");

/* Main loop -- poll both UARTs */
for (;;) {
    if (uart0_rx_ready()) XelpParseKey(&debug_cli, uart0_getc());
    if (uart1_rx_ready()) XelpParseKey(&field_cli, uart1_getc());
}
```

### Use cases

- Debug console on one UART, production interface on another
- Multiple BLE characteristics each with their own CLI
- USB CDC + UART running independent command sets

## Posix simple

**File:** `examples/posix-simple/xelp-example.c`

A full interactive demo for Linux and macOS using ncurses for terminal
handling. Build and run from the repo root:

```bash
make example
```

Requires ncurses (`sudo apt-get install libncurses5-dev` on Debian/Ubuntu).

### Features demonstrated

- CLI commands: `echo`, `banner`, `help`, `numtoks`, `lt` (list tokens),
  math operators (`+`, `-`, `*`, `/`), `exit`
- KEY mode commands: `h` (help), `f` (fooBar), `p`/`w` (print), `b`
  (banner), `x` (exit)
- Mode switching: ESC for KEY mode, CTRL-P for CLI, CTRL-T for THR
- Mode change callback showing mode transitions
- Token parsing and numeric argument handling
- Command history: UP/DOWN arrow recall of previous commands

### Architecture

```
stdin (ncurses getch)
  |
  v
XelpParseKey(&example, char)
  |
  +---> CLI mode: line buffer -> tokenize -> dispatch
  +---> KEY mode: immediate dispatch
  +---> THR mode: pass-through function
  |
  v
gPutChar() -> ncurses addch() -> terminal
```

## Arduino

**File:** `examples/arduino/arduino.ino`

Basic example using the raw C API. Works on any Arduino board with a
Serial port -- LED control, token listing, and built-in help. No
external library dependencies.

```c
void writeChar(char c) { Serial.write(c); }

void setup() {
    Serial.begin(115200);
    XelpInit(&cli, "xelp Arduino example v1.0\n");
    XELP_SET_FN_OUT(cli, &writeChar);
    XELP_SET_FN_CLI(cli, gMyCLICommands);
}

void loop() {
    if (Serial.available() > 0) {
        char c = Serial.read();
        XelpParseKey(&cli, c);
    }
}
```

### Commands

- `help` -- list all commands
- `banner` -- print xelp ASCII art
- `led <0|1>` -- toggle LED
- `lt <args>` -- list parsed tokens (debugging tool)

## Arduino C++

**File:** `examples/arduino-cpp/arduino-cpp.ino`

Same idea as the `arduino` example but uses the `XelpCLI` C++ wrapper
class from `src/XelpArduino.h`. Eliminates boilerplate: no manual
`XELP_SET_FN_*` macros, no `Serial.available()` loop -- just `begin()`,
`setCommands()`, and `poll(Serial)`.

## ESP32 WiFi

**File:** `examples/esp32-wifi/esp32-wifi.ino`

ESP32 WiFi example using the C++ wrapper. Configure WiFi credentials
over the serial CLI, then fetch the current time and weather from free
APIs (worldtimeapi.org and open-meteo.com). No API keys needed.

### Commands

- `ssid <name>` / `pass <password>` -- set WiFi credentials
- `connect` / `disconnect` -- manage WiFi connection
- `status` -- show WiFi status, IP, RSSI
- `time` -- fetch current time
- `weather <lat> <lon>` -- fetch weather for coordinates

## Arduino Live CLI

**File:** `examples/arduino-live-cli/arduino-live-cli.ino`

Full interactive hardware CLI using the C++ wrapper. Over 15 commands for
GPIO, ADC, PWM, tone generation, pulse width measurement, and pin scanning.
Works on any Arduino-compatible board (AVR, ARM, ESP32, RP2040).

### Commands

- `help` / `?` -- list all commands
- `setpin <pin> <0|1>` / `getpin <pin>` -- digital I/O
- `pinmode <pin> <in|out|pullup>` -- configure direction
- `setpwm <pin> <0-255>` / `readadc <pin>` -- analog I/O
- `tone <pin> <freq>` / `notone <pin>` -- tone generation
- `pulsein <pin>` -- measure pulse width
- `scanpins <start> <end>` -- scan GPIO states
- `reboot` -- software reset

## ESP32-C6 WiFi + BLE

**Files:** `examples/esp32c6-wifi/`

Dual-instance demo for Seeed XIAO ESP32-C6: one CLI on USB Serial, one on
BLE (Nordic UART Service). WiFi commands for scanning and connecting.
Includes Web Bluetooth terminal in `web/index.html`.

## ESP32 BLE CLI

**Files:** `examples/esp32-ble-cli/`

Dual-instance CLI for any BLE-capable ESP32. One xelp instance on USB
Serial, one on BLE (Nordic UART Service), both sharing the same command
table. Demonstrates zero-dynamic-memory multi-instance CLI over two
different transports with cross-instance messaging.

### Key features

- Drip-feed BLE notification pacing (one 20-byte chunk per connection interval)
- Cross-instance messaging (`sendmsg serial|ble <text>`)
- RGB NeoPixel LED support with automatic LDO2 power enable
- Web Bluetooth terminal (`web/index.html`) for Chrome/Edge
- App version tracking in `info` command

### Commands

- `help` / `?`, `banner`, `echo`, `info`
- `led <0|1>`, `rgb <r> <g> <b>` -- LED control
- `setpin`, `getpin`, `pinmode`, `setpwm`, `readadc` -- GPIO/analog
- `status`, `sendmsg <serial|ble> <text>`, `reboot`

## Pico CLI (C)

**Files:** `examples/pico-cli/`

Pure C example for Raspberry Pi Pico / Pico W / Pico 2 using the Pico SDK.
Controls GPIO, ADC, and PWM over USB CDC serial. Automatically detects Pico W
boards and uses the CYW43 driver for the wireless-chip LED.

```bash
mkdir build && cd build
cmake -DPICO_BOARD=pico ..   # or pico_w, pico2, pico2_w
make
```

## Pico CLI (Arduino)

**Files:** `examples/pico-cli-arduino/`

Showcases the C++ Easy API on Raspberry Pi Pico using the Arduino-Pico core.
Commands registered with `commands({...})` -- no static tables, no raw `XELP*`
pointers. Lambda callbacks receive `XelpCLI&`, `argc`, and `argv`.

## Scripting

**File:** `examples/scripting/scripting-example.c`

Demonstrates the difference between **scripting mode** (`XelpParse` /
`XelpParseXB` -- execute a buffer of commands at once) and **interactive
mode** (`XelpParseKey` -- character-by-character terminal input).

```bash
gcc -Wall -Isrc examples/scripting/scripting-example.c src/xelp.c -o scripting-example
./scripting-example
```

### Notes

- On Arduino, `Serial.write()` is your output function
- Use `XELP_SET_VAL_CLI_PROMPT` for a custom prompt

## Writing your own commands

All CLI commands have the same signature (since v0.4.0):

```c
XELPRESULT my_command(XELP *ths, int argc, const char **argv);
```

- `ths` is a pointer to the XELP instance that dispatched the command
- `argc` is the number of tokens (including the command name)
- `argv` is an array of null-terminated token strings (`argv[0]` is the
  command name, `argv[1]` through `argv[argc-1]` are the arguments)
- Return `XELP_S_OK` (0) for success, negative for error, positive for warning

The dispatch engine tokenizes the command line, strips quotes, expands
escape sequences, and null-terminates each token before calling your
handler. No manual tokenization is needed.

### Pattern: native argc/argv (standard)

Handlers receive `argc` and `argv` directly -- just like C `main()`.
Use `argv[n]` to access any argument by index:

```c
XELPRESULT cmd_set(XELP *ths, int argc, const char **argv) {
    /* argv[0]="set", argv[1]=key, argv[2]=value (null-terminated) */
    int val;
    if (XelpArgvInt(argv, argc, 2, &val) != XELP_S_OK)
        return XELP_E_ERR;
    /* ... use argv[1] as key string, val as integer ... */
    return XELP_S_OK;
}
```

### Pattern: bounds-checked access with XelpArgvInt / XelpArgvStr

`XelpArgvInt` and `XelpArgvStr` are convenience helpers that perform
bounds checking on the index and type conversion. They return
`XELP_E_ERR` if the index is out of range or the value is not numeric:

```c
XELPRESULT cmd_set(XELP *ths, int argc, const char **argv) {
    const char *key;
    int keylen, value;

    if (XelpArgvStr(argv, argc, 1, &key, &keylen) != XELP_S_OK)
        return XELP_E_ERR;     /* missing key argument */
    if (XelpArgvInt(argv, argc, 2, &value) != XELP_S_OK)
        return XELP_E_ERR;     /* missing or non-numeric value */

    /* do something with key (length keylen) and value */
    return XELP_S_OK;
}
```

### Pattern: random-access by index with argv[]

For random access to any token, use `argv[n]` directly. Each entry is a
null-terminated C string:

```c
XELPRESULT cmd_get(XELP *ths, int argc, const char **argv) {
    if (argc < 2) return XELP_E_ERR;     /* need at least one argument */
    const char *arg = argv[1];            /* first argument after command name */
    /* use arg as a standard null-terminated string */
    return XELP_S_OK;
}
```

### Pattern: output without printf

xelp has no printf. For string output, use `XelpOut`. For numbers, either
use your platform's `sprintf` into a local buffer, or write a simple
int-to-string helper:

```c
void print_int(XELP *x, int val) {
    char buf[12];
    int i = 0;
    if (val < 0) { XelpOut(x, "-", 1); val = -val; }
    if (val == 0) { XelpOut(x, "0", 1); return; }
    while (val > 0) { buf[i++] = '0' + (val % 10); val /= 10; }
    while (i > 0) { XelpOut(x, &buf[--i], 1); }
}
```

## Next steps

- [Tutorial](tutorial.md) -- step-by-step guide to building your first xelp CLI
- [API Reference](api-reference.md) -- complete function documentation
- [Configuration Guide](configuration.md) -- compile-time options
- [Porting Guide](porting.md) -- platform-specific notes

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
| [ESP32 WiFi](#esp32-wifi) | ESP32 | WiFi config, time and weather fetch via CLI |
| [Scripting](#scripting) | Linux / macOS | Batch scripting vs interactive mode |

## Bare metal

**File:** `examples/bare-metal/bare-metal-example.c`

The starting point for any new port. This example shows the complete
xelp integration pattern with no dependencies beyond a UART:

```c
#include "xelp.h"

/* 1. Platform stubs -- replace with your hardware */
static void uart_putc(char c)  { /* UART TX */ }
static void uart_bksp(void)    { uart_putc('\b'); uart_putc(' '); uart_putc('\b'); }
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
    XELPInit(&cli, "My Device v1.0\n");

    XELP_SET_FN_OUT(cli, &uart_putc);
    XELP_SET_FN_BKSP(cli, &uart_bksp);
    XELP_SET_FN_KEY(cli, key_commands);
    XELP_SET_FN_CLI(cli, cli_commands);
    XELP_SET_FN_THR(cli, &thr_passthrough);      /* optional */
    XELP_SET_FN_EMCHG(cli, &on_mode_change);     /* optional */

    XELPHelp(&cli);
    XELPParseKey(&cli, '\n');   /* show initial prompt */

    for (;;) {
        if (uart_rx_ready())
            XELPParseKey(&cli, uart_getc());
    }
}
```

### Key points

- **5 function pointers** are the entire platform abstraction layer:
  output, error, backspace, pass-through, mode change. Only output is
  required.
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

XELPInit(&debug_cli, "Debug Console (UART0)");
XELPInit(&field_cli, "Service Port (UART1)");

XELP_SET_FN_OUT(debug_cli, &uart0_putc);
XELP_SET_FN_OUT(field_cli, &uart1_putc);

XELP_SET_FN_CLI(debug_cli, debug_commands);
XELP_SET_FN_CLI(field_cli, field_commands);

XELP_SET_VAL_CLI_PROMPT(debug_cli, "dbg>");
XELP_SET_VAL_CLI_PROMPT(field_cli, "svc>");

/* Main loop -- poll both UARTs */
for (;;) {
    if (uart0_rx_ready()) XELPParseKey(&debug_cli, uart0_getc());
    if (uart1_rx_ready()) XELPParseKey(&field_cli, uart1_getc());
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
- Backspace handling with ncurses `delch()`
- Mode change callback showing mode transitions
- Token parsing and numeric argument handling

### Architecture

```
stdin (ncurses getch)
  |
  v
XELPParseKey(&example, char)
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
    XELPInit(&cli, "xelp Arduino example v1.0\n");
    XELP_SET_FN_OUT(cli, &writeChar);
    XELP_SET_FN_CLI(cli, gMyCLICommands);
}

void loop() {
    if (Serial.available() > 0) {
        char c = Serial.read();
        XELPParseKey(&cli, c);
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

## Scripting

**File:** `examples/scripting/scripting-example.c`

Demonstrates the difference between **scripting mode** (`XELPParse` /
`XELPParseXB` -- execute a buffer of commands at once) and **interactive
mode** (`XELPParseKey` -- character-by-character terminal input).

```bash
gcc -Wall -Isrc examples/scripting/scripting-example.c src/xelp.c -o scripting-example
./scripting-example
```

### Notes

- On Arduino, `Serial.write()` is your output function
- Use `XELP_SET_VAL_CLI_PROMPT` for a custom prompt

## Writing your own commands

All CLI commands have the same signature:

```c
XELPRESULT my_command(XELP *ths, const char *args, int maxlen);
```

- `ths` is a pointer to the XELP instance that dispatched the command
- `args` points to the full command line (including the command name)
- `maxlen` is the number of valid bytes in `args`
- Return `XELP_S_OK` (0) for success, negative for error, positive for warning

### Pattern: parsing arguments with XelpArgs (preferred)

`XelpArgs` iterates tokens left-to-right in O(1) per token:

```c
XELPRESULT cmd_set(XELP *ths, const char *args, int len) {
    XelpArgs a;
    XelpBuf key;
    int value;

    XelpArgsInit(&a, args, len);
    XelpNextTok(&a, 0);              /* skip command name ("set") */
    XelpNextTok(&a, &key);           /* key: use key.s .. key.p */
    XelpNextInt(&a, &value);

    /* do something with key and value */
    return XELP_S_OK;
}
```

### Pattern: random-access by index (XELPTokN)

For random access to any token by index:

```c
XELPRESULT cmd_get(XELP *ths, const char *args, int len) {
    XelpBuf b, tok;
    XELP_XB_INIT(b, (char*)args, len);  /* cast: XELP_XB_INIT takes char* */
    XELPTokN(&b, 1, &tok);              /* 0-indexed: token 1 = first arg */
    /* use tok.s .. tok.p */
    return XELP_S_OK;
}
```

### Pattern: output without printf

xelp has no printf. For string output, use `XELPOut`. For numbers, either
use your platform's `sprintf` into a local buffer, or write a simple
int-to-string helper:

```c
void print_int(XELP *x, int val) {
    char buf[12];
    int i = 0;
    if (val < 0) { XELPOut(x, "-", 1); val = -val; }
    if (val == 0) { XELPOut(x, "0", 1); return; }
    while (val > 0) { buf[i++] = '0' + (val % 10); val /= 10; }
    while (i > 0) { XELPOut(x, &buf[--i], 1); }
}
```

## Next steps

- [Tutorial](tutorial.md) -- step-by-step guide to building your first xelp CLI
- [API Reference](api-reference.md) -- complete function documentation
- [Configuration Guide](configuration.md) -- compile-time options
- [Porting Guide](porting.md) -- platform-specific notes

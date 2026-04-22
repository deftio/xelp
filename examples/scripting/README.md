# Scripting

Demonstrates the difference between **scripting mode** and **interactive
mode** -- the two ways to drive xelp.

## Building and Running

```bash
cd examples/scripting
make          # build and run
make build    # build only
make clean    # remove build artifacts
```

## Two Modes of Operation

### Scripting mode (`XELPParse` / `XELPParseXB`)

Execute a buffer of commands at once. The buffer is const and never
modified, so scripts can live in ROM, flash, EEPROM, or be received
over a network.

```c
const char *script =
    "echo Initializing...\n"
    "led 1\n"
    "gain 75\n"
    "status\n";

XELPParse(&cli, script, XELPStrLen(script));
```

Semicolons work too:

```c
XELPParse(&cli, "echo start; led 1; gain 100; status", ...);
```

### Interactive mode (`XELPParseKey`)

Process one character at a time. Handles echoing, backspace, line
editing, and dispatching on ENTER. This is what you use in your main
loop for serial/USB input from a human.

```c
for (;;) {
    if (uart_rx_ready())
        XELPParseKey(&cli, uart_getc());
}
```

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `echo <args>` | Echo arguments |
| `led <0\|1>` | Set LED state |
| `gain <N>` | Set gain value |
| `status` | Show current LED and gain state |

## What It Demonstrates

- **Startup scripts** stored as const strings (ROM/flash safe)
- **One-liner macros** using semicolons as separators
- **`XELPParseXB`** for driving xelp from a `XelpBuf` struct
- **Simulated interactive typing** using `XELPParseKey` in a loop
- Both modes use the same command table and can be mixed on the same
  instance

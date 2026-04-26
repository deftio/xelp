# Posix Simple

Full interactive xelp demo for Linux and macOS using ncurses for
terminal handling. This is the kitchen-sink example that exercises most
of xelp's features.

## Requirements

- Linux or macOS
- ncurses development library

```bash
# Debian / Ubuntu
sudo apt-get install libncurses5-dev

# macOS (ships with ncurses)
# Nothing to install
```

## Building and Running

```bash
cd examples/posix-simple
make          # build and run
make build    # build only
make clean    # remove build artifacts
```

## Commands

**CLI mode** (type command + ENTER):

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `echo <args>` | Print arguments |
| `banner` | Print xelp ASCII art |
| `numtoks <args>` | Count tokens |
| `lt <args>` | List parsed tokens |
| `+ <a> <b>` | Add two numbers |
| `- <a> <b>` | Subtract |
| `* <a> <b>` | Multiply |
| `/ <a> <b>` | Divide |
| `divmod <a> <b>` | Division with remainder (R1, R2) |
| `pr` | Print all registers |
| `exit` | Quit |

**KEY mode** (single keypress, no ENTER):

| Key | Description |
|-----|-------------|
| `h` | Show help |
| `f` | fooBar demo |
| `p` / `w` | Print demo |
| `b` | Print banner |
| `x` | Exit |

## Mode Switching

| Key | Action |
|-----|--------|
| ESC | Enter KEY mode |
| CTRL-P | Enter CLI mode |
| CTRL-T | Enter THR (pass-through) mode |

## What It Demonstrates

- All three xelp modes (CLI, KEY, THR) with mode-change callbacks
- Backspace handling via ncurses
- Token parsing and numeric arguments
- Math operations dispatched to the same handler function
- Register file (R0-R3) for command return values

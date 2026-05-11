# Pico CLI Arduino (Easy API)

Raspberry Pi Pico example using the **C++ Easy API**. Commands are
registered with `commands({...})` using inline lambdas -- no static
command tables, no raw `XELP*` pointers.

## Requirements

- [Arduino-Pico core](https://github.com/earlephilhower/arduino-pico)
  by Earle Philhower (install via Board Manager)
- Raspberry Pi Pico, Pico W, Pico 2, or Pico 2 W

## Setup

Copy or symlink xelp source files into this sketch directory:

```bash
ln -s ../../src/xelp.c .
ln -s ../../src/xelp.h .
ln -s ../../src/xelpcfg.h .
ln -s ../../src/XelpArduino.h .
```

### Arduino IDE

Open `pico-cli-arduino.ino` in the Arduino IDE and upload.

### arduino-cli

```bash
# List connected boards to find your port and FQBN
arduino-cli board list

# Compile (replace FQBN with your board)
arduino-cli compile --fqbn rp2040:rp2040:rpipico examples/pico-cli-arduino

# Upload
arduino-cli upload --fqbn rp2040:rp2040:rpipico -p /dev/ttyACM0 examples/pico-cli-arduino

# Serial monitor
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
```

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `led <0\|1>` | Turn on-board LED on/off |
| `adc <pin>` | Read analog pin |
| `pwm <pin> <0-255>` | Set PWM duty cycle |
| `pin <n> <in\|out>` | Configure GPIO direction |
| `set <pin> <0\|1>` | Set GPIO output level |
| `get <pin>` | Read GPIO input level |

Single-key mode (press ESC to enter): `?` = help, `l` = toggle LED.

## How It Works

The Easy API lets you define your entire CLI in one block:

```cpp
cli.commands({
    {"led", "led <0|1>", [](XelpCLI& c, int argc, const char** argv) {
        if (argc < 2) { c.print("usage: led <0|1>\n"); return; }
        int v = argv[1][0] - '0';
        digitalWrite(LED_BUILTIN, v ? HIGH : LOW);
        c.print(v ? "LED ON\n" : "LED OFF\n");
    }},
});
```

Behind the scenes, xelp stores the callbacks in a fixed-capacity internal
array and uses a shared dispatcher to parse `argv` and call your lambda.
No heap allocation is used.

# arduino-live-cli

Interactive hardware CLI for Arduino boards.  Drive pins, read sensors,
and play tones from any serial terminal.

## Requirements

- Any Arduino board with a Serial port (Uno, Nano, Mega 2560, Leonardo,
  Raspberry Pi Pico / Pico 2, ESP32, etc.)
- Arduino IDE 1.8+, Arduino Cloud, arduino-cli, or PlatformIO

## Quick start (Arduino IDE)

1. Open `arduino-live-cli.ino` in the Arduino IDE
2. Select your board and port
3. Upload
4. Open Serial Monitor at **115200 baud**
5. Type `help` (or just `?`)

## Quick start (arduino-cli)

```bash
# Compile (substitute the FQBN for your board):
arduino-cli compile --fqbn rp2040:rp2040:rpipico2 --library path/to/xelp/src .

# Upload via serial port:
arduino-cli upload --fqbn rp2040:rp2040:rpipico2 -p /dev/ttyACM0 .

# Or for RP2040/RP2350 boards: copy the UF2 file directly.
# Hold BOOTSEL, plug in USB, then:
cp build/rp2040.rp2040.rpipico2/arduino-live-cli.ino.uf2 /Volumes/RP2350/
```

Common FQBNs:

| Board | FQBN |
|-------|------|
| Raspberry Pi Pico 2 | `rp2040:rp2040:rpipico2` |
| Raspberry Pi Pico | `rp2040:rp2040:rpipico` |
| Arduino Mega 2560 | `arduino:avr:mega` |
| Arduino Uno | `arduino:avr:uno` |
| ESP32 Dev Module | `esp32:esp32:esp32` |

## Quick start (PlatformIO)

Install xelp from the PlatformIO registry (`pio pkg install -l xelp`),
copy `arduino-live-cli.ino` into your project's `src/` directory, and
build normally with `pio run`.

## Commands

| Command | Usage | Description |
|---------|-------|-------------|
| `help` | `help` | Show all commands |
| `?` | `?` | Same as help |
| `banner` | `banner` | Print xelp ASCII art and welcome message |
| `echo` | `echo <args>` | Echo arguments back |
| `info` | `info` | Board type, free SRAM, uptime, xelp version |
| `setpin` | `setpin <pin> <0\|1>` | `digitalWrite` -- set HIGH or LOW |
| `getpin` | `getpin <pin>` | `digitalRead` -- print 0 or 1 |
| `pinmode` | `pinmode <pin> <in\|out\|pullup>` | Configure pin direction |
| `setpwm` | `setpwm <pin> <0-255>` | `analogWrite` -- set PWM duty cycle |
| `readadc` | `readadc <pin>` | `analogRead` -- print 0-1023 |
| `tone` | `tone <pin> <hz> [ms]` | Play a frequency on a buzzer/speaker * |
| `notone` | `notone <pin>` | Stop tone output * |
| `pulsein` | `pulsein <pin> <high\|low> [timeout]` | Measure pulse width (microseconds) * |
| `delay` | `delay <ms>` | Pause for N milliseconds |
| `millis` | `millis` | Print uptime in milliseconds |
| `micros` | `micros` | Print uptime in microseconds * |
| `scanpins` | `scanpins [first] [last]` | Read all digital pins in range * |
| `demo-blink3` | `demo-blink3` | Blink LED 3x -- scripting demo * |
| `demo-scan` | `demo-scan` | Configure + scan pins -- scripting demo * |
| `demo-info` | `demo-info` | Chain info commands -- scripting demo * |
| `reboot` | `reboot` | Software reset (AVR / ESP32) |

Commands marked with **\*** are excluded on ATmega328P/168 boards (see
notes below).

Unrecognized commands print an error: `foo: unknown command`.

## Command history

Use the up/down arrow keys to recall the last 4 commands.

## Scripting

xelp supports semicolon-separated command scripts.  Blink the onboard
LED from a one-liner:

```
pinmode 13 out; setpin 13 1; delay 500; setpin 13 0; delay 500; setpin 13 1
```

The `demo-*` commands show this in action -- type `demo-blink3` to see
the script printed and then executed.

## Wiring

No external wiring required -- the built-in LED on pin 13 works out of
the box.  For the full demo, connect:

- **LED** on any digital pin (with resistor)
- **Buzzer** on a PWM-capable pin (for `tone`)
- **Potentiometer** on an analog pin (for `readadc`)

## Notes

- Pins 0 and 1 are Serial RX/TX.  Commands that modify them print a
  warning but do not block the operation.
- `reboot` uses a watchdog reset on AVR and `ESP.restart()` on ESP32.
  On other platforms it prints an error.
- Free SRAM reporting is AVR-only; other platforms show xelp version
  and uptime only.
- **ATmega328P / ATmega168** (Uno, Nano): a reduced command set is
  compiled automatically (`XELP_SMALL_TARGET`).  Commands marked with
  **\*** in the table above are excluded to fit within 2 KB SRAM.

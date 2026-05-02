# arduino-live-cli

Interactive hardware CLI for Arduino boards.  Drive pins, read sensors,
and play tones from any serial terminal.

## Requirements

- Any Arduino board with a Serial port (Uno, Nano, Mega 2560, Leonardo, etc.)
- Arduino IDE 1.8+ or Arduino Cloud

## Quick start

1. Open `arduino-live-cli.ino` in the Arduino IDE
2. Select your board and port
3. Upload
4. Open Serial Monitor at **115200 baud**
5. Type `help` (or just `?`)

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
| `tone` | `tone <pin> <hz> [ms]` | Play a frequency on a buzzer/speaker |
| `notone` | `notone <pin>` | Stop tone output |
| `pulsein` | `pulsein <pin> <high\|low> [timeout]` | Measure pulse width (microseconds) |
| `delay` | `delay <ms>` | Pause for N milliseconds |
| `millis` | `millis` | Print uptime in milliseconds |
| `micros` | `micros` | Print uptime in microseconds |
| `scanpins` | `scanpins [first] [last]` | Read all digital pins in range |
| `setprompt` | `setprompt <text>` | Change the CLI prompt string |
| `demo-blink3` | `demo-blink3` | Blink LED 3x -- scripting demo |
| `demo-scan` | `demo-scan` | Configure + scan pins -- scripting demo |
| `demo-info` | `demo-info` | Chain info commands -- scripting demo |
| `reboot` | `reboot` | Software reset (AVR / ESP32) |

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

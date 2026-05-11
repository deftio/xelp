# Arduino Mega CLI Example — TODO

Interactive serial CLI demo for Arduino Mega (and compatible boards).
Registers hardware commands via `XelpArduino.h` so users can drive pins,
read sensors, and play tones from any serial terminal.

Target audience: Arduino beginners/intermediate users looking for a
ready-made serial command interface. Doubles as article material for
Arduino forums.

## Commands

| Command | Args | Description |
|---------|------|-------------|
| `setpin` | `<pin> <0\|1>` | `digitalWrite` — set a digital pin HIGH or LOW |
| `getpin` | `<pin>` | `digitalRead` — read a digital pin, print 0 or 1 |
| `pinmode` | `<pin> <in\|out\|pullup>` | `pinMode` — configure pin direction |
| `setpwm` | `<pin> <0-255>` | `analogWrite` — set PWM duty cycle |
| `readadc` | `<pin>` | `analogRead` — read analog pin, print 0-1023 |
| `tone` | `<pin> <freq_hz> [dur_ms]` | `tone()` — play a frequency on a buzzer/speaker |
| `notone` | `<pin>` | `noTone()` — stop tone output |
| `pulsein` | `<pin> <high\|low> [timeout_us]` | `pulseIn()` — measure pulse width in microseconds |
| `delay` | `<ms>` | `delay()` — pause for N milliseconds (useful in scripts) |
| `millis` | — | Print uptime in milliseconds |
| `micros` | — | Print uptime in microseconds |
| `scanpins` | `[first] [last]` | Read all digital + analog pins in range, print table |
| `info` | — | Print board type, SRAM free, uptime, xelp version |
| `help` | — | List available commands (built-in xelp) |
| `reboot` | — | reboots, prints the startup banner |
| 'setprompt' | `string-new prompt` | if easy do this, set the prompt by saving a new global null terminated str (max 32 chars) |

## File Layout

```
examples/
  arduino-live-cli/
    arduino-live-cli.ino    # main sketch
    README.md               # wiring notes, screenshot, usage
```

## Implementation Notes

- Use `XelpArduino.h` C++ wrapper (already in repo)
- Each command is a plain C callback registered with `XelpAddCommand()`
- Argument parsing via `strtol()` / `atoi()` — keep it simple, no extra deps
- Default serial baud: 115200
- Should compile on Mega 2560, Uno, Nano, Leonardo — nothing Mega-specific
  in the API, but Mega has the most pins to play with
- Guard dangerous pins (0, 1 = Serial TX/RX) with a warning, don't block

## Script Demo

Show that xelp can run multi-command scripts over serial:

```
pinmode 13 out; setpin 13 1; delay 500; setpin 13 0; delay 500; setpin 13 1
```

Blink the onboard LED from a one-liner — good for the article.

## Arduino env

- Add board entry to `platformio.ini` (e.g. `megaatmega2560`)
- CI already builds `.ino` examples — just needs the new directory

## BLE Variant (Future)

- Same command set, but over BLE serial (Nano 33 BLE / ESP32)
- Lets you drive pins wirelessly from a phone or laptop terminal
- Separate example directory: `examples/arduino-ble-cli/`
- Depends on ArduinoBLE or ESP32 BLE libraries — different scope

## Article Outline

1. What is xelp — one-paragraph intro
2. Wiring photo / Fritzing (LED + buzzer + potentiometer on Mega)
3. Flash the sketch, open Serial Monitor
4. Walk through commands: read a pot, blink an LED, play a tone
5. Show scripting: one-liner blink sequence
6. Link to repo + Arduino Library Manager install
7. Mention BLE variant as teaser

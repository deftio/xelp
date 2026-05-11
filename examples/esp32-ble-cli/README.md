# ESP32 BLE CLI Example

Two independent xelp CLI instances on any BLE-capable ESP32: one on USB
Serial, one on BLE (Nordic UART Service).  Both share the same command
table and can drive GPIO pins, read ADC, and control the board -- all
with zero dynamic memory allocation.

## Hardware

- Any ESP32 with BLE: ESP32, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2
- Built-in LED (`LED_BUILTIN`, falls back to GPIO 2). Boards with RGB
  NeoPixel LEDs (e.g. ProS3, TinyS3, FeatherS3) are auto-detected and use
  `rgbLedWrite()` with LDO2 power enable.
- Optional: external LED, potentiometer, or sensor on any GPIO

## Dependencies

| Dependency | Install |
|------------|---------|
| ESP32 Arduino core | Arduino IDE Board Manager: esp32 by Espressif |
| NimBLE-Arduino | Arduino IDE Library Manager: search "NimBLE-Arduino" by h2zero |

## Partition Scheme

**BLE may exceed the default 1.3 MB app partition on some ESP32 variants.**
If the build fails with "text section exceeds available space", change
the partition scheme:

> **Tools > Partition Scheme > "Huge APP (3MB No OTA/1MB SPIFFS)"**

This is an ESP-IDF / Arduino core limitation, not xelp.  xelp itself
adds ~4-5 KB.  The NimBLE stack accounts for the rest.

## Files

```
esp32-ble-cli/
  esp32-ble-cli.ino    Arduino sketch (main source)
  xelp.c               symlink -> ../../src/xelp.c
  xelp.h               symlink -> ../../src/xelp.h
  xelpcfg.h            symlink -> ../../src/xelpcfg.h
  XelpArduino.h        symlink -> ../../src/XelpArduino.h
  web/
    index.html          Web Bluetooth terminal (open in Chrome/Edge)
```

The symlinks let you develop against the repo's `src/` directly.  Arduino
IDE compiles all `.c` files in the sketch folder automatically.

## Quick Start

### Arduino IDE

1. Open `esp32-ble-cli.ino`
2. Select your ESP32 board under Tools > Board
3. If needed, set Tools > Partition Scheme > "Huge APP (3MB No OTA/1MB SPIFFS)"
4. Install NimBLE-Arduino from Library Manager
5. Upload and open Serial Monitor at 115200 baud

### arduino-cli

```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3 esp32-ble-cli
arduino-cli upload  --fqbn esp32:esp32:esp32s3 -p /dev/ttyUSB0 esp32-ble-cli
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### PlatformIO

```ini
[env:esp32]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
lib_deps = h2zero/NimBLE-Arduino
monitor_speed = 115200
```

## Commands

| Command | Description |
|---------|-------------|
| `help` / `?` | List all commands |
| `banner` | Print xelp banner |
| `echo <args>` | Print arguments |
| `info` | App version, board type, uptime, xelp version |
| `led <0\|1>` | Built-in LED on/off (RGB NeoPixel or GPIO) |
| `rgb <r> <g> <b>` | Set RGB LED color (0-255 each, boards with NeoPixel) |
| `setpin <pin> <0\|1>` | digitalWrite |
| `getpin <pin>` | digitalRead |
| `pinmode <pin> <in\|out\|pullup>` | Set pin mode |
| `setpwm <pin> <0-255>` | analogWrite (PWM) |
| `readadc <pin>` | analogRead |
| `delay <ms>` | Blocking delay |
| `millis` | Uptime in milliseconds |
| `status` | Serial, BLE, LED type, uptime |
| `sendmsg <serial\|ble> <text>` | Send message to the other CLI instance |
| `reboot` | Software reset (ESP.restart) |

Unknown commands print an error message.

## Web Bluetooth Terminal

Open `web/index.html` in Chrome or Edge (Web Bluetooth is not supported
in Firefox or Safari).

1. Click **Connect**
2. Select **xelp-ble** from the pairing dialog
3. Type commands in the input field or use the quick buttons

The page is fully self-contained -- no build step, no server required.

## Serial Usage

```
$ screen /dev/tty.usbmodem* 115200

xelp> help
...
xelp> setpin 2 1
xelp> getpin 2
1
xelp> readadc 34
1823
xelp> status
Serial: connected
BLE:    advertising
LED:    RGB (NeoPixel)
Uptime: 12345 ms
```

## Scripting

Commands can be chained with semicolons:

```
xelp> pinmode 2 out; setpin 2 1; delay 500; setpin 2 0
```

This works identically over both Serial and BLE.

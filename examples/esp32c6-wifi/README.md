# ESP32-C6 Dual-CLI Example

Two independent xelp CLI instances on a Seeed XIAO ESP32-C6: one on USB
Serial, one on BLE (Nordic UART Service). Both share the same command
table and can provision WiFi credentials, toggle the LED, and read ADC
voltage.

## Hardware

- Seeed XIAO ESP32-C6
- LED on `LED_BUILTIN` (GPIO15)
- A0 analog input (assumes external 1:2 voltage divider for battery monitoring)

## Dependencies

| Dependency | Install |
|------------|---------|
| ESP32 Arduino core | Arduino IDE Board Manager → esp32 by Espressif |
| NimBLE-Arduino | Arduino IDE Library Manager → search "NimBLE-Arduino" by h2zero |

## Partition Scheme (required)

**WiFi + BLE together exceed the default 1.3 MB app partition.** You must
change the partition scheme or the build will fail with "text section
exceeds available space."

In Arduino IDE:

> **Tools → Partition Scheme → "Huge APP (3MB No OTA/1MB SPIFFS)"**

This gives ~3 MB for application code. OTA updates are not available with
this partition layout.

This is an ESP-IDF / Arduino core limitation, not xelp. xelp itself adds
~4-5 KB. The WiFi stack (~500 KB) and NimBLE stack (~300 KB) account for
the bulk of the Flash usage. Even a trivial WiFi + BLE sketch without
xelp hits the same wall on the default partition.

## Files

```
esp32c6-wifi/
  esp32c6-wifi.ino    Arduino sketch (main source)
  xelp.c              symlink → ../../src/xelp.c
  xelp.h              symlink → ../../src/xelp.h
  xelpcfg.h           symlink → ../../src/xelpcfg.h
  XelpArduino.h       symlink → ../../src/XelpArduino.h
  web/
    index.html         Web Bluetooth terminal (open in Chrome/Edge)
```

The symlinks let you develop against the repo's `src/` directly. Arduino
IDE compiles all `.c` files in the sketch folder automatically.

## Commands

| Command | Description |
|---------|-------------|
| `help` | List all commands |
| `ssid <name>` | Set WiFi SSID (saved to NVS) |
| `wifipass <pw>` | Set WiFi password (saved to NVS) |
| `connect` | Connect to WiFi using saved credentials |
| `disconnect` | Disconnect from WiFi |
| `status` | Show WiFi state, IP, RSSI, BLE connection |
| `led <0\|1>` | Toggle on-board LED |
| `adc` | Read A0 voltage (16-sample average, 1:2 divider) |

Credentials are stored in NVS and persist across power cycles.

## Web Bluetooth Terminal

Open `web/index.html` in Chrome or Edge (Web Bluetooth is not supported
in Firefox or Safari). Click "Connect", select "xelp-c6", and type
commands in the terminal. Quick buttons are provided for common actions.

The page is fully self-contained -- no build step, no server required.

## Serial Usage

```
$ screen /dev/tty.usbmodem* 115200

xelp> ssid MyNetwork
SSID set: MyNetwork (saved)
xelp> wifipass MyPassword
Password set (saved)
xelp> connect
Connecting to MyNetwork........ OK
IP: 192.168.1.42
xelp> adc
A0: 3.312 V
xelp> led 1
LED ON
```

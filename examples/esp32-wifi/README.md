# ESP32 WiFi

ESP32 WiFi CLI using the C++ wrapper. Configure WiFi credentials over the
serial CLI, then fetch the current time and weather from free APIs. No API
keys needed.

## Requirements

- ESP32 board (any variant: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, etc.)
- [ESP32 Arduino core](https://docs.espressif.com/projects/arduino-esp32/)
  (ships with WiFi.h and HTTPClient.h)
- xelp installed via Arduino Library Manager or symlinked from `src/`

## Setup

1. Open `esp32-wifi.ino` in the Arduino IDE.
2. Select your ESP32 board and port.
3. Upload and open the Serial Monitor at **115200 baud**.

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `ssid <name>` | Set WiFi SSID |
| `pass <password>` | Set WiFi password |
| `connect` | Connect to WiFi |
| `disconnect` | Disconnect from WiFi |
| `status` | Show WiFi status, IP, RSSI |
| `time` | Fetch current time (worldtimeapi.org) |
| `weather <lat> <lon>` | Fetch weather (open-meteo.com) |
| `led <0\|1>` | Turn on-board LED on/off |
| `divmod <a> <b>` | Integer division (R1 = quotient, R2 = remainder) |
| `pr` | Print all registers |

## Example Session

```
esp32> ssid MyNetwork
SSID set: MyNetwork
esp32> pass MyPassword
Password set
esp32> connect
Connecting to MyNetwork...... OK
IP: 192.168.1.42
wifi> time
Fetching time...
2025-06-15T14:30:22.123456-07:00 (America/Los_Angeles)
wifi> weather 37.77 -122.42
Fetching weather...
Temperature: 18.5 C
Wind: 12.3 km/h
Weather code: 1 (partly cloudy)
```

## What It Demonstrates

- WiFi connection management over CLI
- HTTP GET requests from CLI commands
- Lightweight JSON parsing (no library needed)
- Dynamic prompt change (`esp32>` before connect, `wifi>` after)
- Register accessors for command return values

For a version with BLE support, see the
[esp32c6-wifi](../esp32c6-wifi/) example.

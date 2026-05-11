# Pico CLI (Pure C, Pico SDK)

Interactive CLI for Raspberry Pi Pico / Pico W / Pico 2 using xelp and the
Pico SDK. No C++ or Arduino required.

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `led <0\|1>` | Turn on-board LED on/off |
| `pin <n> <in\|out>` | Configure GPIO direction |
| `set <pin> <0\|1>` | Set GPIO output level |
| `get <pin>` | Read GPIO input level |
| `adc <0-3>` | Read ADC channel (GP26-GP29) |
| `pwm <pin> <0-255>` | Set PWM duty cycle |

Single-key mode (press ESC to enter): `?` = help, `l` = toggle LED.

## Building

Requires the [Pico SDK](https://github.com/raspberrypi/pico-sdk).

```bash
# Copy (or symlink) xelp source into this directory
ln -s ../../src/xelp.c .
ln -s ../../src/xelp.h .
ln -s ../../src/xelpcfg.h .

# Build
mkdir build && cd build
cmake -DPICO_BOARD=pico ..    # or pico_w, pico2, pico2_w
make

# Flash xelp_pico_cli.uf2 via USB bootloader
# Hold BOOTSEL, plug in USB, copy .uf2 to the mounted drive:
cp xelp_pico_cli.uf2 /Volumes/RP2350/   # macOS
# cp xelp_pico_cli.uf2 /media/$USER/RP2350/  # Linux

# Serial monitor
screen /dev/tty.usbmodem* 115200         # macOS
# screen /dev/ttyACM0 115200             # Linux
```

## Pico W LED

The build automatically detects Pico W boards and links the CYW43 driver
for the wireless-chip-controlled LED. No code changes needed.

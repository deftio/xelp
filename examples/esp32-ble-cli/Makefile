# Makefile for esp32-ble-cli xelp example
#
# Wraps arduino-cli for compile, upload, and monitor.
# Auto-detects the first ESP32 USB serial port.
#
# Usage:
#   make              # compile, upload, and open serial monitor
#   make compile      # compile only
#   make upload       # compile and upload
#   make monitor      # open serial monitor
#   make web          # serve Web Bluetooth terminal on localhost:8086
#   make clean        # remove build artifacts
#   make ports        # list connected boards
#
# Override defaults:
#   make upload FQBN=esp32:esp32:esp32s3 PORT=/dev/ttyUSB0

FQBN     ?= esp32:esp32:um_pros3
BAUD     ?= 115200
WEB_PORT ?= 8086
SKETCH    = .

# Auto-detect first ESP32 USB port (override with PORT=...)
PORT ?= $(shell arduino-cli board list 2>/dev/null \
          | grep -i 'usb.*esp32\|usbmodem' \
          | head -1 | awk '{print $$1}')

.PHONY: all compile upload monitor web clean ports help

all: help

compile:
	arduino-cli compile --fqbn $(FQBN) $(SKETCH)

upload: compile
	@if [ -z "$(PORT)" ]; then \
	  echo "ERROR: no ESP32 port detected. Plug in the board or set PORT="; \
	  exit 1; \
	fi
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) $(SKETCH)

monitor:
	@if [ -z "$(PORT)" ]; then \
	  echo "ERROR: no ESP32 port detected. Plug in the board or set PORT="; \
	  exit 1; \
	fi
	arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD)

web:
	@echo "Serving Web Bluetooth terminal at http://localhost:$(WEB_PORT)"
	@echo "Open in Chrome or Edge.  Press Ctrl-C to stop."
	cd web && python3 -m http.server $(WEB_PORT)

clean:
	rm -rf build

ports:
	arduino-cli board list

help:
	@echo "Targets:"
	@echo "  make              compile, upload, open serial monitor"
	@echo "  make compile      compile only"
	@echo "  make upload       compile and upload"
	@echo "  make monitor      serial monitor ($(BAUD) baud)"
	@echo "  make web          serve BLE web terminal on port $(WEB_PORT)"
	@echo "  make clean        remove build artifacts"
	@echo "  make ports        list connected boards"
	@echo ""
	@echo "Variables:"
	@echo "  FQBN=$(FQBN)"
	@echo "  PORT=$(PORT)"
	@echo "  BAUD=$(BAUD)"
	@echo "  WEB_PORT=$(WEB_PORT)"

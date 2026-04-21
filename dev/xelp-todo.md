# xelp Roadmap and TODO

Future work for xelp, organized by area. Items marked with a design doc
link have detailed specifications in `dev/`.

## Recently Completed (0.3.0 – 0.3.1)

- [x] Breaking API change: `XELP *ths` on all command/key signatures
- [x] XelpBuf macro normalization (SCREAMING_CASE)
- [x] 100% line coverage (295 cases, 22 units)
- [x] GitHub Actions CI (build matrix, coverage, release workflow)
- [x] Cross-compilation Docker tooling (12 architectures)
- [x] Default command handlers (`mpfDefKey`, `mpfDefCLI`)
- [x] 4 return registers (`mR[0..3]`) with `XELP_R0`-`XELP_R3` macros
- [x] C++ wrapper register accessors (`r0()`-`r3()`)
- [x] Published to GitHub, Arduino, ESP-IDF, PlatformIO
- [x] Full documentation site (pages/, docs/)
- [x] Release automation (`make_release.sh`)

## Scripting Engine (deferred -- clean up core first)

Text-based scripting extensions building on the existing `XELPParse`
infrastructure. Tcl/bash-style syntax, no new parsing concepts.
Not starting until dev experience and examples are solid.

- [ ] Named variables (`$x`, `$count`) -- integer-valued, stored in a
      user-supplied heap buffer
- [ ] Positional parameters (`$1`, `$2`) for function arguments
- [ ] User-defined functions (`proc`) -- body strings stored in heap
- [ ] Control flow: `_if`/`_else`/`_endif`, `_while`/`_endwhile`, `_goto`
- [ ] Arithmetic and comparison builtins: `_add`, `_sub`, `_mul`, `_eq`, `_lt`
- [ ] Return values via `$?` / `r0`

Design doc: [dev/xelp_script.md](xelp_script.md)

## Virtual Machine (postponed -- likely a separate project)

Register-based bytecode VM. Feels like its own project that happens to
include xelp as the text front-end. Parked here for reference.

Design doc: [dev/xelp_vm.md](xelp_vm.md)

## CLI Ergonomics

Quality-of-life improvements for interactive use. Each is compile-time
optional (`XELP_ENABLE_ANSI` or similar). Design constraint: must not
bloat the core or break existing bare-metal use cases.

- [ ] **Command history** -- circular buffer of last N command lines,
      up/down arrow recall. User-supplied buffer (e.g. `char hist[4][64]`).
      Requires ANSI escape sequence detection (arrow keys are 3-byte:
      ESC `[` A/B). ~100-150 lines, ~300 bytes ARM Thumb.
      **Key design issue:** ESC is currently the KEY-mode switch key.
      Arrow keys start with ESC. Options:
      - (a) Reassign KEY-mode to CTRL-K; ESC is then free for sequences
      - (b) After ESC, peek at next byte: `[` = sequence, else = mode switch
      - (c) Timeout-based (requires timer, bad for bare metal)
      Option (b) is most embedded-friendly (no timer, deterministic).
- [ ] **Tab completion** -- match partial input against the CLI command
      table, complete or show candidates. No extra memory needed (walks
      the existing table). Small and self-contained.
- [ ] **Line editing** -- left/right cursor, insert, delete. Requires
      reworking buffer model from append-only to insert-at-cursor.
      More invasive (~200-300 lines, touches core ParseKey logic).
      Ship after history, not with it. Consider whether the code size
      cost is justified for the target audience.
- [ ] **ANSI terminal helpers** -- small utility functions for cursor
      movement, color, clear screen. Useful for KEY mode menus.

### KEY mode string keys

Currently KEY mode matches a single `char`. Plan: allow KEY mode
commands to match multi-byte strings (e.g. `"a"` instead of `'a'`).
This unifies KEY and CLI dispatch (KEY = CLI with 1-char commands)
and naturally handles escape sequences as key bindings. Prerequisite
for clean arrow key handling. See Phase 6 (unified command structure)
in xelp-plan-2025.md.

## C++ Wrappers

Two audiences: Arduino hobbyists and professional embedded devs (ESP-IDF,
Zephyr, Mbed, PlatformIO native). Many pro devs won't touch a header
with `#include <Arduino.h>` in it. Split serves usability for both.

- [ ] **`xelp_cpp.h`** -- generic C++ wrapper, no platform dependencies.
      RAII class, works anywhere with a C++ compiler. The "professional"
      entry point.
- [ ] **`xelp_arduino.h`** -- Arduino-specific. `Stream&` binding,
      `poll()`, `begin()` convenience. The "easy" entry point.
- [ ] **`XelpBuf` C++ wrapper** -- safer buffer class with bounds
      checking, iteration, auto-increment. Wraps the C macros.

## Build and Packaging

- [x] ESP Component Registry publishing (live at components.espressif.com)
- [x] Arduino Library Manager (live, indexed)
- [x] PlatformIO Registry (live, note: `v0.3` ghost release issue)
- [ ] **Remove RISC-V badge** from README -- replace with Arduino,
      PlatformIO, and Espressif library badges (on a second badge line).
- [ ] **CMake native build** -- optional `CMakeLists.txt` for non-ESP
      CMake projects (guard with `if(ESP_PLATFORM)` / `else`).
- [ ] **PlatformIO library.json CI** -- add PlatformIO build check to
      GitHub Actions CI.
- [ ] **Fix PlatformIO `0.3` ghost** -- delete `v0.3` tag/release so
      PlatformIO re-indexes `0.3.0` as latest.

## Testing

- [ ] **Fuzz testing** -- feed random / adversarial byte streams through
      `XELPParseKey` and `XELPParse` to catch buffer overflows or hangs.
- [ ] **Multi-instance stress test** -- run two XELP instances
      interleaved in the same test to verify no shared state leaks.
- [ ] **Cross-platform CI** -- add ARM and RISC-V QEMU builds to GitHub
      Actions to catch word-size or alignment bugs.

## Version Encoding

- [ ] **Add `XELP_BUILD` define** -- separate build counter alongside
      `XELP_VERSION`. Keep `0x00MMmmpp` encoding unchanged (no breakage).
      `XELP_BUILD` is 0 for releases, incremented during dev for
      traceability. `extract_version.c` emits it in YAML.
      `make_release.sh` validates `XELP_BUILD == 0` before tagging.
      Print format: `"0.3.1"` (release) or `"0.3.1+3"` (dev build).

## Examples

- [ ] **ESP32-C6 dual-CLI example** (`examples/esp32c6-wifi/`) --
      Multi-instance demo running xelp on both Serial and BLE
      simultaneously. Both CLI instances can set WiFi SSID and password
      (stored in NVS). Once configured, the C6 connects to WiFi and
      serves a small web page (or hits an external API). Demonstrates:
      - Two XELP instances with different prompts (`ser>`, `ble>`)
      - Shared command table (wifi_ssid, wifi_pw, wifi_connect, status)
      - NVS persistence for credentials
      - Lightweight HTTP server or HTTP client after connect
      - BLE GATT characteristic as xelp byte stream input
      - Real-world use case: headless device provisioning over BLE,
        then switching to serial for debug/monitoring

## Build Tooling

- [ ] **crossbuild.sh multi-config** -- update `tools/crossbuild.sh` to
      build each target three times (KEY-only, CLI, FULL) and output a
      table with all three sizes. Group by word size (8/16/32/64-bit).
      Add PIC16 and PIC18 targets via SDCC (`sdcc -mpic14`, `sdcc -mpic16`).
      Add `sdcc` to `Dockerfile.crossbuild` apt install list.
      This produces the data for the README size table.

## Documentation

- [ ] **README size messaging** -- Opening paragraph and size table are
      confusing for new readers. Currently the table shows one number per
      target with no indication of which features are enabled, and the
      prose mentions "KEY-only mode: 900 bytes" separately. Fix:
      - Size table should have three columns: KEY-only, CLI, FULL
      - Group rows by word size (8-bit, 16-bit, 32-bit, 64-bit families)
      - Add PIC16x and PIC18x targets
      - Opening paragraph should say something like "900 bytes (KEY-only)
        to 4 KB (all features)" with a clear pointer to the table
      - Remove or reword the "KEY-only mode" phrasing that assumes the
        reader already knows what KEY mode is
- [ ] **Interactive web demo** -- Emscripten/WASM build of xelp running
      in the browser on the GitHub Pages site.
- [ ] **Migration guide** -- standalone doc for upgrading from v0.2.x to
      v0.3.x (currently in CHANGELOG overview, could be a separate page).

## Legacy Reference

Historical design notes and early brainstorming are preserved at
[dev/manu_xelp_notes_legacy.md](manu_xelp_notes_legacy.md).

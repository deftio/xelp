# xelp Roadmap and TODO

Future work for xelp, organized by area. Items marked with a design doc
link have detailed specifications in `dev/`.

## Recently Completed (0.3.0 – 0.3.2)

- [x] Breaking API change: `XELP *ths` on all command/key signatures
- [x] XelpBuf macro normalization (SCREAMING_CASE)
- [x] 100% line coverage (39 units, 531 cases)
- [x] GitHub Actions CI (build matrix, coverage, release)
- [x] Cross-compilation Docker tooling (18 targets, 8-bit to 64-bit)
- [x] Default command handlers (`mpfDefKey`, `mpfDefCLI`)
- [x] 4 return registers (`mR[0..3]`) with `XELP_R0`-`XELP_R3` macros
- [x] C++ wrapper (`XelpArduino.h`) with register accessors, lambdas,
      `poll()`. Works on Arduino and desktop C++.
- [x] Published to GitHub, Arduino, ESP-IDF, PlatformIO
- [x] Full documentation site (pages/, docs/)
- [x] Release automation (`make_release.sh` -- guided end-to-end pipeline)
- [x] Single-line editing + multi-byte key support (`XELPKEYCODE`,
      `XELP_KEYCODE_*` macros, `XELP_ENABLE_LINE_EDIT`, cursor movement,
      Home/End, insert-at-cursor, Delete at cursor)
- [x] Fuzz testing (found and fixed SEGV in XelpTokLineXB)
- [x] Multi-instance stress test (two interleaved instances)
- [x] ESP32-C6 dual-CLI example (Serial + BLE, WiFi provisioning, NVS)
- [x] PlatformIO CI removed from CI (run locally; was blocking releases)
- [x] Cross-build multi-config (KEY/CLI/FULL, `extract_size.py`, 18 targets)
- [x] README rewrite (3-column size table, grouped by word size)
- [x] CI aligned with local validation (`make validate` in all workflows)
- [x] Command history (`XELP_ENABLE_HISTORY`): ring-buffer recall with
      UP/DOWN arrows, consecutive dup suppression, in-progress save/restore.
      ~420 bytes ARM Thumb.
- [x] `XelpArgInt` / `XelpArgStr` convenience functions for direct argument
      access by index.
- [x] Test suite: 50 units, 693 cases, 100% line coverage

## Scripting Engine (deferred -- clean up core first)

Text-based scripting extensions building on the existing `XelpParse`
infrastructure. Tcl/bash-style syntax, no new parsing concepts.
Not starting until dev experience and examples are solid.

- [ ] Named variables (`$x`, `$count`) -- integer-valued, stored in a
      user-supplied heap buffer
- [ ] Positional parameters (`$1`, `$2`) for function arguments
- [ ] User-defined functions (`proc`) -- body strings stored in heap
- [ ] Control flow: `_if`/`_else`/`_endif`, `_while`/`_endwhile`, `_goto`
- [ ] Arithmetic and comparison builtins: `_add`, `_sub`, `_mul`, `_eq`, `_lt`
- [ ] Return values via `$?` / `r0`
- [ ] **Parenthesized sub-expressions** -- `command (subcommand arg) arg`
      evaluates the inner expression first and substitutes the result.
      Two parts: (1) PSM paren recognition + depth counter, (2) expansion
      buffer + recursive eval. Integer-only substitution as MVP. See
      conversation notes on buffer cost (~64 bytes) and stack depth risk
      on 8-bit targets.
- [ ] **Tagged strings** -- explore post-tag syntax `"string"f` where
      a single character after the closing quote annotates the token.
      `_PS_QEND` is the natural intercept point (already fires once after
      closing `"`). Currently the tag char is silently swallowed by the
      default `_PS_QEND` -> `_PS_SEOL` transition. Implementation needs:
      (a) new `_PS_QEND` rule to capture alphanumeric tag chars,
      (b) plumbing the tag out of the tokenizer (XelpBuf has no tag field
      today -- either extend XelpBuf or use a side channel).
      Pre-tag syntax `f"string"` is harder (requires look-ahead from
      `_PS_TOK0` into `_PS_QUOT`). Post-tag is architecturally cleaner.
      Use cases: format specifiers, string type hints for the scripting
      engine, user-defined token annotations.

Design doc: [dev/xelp_script.md](xelp_script.md)

## Virtual Machine (postponed -- likely a separate project)

Register-based bytecode VM. Feels like its own project that happens to
include xelp as the text front-end. Parked here for reference.

Design doc: [dev/xelp_vm.md](xelp_vm.md)

## Argument Parsing Ergonomics

Two-phase plan to reduce per-command boilerplate. Design doc:
[dev/arg_parse_updates.md](arg_parse_updates.md).

### 0.3.2: Non-breaking convenience functions

- [x] **`XelpArgInt(args, len, n, &val)`** -- get arg N as int, one call.
      Wrapper over XelpTokN + XelpParseNum. ~50 bytes ARM Thumb.
- [x] **`XelpArgStr(args, len, n, &s, &slen)`** -- get arg N as string
      span. ~40 bytes ARM Thumb.

Functions (not macros) in the base CLI API. No new flag. Arg 0 is the
command name per argc/argv convention.

### 0.4.0: Breaking handler signature change (argc/argv)

- [ ] **Change CLI handler signature** from
      `fn(XELP *ths, const char *args, int len)` to
      `fn(XELP *ths, int argc, XelpBuf *argv)`.
      Dispatcher pre-tokenizes into stack-allocated `XelpBuf argv[]`.
      `XelpArgInt`/`XelpArgStr` simplify to take `XelpBuf *` directly.

Last planned breaking change before the scripting engine. Stack cost
(~12 bytes per arg on 32-bit) is acceptable -- the primary audience
is 32-bit targets (ARM, ESP32, RISC-V). 8/16-bit builds remain
supported but are not the optimization target.

## CLI Ergonomics

Quality-of-life improvements for interactive use. Each is compile-time
optional. Design constraint: must not bloat the core or break existing
bare-metal use cases.

- [x] **Command history** -- ring buffer of last N command lines,
      up/down arrow recall. User-supplied buffer (e.g. `char hist[4][64]`).
      Multi-byte key detection is already in place (`XELPKEYCODE` handles
      ESC sequences). Up/Down are currently silently dropped -- reserved
      for this feature. ~100-150 lines, ~300 bytes ARM Thumb.
- [ ] **Tab completion** -- match partial input against the CLI command
      table, complete or show candidates. No extra memory needed (walks
      the existing table). Small and self-contained.
- [x] **Line editing** -- left/right cursor, Home/End, insert, delete.
      Implemented via `XELP_ENABLE_LINE_EDIT`. Cursor tracking in
      `mCur` pointer. Insert-at-cursor with shift model.
- [ ] **ANSI terminal helpers** -- small utility functions for cursor
      movement, color, clear screen. Useful for KEY mode menus.

### KEY mode multi-byte keys (done)

KEY mode now matches `XELPKEYCODE` (unsigned long) instead of `char`.
This naturally handles escape sequences as key bindings. The 4-byte
packed format (`XELP_KEYCODE_UP`, etc.) allows single-char keys to
work unchanged while multi-byte ANSI sequences are recognized.

## C++ Wrappers

Single header `src/XelpArduino.h` serves both audiences:

- [x] **`XelpArduino.h`** -- header-only C++ wrapper. RAII class with
      `poll()`, `begin()` (Arduino), lambda command registration (Easy
      API), register accessors (`r0()`-`r3()`). Works on Arduino and
      desktop C++ (compiles without `<Arduino.h>` when not on Arduino).

Future:
- [ ] **Standalone `xelp_cpp.h`** -- minimal C++ wrapper without any
      Arduino dependency in the header. For professional embedded devs
      using ESP-IDF, Zephyr, Mbed, PlatformIO native who won't touch
      a header with `#include <Arduino.h>` guarded or not.

## Build and Packaging

- [x] ESP Component Registry publishing (live at components.espressif.com)
- [x] Arduino Library Manager (live, indexed)
- [x] PlatformIO Registry (live, v0.3 ghost fixed)
- [x] Badges (Arduino, PlatformIO, Espressif library)
- [x] CMake native build (`CMakeLists.txt`)
- [x] PlatformIO CI removed (fuzz, 32-bit, pio run locally not in CI)
- [x] PlatformIO `v0.3` ghost release deleted

## Testing

- [x] **Fuzz testing** -- `XelpParseKey` and `XelpParse` harnesses
      in `tests/fuzz/`. Found and fixed SEGV.
- [x] **Multi-instance stress test** -- two interleaved XELP instances,
      shared command table, verified no shared state leaks.
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

- [x] **ESP32-C6 dual-CLI example** (`examples/esp32c6-wifi/`) --
      Serial + BLE simultaneous CLI instances, WiFi provisioning,
      NVS persistence, web interface.
- [ ] **Pico W example** (`examples/pico-cli/`) -- RP2040/RP2350
      demo with USB serial CLI.

## Documentation

- [x] **README size messaging** -- 3-column table (KEY/CLI/FULL),
      grouped by word size, clear opening paragraph.
- [ ] **Interactive web demo** -- Emscripten/WASM build of xelp running
      in the browser on the GitHub Pages site.
- [ ] **Migration guide** -- standalone doc for upgrading from v0.2.x to
      v0.3.x (currently in CHANGELOG overview, could be a separate page).

## Legacy Reference

Historical design notes and early brainstorming are preserved at
[dev/manu_xelp_notes_legacy.md](manu_xelp_notes_legacy.md).

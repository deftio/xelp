# xelp Roadmap and TODO

Future work for xelp, organized by area. Items marked with a design doc
link have detailed specifications in `dev/`.

## Scripting Engine

Text-based scripting extensions building on the existing `XELPParse`
infrastructure. Tcl/bash-style syntax, no new parsing concepts.

- [ ] Named variables (`$x`, `$count`) -- integer-valued, stored in a
      user-supplied heap buffer
- [ ] Positional parameters (`$1`, `$2`) for function arguments
- [ ] User-defined functions (`proc`) -- body strings stored in heap
- [ ] Control flow: `_if`/`_else`/`_endif`, `_while`/`_endwhile`, `_goto`
- [ ] Arithmetic and comparison builtins: `_add`, `_sub`, `_mul`, `_eq`, `_lt`
- [ ] Return values via `$?` / `r0`

Design doc: [dev/xelp_script.md](xelp_script.md)

## Virtual Machine

Register-based bytecode VM for binary protocols, compact ROM scripts,
and deterministic dispatch timing. Runs alongside the text CLI on the
same XELP instance.

- [ ] Opcode set: CALL, LOAD, STORE, ADD, SUB, CMP, JMP, JZ, JNZ, RET, HALT
- [ ] Shared function table with CLI/KEY dispatch
- [ ] Program counter, register file, small call stack
- [ ] `XELPVMExec()` entry point
- [ ] Target: 2-3 KB compiled (ARM Thumb-2)

Design doc: [dev/xelp_vm.md](xelp_vm.md)

## CLI Ergonomics

Quality-of-life improvements for interactive use. Each is small and
independent.

- [ ] **Command history** -- circular buffer of last N command lines,
      up/down arrow recall. Requires ANSI escape sequence detection
      (arrow keys are multi-byte: ESC `[` A/B). User-supplied buffer
      to keep zero-malloc guarantee.
- [ ] **Line editing** -- left/right arrow cursor movement within the
      command buffer, insert mode. Requires ANSI cursor control output.
- [ ] **Tab completion** -- match partial input against the CLI command
      table, complete or show candidates. No extra memory needed (walks
      the existing table).
- [ ] **ANSI terminal helpers** -- small utility functions for cursor
      movement, color, clear screen. Optional compile flag
      (`XELP_ENABLE_ANSI`). Useful for KEY mode menus and status displays.

## C++ Wrappers

Split the current `XelpArduino.h` into two headers.

- [ ] **`xelp_cpp.h`** -- generic C++ wrapper, no platform dependencies.
      RAII class wrapping the C `XELP` struct. Works on ESP-IDF, Mbed,
      Zephyr, PlatformIO native, desktop.
- [ ] **`xelp_arduino.h`** -- Arduino-specific wrapper. Adds `Stream&`
      binding for input polling, `begin(aboutMsg, outputFn, Stream&)`
      convenience overload, `poll()` method.
- [ ] **`XelpBuf` C++ wrapper** -- safer buffer class that handles pointer
      arithmetic, bounds checking, `getChar()` with auto-increment,
      iteration, and length queries. Wraps the C `XelpBuf` macros.

## Build and Packaging

- [ ] **ESP Component Registry publishing** -- `idf_component.yml` and
      `CMakeLists.txt` are in place; register with
      components.espressif.com and add publish step to release script.
- [ ] **CMake native build** -- optional `CMakeLists.txt` for non-ESP
      CMake projects (guard with `if(ESP_PLATFORM)` / `else`).
- [ ] **PlatformIO library.json CI** -- add PlatformIO build check to
      GitHub Actions CI.

## Testing

- [ ] **Fuzz testing** -- feed random / adversarial byte streams through
      `XELPParseKey` and `XELPParse` to catch buffer overflows or hangs.
- [ ] **Multi-instance stress test** -- run two XELP instances
      interleaved in the same test to verify no shared state leaks.
- [ ] **Cross-platform CI** -- add ARM and RISC-V QEMU builds to GitHub
      Actions to catch word-size or alignment bugs.

## Documentation

- [ ] **Interactive web demo** -- Emscripten/WASM build of xelp running
      in the browser on the GitHub Pages site.
- [ ] **Migration guide** -- standalone doc for upgrading from v0.2.x to
      v0.3.x (currently in CHANGELOG overview, could be a separate page).

## Legacy Reference

The original TODO list with historical context and early design notes
is preserved at [dev/xelp-TODO-legacy.md](xelp-TODO-legacy.md).

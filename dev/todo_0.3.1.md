# xelp 0.3.1 Sprint

May ship as 0.4.0 if the key table struct change warrants a minor bump.
Decision deferred until implementation is complete.

## Tasks

- [ ] **Single-line editing + multi-byte key support** -- Structural
      change to KEY mode: `char key` becomes `char key[4]` in the command
      table. 4-byte key accumulator in `XELPParseKey` to recognize
      multi-byte ANSI sequences (arrow keys, Home/End, Delete, function
      keys). Provide `XELP_KEY_CHAR(c)` macro for single-char keys and
      named macros for common multi-byte keys (`XELP_KEY_UP`, etc.).
      In CLI mode: left/right cursor movement, Home/End, insert-at-cursor
      (shift model replaces append-only), Delete at cursor. Up/Down
      silently dropped (reserved for future history). Eliminates garbage
      `[A` characters when users press arrow keys. Header and docs must
      explain the 4-byte design rationale clearly for less experienced
      users. Estimated +1 KB compiled (ARM Thumb). Needs thorough testing
      of all insert/delete/cursor combinations.
- [ ] **Badges** -- Remove RISC-V badge from README, add Arduino,
      PlatformIO, and Espressif library badges on a second line.
- [ ] **PlatformIO ghost fix** -- Delete `v0.3` tag/release from GitHub
      so PlatformIO re-indexes `0.3.0` as latest.
- [ ] **crossbuild.sh multi-config** -- Build each target three times
      (KEY-only, CLI, FULL). Add PIC16/PIC18 via SDCC. Group output by
      word size (8/16/32/64-bit). Add `sdcc` to Dockerfile.
- [ ] **README rewrite** -- Rewrite opening paragraph for clarity.
      Replace size table with 3-column (KEY/CLI/FULL) version grouped
      by word size. Remove jargon that assumes reader knows KEY mode.
- [ ] **Fuzz testing** -- Create AFL/libFuzzer harness for `XELPParseKey`
      and `XELPParse`. Run, fix any findings. Add harness to repo
      (e.g. `tests/fuzz/`).
- [ ] **Multi-instance stress test** -- Add unit test that runs two XELP
      instances interleaved (alternating char feeds, shared command table)
      to verify no shared state leaks.
- [ ] **ESP32-C6 dual-CLI example** -- `examples/esp32c6-wifi/`. Serial +
      BLE simultaneous CLI instances. WiFi SSID/password provisioning via
      either transport. NVS persistence. Small HTTP server or client after
      connect. Starter for an article demonstrating xelp in a real product
      pattern.
- [ ] **PlatformIO CI** -- Add PlatformIO build check to GitHub Actions
      CI workflow. Install PlatformIO via pip, run `pio ci` against a
      few boards (e.g. uno, esp32dev). Validates `library.json` on every
      push. ~30 lines of YAML.
- [ ] **Documentation sweep** -- Update all docs, pages, and metadata to
      reflect the release: `docs/` markdown (api-reference, tutorial),
      `pages/` HTML (api-reference.html, index.html), `README.md`,
      `AGENTS.md`, `LLMTEXT.md`, `CHANGELOG.md`. Ensure new features
      (multi-byte keys, single-line editing, register macros, key macros)
      are documented everywhere consistently. Final review pass.

## Order of operations

1. Single-line editing + multi-byte keys (structural, touches core)
2. Fuzz testing (finds bugs in new + existing code)
3. Multi-instance stress test (verify no shared state)
4. PlatformIO ghost fix (unblocks clean badge URLs)
5. PlatformIO CI (small, validates library.json going forward)
6. crossbuild.sh multi-config (produces size data, shows new code size)
7. README rewrite + badges (uses size data, documents new features)
8. ESP32-C6 example (can develop in parallel)
9. Documentation sweep (last -- captures everything above)

## Notes

After this release, focus shifts to developer experience: C++ wrappers
(Arduino and posix), simple "get started in 3 lines" examples, and
content for articles and community forums. The library works well once
you understand it, but onboarding is too industrial for casual users.

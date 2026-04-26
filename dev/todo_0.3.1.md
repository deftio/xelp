# xelp 0.3.1 Tasks

May ship as 0.4.0 if the key table struct change warrants a minor bump.
Decision deferred until implementation is complete.

## Tasks

- [x] **Single-line editing + multi-byte key support** -- Structural
      change to KEY mode: `char key` becomes `XELPKEYCODE` (unsigned long)
      in the command table. 4-byte key accumulator in `XelpParseKey` to
      recognize multi-byte ANSI sequences (arrow keys, Home/End, Delete,
      PgUp/PgDn). Named macros: `XELP_KEYCODE_UP`, `XELP_KEYCODE_DOWN`,
      `XELP_KEYCODE_LEFT`, `XELP_KEYCODE_RIGHT`, `XELP_KEYCODE_HOME`,
      `XELP_KEYCODE_END`, `XELP_KEYCODE_KDEL`, `XELP_KEYCODE_INS`,
      `XELP_KEYCODE_PGUP`, `XELP_KEYCODE_PGDN`. Helper macros:
      `XELP_KC_IS_MULTI(k)`, `XELP_KC_B0..B3(k)`. In CLI mode (with
      `XELP_ENABLE_LINE_EDIT`): left/right cursor movement, Home/End,
      insert-at-cursor, Delete at cursor. Up/Down silently dropped
      (reserved for future history). Eliminates garbage `[A` characters
      when users press arrow keys.
- [x] **Badges** -- Remove RISC-V badge from README, add Arduino,
      PlatformIO, and Espressif library badges on a second line.
- [x] **PlatformIO ghost fix** -- Delete `v0.3` tag/release from GitHub
      so PlatformIO re-indexes `0.3.0` as latest.
- [x] **crossbuild.sh multi-config** -- Build each target three times
      (KEY-only, CLI, FULL). Group output by word size (8/16/32/64-bit).
      Added `sdcc` to Dockerfile. Fixed missing `#ifdef XELP_ENABLE_CLI`
      guard on mCmdXB init in XelpInit (KEY-only config would not compile
      without it). Added `extract_size.py` for robust multi-format size
      extraction (ELF, SDCC .rel, SDCC .map, Intel HEX). Dropped targets
      that cannot compile xelp (6502/cc65, 8086/ia16, PIC14, PIC16).
      Commented out MCS-51 (code areas split across HOME+_CODE makes
      reporting unreliable). 18 targets produce real sizes.
- [x] **README rewrite** -- Rewrite opening paragraph for clarity.
      Replace size table with 3-column (KEY/CLI/FULL) version grouped
      by word size. Remove jargon that assumes reader knows KEY mode.
      Updated test counts (39 units, 531 cases). Fixed typos. Added
      line editing to feature table.
- [x] **Fuzz testing** -- Create AFL/libFuzzer harness for `XelpParseKey`
      and `XelpParse`. Run, fix any findings. Add harness to repo
      (`tests/fuzz/`).
      Found and fixed SEGV in XelpTokLineXB: uninitialised tok->s when
      buffer exhausted in _PS_ESCA state (CLI escape char at end of input).
- [x] **Multi-instance stress test** -- Add unit test that runs two XELP
      instances interleaved (alternating char feeds, shared command table)
      to verify no shared state leaks.
- [x] **ESP32-C6 dual-CLI example** -- `examples/esp32c6-wifi/`. Serial +
      BLE simultaneous CLI instances. WiFi SSID/password provisioning via
      either transport. NVS persistence. HTTP server after connect.
      Companion `web/index.html` for browser-based BLE terminal.
- [x] **PlatformIO CI** -- Add PlatformIO build check to GitHub Actions
      CI workflow. Install PlatformIO via pip, run `pio ci` against
      boards (uno, megaatmega2560, esp32dev). Validates `library.json`
      on every push.
- [x] **Documentation sweep** -- Update all docs, pages, and metadata to
      reflect the release: `docs/` markdown (api-reference, tutorial,
      porting, build-profiles, configuration, testing), `pages/` HTML
      (all synced with markdown counterparts), `README.md`, `AGENTS.md`,
      `llms.txt`, `CHANGELOG.md`. Fixed old-style function names
      throughout (XELPParseKey -> XelpParseKey, etc.). Removed dropped
      targets from size tables and architecture lists. Updated all
      code examples to use current API.
- [x] **Release pipeline rewrite** -- Rewrote `tools/make_release.sh`
      to handle the full release end-to-end: extract version, sync
      manifests, update badges, validate, Docker cross-build + size
      tables, commit pipeline changes (with whitelist), push, PR, CI
      wait (with on-master polling), squash merge (with fallback),
      switch to master (squash-merge divergence handling), verify,
      tag, wait for GitHub Release, publish to PlatformIO and ESP-IDF.
      Graceful skip when pio/compote CLIs not installed. Actionable
      error messages with recovery instructions at every failure point.
- [x] **CI alignment** -- Updated `.github/workflows/ci.yml` and
      `release.yml` to use `make validate` (tests + examples) instead
      of `make tests`. Added `libncurses-dev` dependency. Fixed 32-bit
      build flags (were missing `-Wextra -Werror`). CI now matches
      local validation exactly.

## Order of operations

1. Single-line editing + multi-byte keys (structural, touches core)
2. Fuzz testing (finds bugs in new + existing code)
3. Multi-instance stress test (verify no shared state)
4. PlatformIO ghost fix (unblocks clean badge URLs)
5. PlatformIO CI (small, validates library.json going forward)
6. crossbuild.sh multi-config (produces size data, shows new code size)
7. README rewrite + badges (uses size data, documents new features)
8. ESP32-C6 example (developed in parallel)
9. Documentation sweep (captures everything above)
10. Release pipeline rewrite + CI alignment (robust release flow)

## Notes

All tasks complete. After this release, focus shifts to developer
experience: C++ wrappers (Arduino and posix), simple "get started in 3
lines" examples, and content for articles and community forums. The
library works well once you understand it, but onboarding is too
industrial for casual users.

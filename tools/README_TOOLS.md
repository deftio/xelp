# tools/

Development and build tools for the xelp library. None of these tools are
required to use xelp in your project -- they support development, testing,
and cross-compilation workflows.

## Banner Generator

**`generate_banner.py`** -- Render text as ASCII art banners and optionally
export as C `#define` macros for embedding in header files.

Requires Python 3.7+. No external dependencies.

```
# Display a banner at the terminal
python3 tools/generate_banner.py "Hello World"

# Use the compact 3-row font
python3 tools/generate_banner.py "xelp" --font small

# Read ASCII art from a file
python3 tools/generate_banner.py -f my_logo.txt

# Output as a C #define macro
python3 tools/generate_banner.py "My App" --c-macro APP_BANNER_STR

# Write C macro to a file
python3 tools/generate_banner.py "My App" --c-macro APP_BANNER_STR -o banner.h

# Output just the escaped C string (no #define wrapper)
python3 tools/generate_banner.py "test" --raw

# Pad each line to a fixed width
python3 tools/generate_banner.py "xelp" -w 40
```

Fonts:

| Font | Rows | Description |
|------|------|-------------|
| `standard` | 5 | Default. Figlet-style block letters. |
| `small` | 3 | Compact font for constrained targets. |

Run `python3 tools/generate_banner.py --help` for full usage.

---

## Cross-Compilation Size Report

**`crossbuild.sh`** -- Build a Docker image with cross-compilers and run
xelp through all of them, reporting object file and `.text` section sizes
for each target architecture.

Requires Docker.

```
# Build image and run report
bash tools/crossbuild.sh

# Only rebuild the Docker image
bash tools/crossbuild.sh --build

# Only run the report (image must already exist)
bash tools/crossbuild.sh --run
```

Targets compiled:

| Target | Compiler |
|--------|----------|
| x86-64 | GCC, Clang |
| x86-32 | GCC, TCC |
| AArch64 (ARM64) | aarch64-linux-gnu-gcc |
| ARM32 (bare metal) | arm-none-eabi-gcc |
| ARM32 Thumb | arm-none-eabi-gcc -mthumb |
| MSP430 | msp430-gcc |
| AVR5 (ATmega328P) | avr-gcc |
| AVR ATtiny85 | avr-gcc |
| Xtensa LX106 (ESP8266) | xtensa-lx106-elf-gcc |
| Xtensa LX7 (ESP32-S3) | xtensa-esp-elf-gcc |
| 68HC11 | m68hc11-gcc |
| PowerPC | powerpc-linux-gnu-gcc |

The report also writes `build/sizes.csv` (CSV with columns:
`cpu,width,compiler,key,cli,full`).

Supporting files:

- **`Dockerfile.crossbuild`** -- Docker image definition (Ubuntu 22.04 + toolchains).
- **`compactbuilds-docker.sh`** -- The script that runs inside the container.
- **`compactbuilds.sh`** -- Original host-native version (requires all
  toolchains installed locally).

---

## Size Table Updater

**`update_sizes.sh`** -- Read `build/sizes.csv` and patch the compiled-size
tables in `README.md` and `pages/index.html`.

Tables are delimited by `<!-- BEGIN SIZE TABLE -->` / `<!-- END SIZE TABLE -->`
markers.  Rows are sorted by CPU width ascending (8 → 16 → 32 → 64), then
KEY size ascending within each group.

```
# After running crossbuild.sh:
bash tools/update_sizes.sh

# Preview without writing:
bash tools/update_sizes.sh --dry-run

# Explicit CSV path:
bash tools/update_sizes.sh path/to/sizes.csv
```

---

## Parser State Machine Generator

**`xelp_parser_sm_gen.py`** -- Generate the parser state machine transition
tables used by the tokenizer in `xelp.c`.

Requires Python 3.7+ (also works with Python 2.7).

```
# Generate default output files
python3 tools/xelp_parser_sm_gen.py

# Specify output filenames
python3 tools/xelp_parser_sm_gen.py -p my_tables.c -b my_tables_brkts.c
```

The state machine defines how the tokenizer walks through input characters,
handling whitespace, comments, quoted strings, escape sequences, and
statement terminators. The generator outputs two C files:

- **`xelp_psm_tables.c`** -- Standard parser state table.
- **`xelp_psm_tables_brkts.c`** -- Extended table with bracket support.

These are checked-in generated files. Re-run the generator only when
modifying the tokenizer's state machine logic.

Supporting files:

- **`make_sm_tab.c`** -- Original C version of the state machine table
  generator (superseded by the Python version).
- **`parser-sm-table.c`** -- Generated C output from `make_sm_tab.c`.
- **`runtab.sh`** -- Convenience script to compile and run `make_sm_tab.c`.

---

## Version Extractor

**`extract_version.c`** -- Tiny C program that `#include`s `xelp.h` and
emits the version as machine-readable YAML.  The version is resolved by the
C preprocessor, not by regex -- so it is always consistent with the compiled
library.

Requires only a C compiler (gcc, clang, tcc, etc.).

```
# Via make (writes build/xelp_version.yaml)
make version

# Manually
gcc tools/extract_version.c -Isrc -o build/extract_version
build/extract_version                      # YAML to stdout
build/extract_version build/version.yaml   # YAML to file
```

Output example:

```yaml
# Auto-generated from XELP_VERSION in src/xelp.h -- do not edit
version_hex: "0x00000201"
major: 0
minor: 2
patch: 1
version: "0.2.1"
tag: "v0.2.1"
```

Used by `make_release.sh` and the GitHub Actions release workflow.  Any
CI/CD step that needs the version should compile and run this tool rather
than parsing `xelp.h` directly.

---

## Release Script

**`make_release.sh`** -- Guided release pipeline. Walks through every step
from local validation to published GitHub Release, pausing for confirmation
before anything visible to others. Uses `extract_version.c` to read the
version from `xelp.h` via the C preprocessor.

```
# Full guided release (recommended)
bash tools/make_release.sh

# Local validation only (build, tests, coverage)
bash tools/make_release.sh --validate

# Full flow but create GH release locally (fallback if CI unavailable)
bash tools/make_release.sh --release-local
```

The full flow: extract version, validate build, check git state, push
branch, open PR, wait for CI, merge, switch to master, verify, tag, wait
for release.  Each step prints what it's doing and prompts before actions
that affect the remote.

See `tools/make-release.md` for detailed documentation and troubleshooting.

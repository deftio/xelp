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
| 68HC11 | m68hc11-gcc |
| PowerPC | powerpc-linux-gnu-gcc |

Supporting files:

- **`Dockerfile.crossbuild`** -- Docker image definition (Ubuntu 22.04 + toolchains).
- **`compactbuilds-docker.sh`** -- The script that runs inside the container.
- **`compactbuilds.sh`** -- Original host-native version (requires all
  toolchains installed locally).

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

## Release Script

**`make_release.sh`** -- Validate the build and optionally create a tagged
GitHub release. Reads the version from `XELP_VERSION` in `src/xelp.h`.

```
# Dry run: build, test, coverage check
bash tools/make_release.sh

# Validate + create git tag
bash tools/make_release.sh --tag

# Validate + tag + push + GitHub release (requires gh CLI)
bash tools/make_release.sh --release
```

The script checks:
- Clean build with zero warnings
- All tests pass
- Coverage report
- Working tree is clean (for tag/release mode)
- Tag doesn't already exist

See `release_management.md` for the full release workflow.

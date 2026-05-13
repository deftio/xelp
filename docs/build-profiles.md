# Build Profiles & Configuration Guide

xelp is a compile-time modular library. Every feature -- CLI mode, line
editing, single-key menus, pass-through, help -- is controlled by a
`#define` flag in `src/xelpcfg.h`. Features you leave out are stripped at
compile time, not just disabled at runtime. On a target with 8 KB of Flash,
paying 3 KB for a CLI you don't need isn't an option.

This guide explains the feature system, shows three ready-made profiles,
and documents every configuration knob.

## Three Profiles

Most projects fit one of three profiles. Pick the one closest to your needs
and adjust from there.

### KEY only (~1 KB)

Single-keypress dispatch. Each key triggers a function immediately -- no
ENTER, no prompt, no line buffer. Good for:

- Debug menus ("press L to toggle LED")
- Hardware test jigs
- Minimal footprint on tiny targets (ATtiny85, 8051)

```c
/* xelpcfg.h */
#define XELP_ENABLE_KEY   1
```

Representative size: **~550 bytes** on ARM Thumb, **~900 bytes** on AVR.

### CLI (~3-5 KB)

Line-buffered command prompt with line editing, cursor movement, multi-byte
ANSI key recognition, command dispatch, tokenizer, scripting, and help.
This is the typical interactive configuration.

```c
/* xelpcfg.h */
#define XELP_ENABLE_CLI        1
#define XELP_ENABLE_LINE_EDIT  1
#define XELP_ENABLE_KEY        1
#define XELP_ENABLE_HELP       1
```

Representative size: **~2600 bytes** on ARM Thumb, **~4200 bytes** on AVR.

### Full (~3-5 KB)

Everything in CLI plus THR (pass-through) mode, which forwards all
keystrokes to another peripheral -- a modem, radio module, or debug port.
Adds ~50-125 bytes over CLI.

```c
/* xelpcfg.h */
#define XELP_ENABLE_FULL  1
```

Or equivalently, add `XELP_ENABLE_THR` to the CLI profile.

Representative size: **~2650 bytes** on ARM Thumb, **~4250 bytes** on AVR.

Every flag is independent -- mix and match. For example, you can enable CLI
without KEY, or KEY without HELP.

## Feature Flag Reference

| Flag | What it enables | Requires | Size impact |
|------|----------------|----------|-------------|
| `XELP_ENABLE_KEY` | Single key press mode (menus, immediate actions) | -- | ~200-500 bytes |
| `XELP_ENABLE_CLI` | Command line prompt, backspace, command dispatch, scripting, tokenizer | -- | Core (~2 KB) |
| `XELP_ENABLE_LINE_EDIT` | Cursor movement (left/right, Home/End), insert-at-cursor, Delete, multi-byte ANSI key recognition | `XELP_ENABLE_CLI` | ~800-1000 bytes |
| `XELP_ENABLE_HISTORY` | Command history (UP/DOWN arrow recall of previous commands) | `XELP_ENABLE_CLI` + `XELP_ENABLE_LINE_EDIT` | ~420 bytes |
| `XELP_ENABLE_ARGV` | Structured argc/argv parsing (`XelpBuf2Argv`, `XELP_PARSE_ARGV`). Adds a per-instance scratch buffer. | -- | ~530-700 bytes + `XELP_ARGVBUFSZ` RAM |
| `XELP_ENABLE_THR` | Pass-through mode -- redirect all keys to another peripheral | -- | ~50-125 bytes |
| `XELP_ENABLE_HELP` | Built-in help command listing all registered commands | -- | ~180-350 bytes |
| `XELP_ENABLE_FULL` | Shorthand: enables KEY, CLI, THR, and HELP | -- | All combined |

`XELP_ENABLE_FULL` does *not* enable `XELP_ENABLE_LINE_EDIT`. To get line
editing with FULL, define both:

```c
#define XELP_ENABLE_FULL       1
#define XELP_ENABLE_LINE_EDIT  1
```

## Key Mappings

These define which key presses switch between modes:

| Define | Default | Purpose |
|--------|---------|---------|
| `XELPKEY_CLI` | CTRL-P (0x10) | Enter CLI mode |
| `XELPKEY_KEY` | ESC (0x1B) | Enter KEY mode |
| `XELPKEY_THR` | CTRL-T (0x14) | Enter THR mode |

Override in `xelpcfg.h`:

```c
#define XELPKEY_CLI  ('c')   /* now 'c' switches to CLI mode */
```

## Escape Characters

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_CLI_ESC` | `` ` `` (backtick) | Escape character at command line / in scripts |
| `XELP_QUO_ESC` | `\` (backslash) | Escape character inside quoted strings |

## Buffer and Register Sizes

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_CMDBUFSZ` | 64 | CLI input buffer size in bytes |
| `XELP_ARGVBUFSZ` | `XELP_CMDBUFSZ` | Scratch buffer size for `XelpBuf2Argv` (bytes per instance, only when `XELP_ENABLE_ARGV`). Override for variable expansion. |
| `XELP_ARGV_MAX` | 8 | Default max arguments for `XelpBuf2Argv` / `XELP_PARSE_ARGV`. |
| `XELP_HIST_DEPTH` | 4 | Number of commands stored in history ring (requires `XELP_ENABLE_HISTORY`). |
| `XELP_REGS_SZ` | 4 | Number of callee-clobbers-all return registers (minimum 4). R0 is command status, R1-R3 are command-specific. |
| `XELPREG` | `int` | Register type (change for platforms where `int` is not ideal) |

## Prompt

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_CLI_PROMPT` | `"xelp>"` | String shown at CLI prompt |

For per-instance prompts, set to `(ths->mpPrompt)` and assign at runtime:

```c
#define XELP_CLI_PROMPT  (ths->mpPrompt)
```

```c
XELP_SET_VAL_CLI_PROMPT(myXelp, "ser1>");
```

## Help Strings

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_HELP_KEY_STR` | `"\nKey functions\n"` | Header for KEY command help section |
| `XELP_HELP_CLI_STR` | `"\nCLI functions\n"` | Header for CLI command help section |
| `XELP_HELP_ABT_STR` | `(ths->mpAboutMsg)` | About message shown at top of help |

## Example xelpcfg.h for Each Profile

### KEY only

```c
#ifndef __XELP_CONFIG_H__
#define __XELP_CONFIG_H__

#ifdef XELP_CONFIG_OVERRIDE
#include "xelp_ovr.h"
#else

#define XELPKEY_CLI      (XELPKEY_CTP)
#define XELPKEY_KEY      (XELPKEY_ESC)

#define XELP_ENABLE_KEY  1

#define XELP_REGS_SZ    4
#define XELPREG int

#endif /* XELP_CONFIG_OVERRIDE */
#endif /* __XELP_CONFIG_H__ */
```

### CLI (typical)

```c
#ifndef __XELP_CONFIG_H__
#define __XELP_CONFIG_H__

#ifdef XELP_CONFIG_OVERRIDE
#include "xelp_ovr.h"
#else

#define XELPKEY_CLI      (XELPKEY_CTP)
#define XELPKEY_KEY      (XELPKEY_ESC)

#define XELP_CLI_ESC     ('`')
#define XELP_QUO_ESC     ('\\')

#define XELP_ENABLE_CLI        1
#define XELP_ENABLE_LINE_EDIT  1
#define XELP_ENABLE_KEY        1
#define XELP_ENABLE_HELP       1
#define XELP_ENABLE_HISTORY    1   /* optional: UP/DOWN arrow command recall */

#define XELP_CLI_PROMPT  "mydev>"

#define XELP_REGS_SZ    4
#define XELPREG int

#endif /* XELP_CONFIG_OVERRIDE */
#endif /* __XELP_CONFIG_H__ */
```

### Full

```c
#ifndef __XELP_CONFIG_H__
#define __XELP_CONFIG_H__

#ifdef XELP_CONFIG_OVERRIDE
#include "xelp_ovr.h"
#else

#define XELPKEY_CLI      (XELPKEY_CTP)
#define XELPKEY_KEY      (XELPKEY_ESC)
#define XELPKEY_THR      (XELPKEY_CTT)

#define XELP_CLI_ESC     ('`')
#define XELP_QUO_ESC     ('\\')

#define XELP_ENABLE_FULL       1
#define XELP_ENABLE_LINE_EDIT  1

#define XELP_CLI_PROMPT  "xelp>"

#define XELP_REGS_SZ    4
#define XELPREG int

#endif /* XELP_CONFIG_OVERRIDE */
#endif /* __XELP_CONFIG_H__ */
```

## Config Override for Multi-Target Builds

Define `XELP_CONFIG_OVERRIDE` in your compiler flags and create
`xelp_ovr.h` in your include path. The file is included *after* the
defaults in `xelpcfg.h`, so use `#undef` then `#define` to change values.
Anything you don't touch keeps its default.

```c
/* xelp_ovr.h example -- lean build for ATtiny */
#undef  XELP_ENABLE_CLI
#undef  XELP_ENABLE_LINE_EDIT
#undef  XELP_ENABLE_HISTORY
#undef  XELP_ENABLE_THR
#undef  XELP_ENABLE_HELP
#undef  XELP_ENABLE_ARGV
/* leaves only XELP_ENABLE_KEY */
```

Use cases:

- **Docker crossbuild** -- the `tools/Dockerfile.crossbuild` script defines
  different configs for each target architecture.
- **PlatformIO per-board configs** -- use `build_flags = -DXELP_CONFIG_OVERRIDE`
  in `platformio.ini` and provide a per-environment `xelp_ovr.h`.
- **Multi-target firmware** -- a single source tree builds for different
  hardware variants, each with its own feature set.

The 8.3 filename (`xelp_ovr.h`) is intentional for compatibility with
legacy filesystems on older embedded toolchains.

## See Also

- [Configuration Guide](configuration.md) -- concise quick-reference for all `#define` flags
- [API Reference](api-reference.md) -- all public functions, macros, and types
- [Porting Guide](porting.md) -- bringing up xelp on a new platform

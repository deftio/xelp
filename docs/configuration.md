# Configuration Guide

All compile-time options are controlled in `src/xelpcfg.h`. Comment in or
out `#define` lines to enable or disable features. Each feature that is
compiled out saves code space.

## Feature Flags

| Flag | Purpose | Size Impact |
|------|---------|-------------|
| `XELP_ENABLE_CLI` | Command line mode with prompt, backspace, and command dispatch | Required for CLI and scripting |
| `XELP_ENABLE_LINE_EDIT` | Cursor movement (left/right, Home/End), insert-at-cursor, Delete. Requires `XELP_ENABLE_CLI`. | ~800--1000 bytes |
| `XELP_ENABLE_KEY` | Single key press mode (menus, immediate actions) | ~200--500 bytes |
| `XELP_ENABLE_THR` | Pass-through mode (redirect keys to another peripheral) | ~50--125 bytes |
| `XELP_ENABLE_HELP` | Built-in help function listing all commands | ~180--350 bytes |
| `XELP_ENABLE_HISTORY` | Command history (UP/DOWN arrow recall). Requires `XELP_ENABLE_CLI` + `XELP_ENABLE_LINE_EDIT`. | ~420 bytes |
| `XELP_ENABLE_FULL` | Enable all of the above | All combined |

## Key Mappings

These define which key presses switch between modes at the command line:

| Define | Default | Purpose |
|--------|---------|---------|
| `XELPKEY_CLI` | CTRL-P (0x10) | Enter CLI mode |
| `XELPKEY_KEY` | ESC (0x1B) | Enter KEY mode |
| `XELPKEY_THR` | CTRL-T (0x14) | Enter THRU mode |

Override by redefining in `xelpcfg.h`, e.g. `#define XELPKEY_CLI ('c')`

## Escape Characters

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_CLI_ESC` | `` ` `` (backtick) | Escape character at command line / in scripts |
| `XELP_QUO_ESC` | `\` (backslash) | Escape character inside quoted strings |
| `XELP_ESC_MAP` | `"n\x0A" "t\x09" ""` | Packed key-value pairs for escape expansion inside double-quoted arguments during CLI dispatch tokenization. Each 2-byte entry maps the char after `XELP_QUO_ESC` to a replacement byte (e.g. `\n` -> newline, `\t` -> tab). Terminated by `'\0'`. Unmapped escapes pass through as identity (`\\` -> `\`, `\"` -> `"`). Set to `""` to disable expansion. |

## Buffer and Register Sizes

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_CMDBUFSZ` | 64 | CLI input buffer size in bytes |
| `XELP_ARGVBUFSZ` | `XELP_CMDBUFSZ` | Scratch buffer size for CLI dispatch argv tokenization (bytes per instance). Override to a larger value if variable expansion or long script lines may produce arguments longer than the CLI input buffer. Only allocated when `XELP_ENABLE_CLI` is defined. |
| `XELP_ARGV_MAX` | 8 | Maximum number of arguments for CLI dispatch tokenization. |
| `XELP_HIST_DEPTH` | 4 | Number of commands stored in history ring (requires `XELP_ENABLE_HISTORY`) |
| `XELP_REGS_SZ` | 4 | Number of callee-clobbers-all return registers (minimum 4). R0 is command status, R1-R3 are command-specific. |
| `XELPREG` | `int` | Register type (change for platforms where `int` is not ideal) |

## ENTER Key Detection

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_ENTER_LF` | 1 | Accept `\n` (0x0A) as ENTER |
| `XELP_ENTER_CR` | 1 | Accept `\r` (0x0D) as ENTER |

Both enabled by default for cross-platform use (some terminals send CR,
others LF, some send both). Only affects interactive input
(`XelpParseKey`); script parsing always uses `\n`.

## Prompt

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_CLI_PROMPT` | `"xelp>"` | String shown at CLI prompt |

For per-instance prompts, set to `(ths->mpPrompt)` and use
`XELP_SET_VAL_CLI_PROMPT(myXelp, "myPrompt>")` at runtime.

## Help Strings

| Define | Default | Purpose |
|--------|---------|---------|
| `XELP_HELP_KEY_STR` | `"\nKey functions\n"` | Header for KEY command help section |
| `XELP_HELP_CLI_STR` | `"\nCLI functions\n"` | Header for CLI command help section |
| `XELP_HELP_ABT_STR` | `(ths->mpAboutMsg)` | About message shown at top of help |

## Config Override

Define `XELP_CONFIG_OVERRIDE` in your compiler flags and create
`xelp_ovr.h` in your include path. The file is included *after* the
defaults in `xelpcfg.h`, so use `#undef` then `#define` to change values.
Anything you don't touch keeps its default.

```c
/* xelp_ovr.h example */
#undef  XELP_CLI_PROMPT
#define XELP_CLI_PROMPT   (ths->mpPrompt)

#undef  XELP_ENABLE_THR    /* disable THR mode */

#undef  XELP_CMDBUFSZ
#define XELP_CMDBUFSZ  128 /* larger input buffer */
```

## Example Configurations

### Minimal (KEY mode only)

```c
/* xelpcfg.h */
#define XELP_ENABLE_KEY   1
/* Leave CLI, LINE_EDIT, THR, HELP undefined */
```

Estimated size: ~900 bytes

### CLI with help

```c
#define XELP_ENABLE_CLI   1
#define XELP_ENABLE_HELP  1
```

Estimated size: ~2 KB

### Full

```c
#define XELP_ENABLE_FULL  1
```

Estimated size: ~3--4 KB (platform dependent)

# Xelp 2.0 Modernization Plan

## Executive Summary

This document outlines a comprehensive plan to modernize the xelp embedded CLI library while preserving its core strengths: char-at-a-time parsing, minimal footprint, multi-instance support, and no dynamic memory allocation.

**Key Goals:**
1. Clean up file layout and improve documentation
2. Achieve 100% test coverage with robust edge case handling
3. Unify command structures (KEY and CLI) while keeping separate namespaces
4. Add optional scripting support with static memory management

**Guiding Principles:**
- All changes are backward-compatible or provide migration path
- Memory remains 100% static (no malloc, ever)
- Multi-instance support preserved
- Code size impact is documented and optional features are compile-time selectable

---

## Table of Contents

1. [Current Repository Analysis](#1-current-repository-analysis)
2. [File Layout Modernization](#2-file-layout-modernization)
3. [Documentation Plan](#3-documentation-plan)
4. [Test Coverage Strategy](#4-test-coverage-strategy)
5. [Unified Command Structure](#5-unified-command-structure)
6. [Scripting Strategy](#6-scripting-strategy)
7. [Implementation Phases](#7-implementation-phases)

---

## 1. Current Repository Analysis

### Existing Structure

```
xelp/
├── dev/                    # Development fragments, old files
├── docs/                   # Currently just ASCII table
├── examples/
│   ├── arduino/           # Arduino example
│   └── posix-simple/      # POSIX ncurses example
├── img/                    # Icons and branding (extensive)
├── libs/                   # bitwrench JS/CSS for web stuff
├── src/                    # Core library
│   ├── xelp.c
│   ├── xelp.h
│   ├── xelpcfg.h
│   └── compactbuilds.sh
├── tests/                  # Unit tests with jumpbug framework
│   ├── jumpbug_unit_test_fw.*
│   ├── xelp_unit_tests.c
│   └── various coverage files
├── tools/                  # Parser state machine generator
│   ├── xelp_parser_sm_gen.py
│   └── generated tables
├── index.html             # Project web page
├── makefile
├── README.md
└── LICENSE.txt
```

### What's Working Well

- **src/**: Clean separation of core library
- **tools/**: Parser generator is separate and well-organized
- **tests/**: Has test framework (jumpbug) and some coverage
- **examples/**: Good platform separation (arduino, posix)
- **img/**: Complete branding assets

### What Needs Improvement

- **dev/**: Old fragments mixed with design docs - needs cleanup
- **docs/**: Nearly empty - needs comprehensive documentation
- **tests/**: Coverage appears incomplete, needs systematic expansion
- **.gcda/.gcno/.o files**: Build artifacts in source directories
- **Generated files in repo**: parser-sm-table.c etc. could be .gitignored

---

## 2. File Layout Modernization

### Proposed Structure

```
xelp/
├── README.md                    # Project overview (update existing)
├── LICENSE.txt                  # Keep as-is
├── CHANGELOG.md                 # NEW: Version history
├── CONTRIBUTING.md              # NEW: Contribution guidelines
├── makefile                     # Update for new structure
├── index.html                   # Keep: project web page
│
├── src/
│   ├── xelp.h                   # Public API (keep)
│   ├── xelp.c                   # Implementation (keep)
│   ├── xelp_cfg.h               # RENAME from xelpcfg.h (underscore)
│   └── xelp_builtins.c          # NEW: Built-in script commands (optional)
│
├── tests/
│   ├── jumpbug_unit_test_fw.h   # Keep: test framework
│   ├── jumpbug_unit_test_fw.c   # Keep: test framework
│   ├── xelp_unit_tests.c        # EXPAND: comprehensive tests
│   ├── test_tokenizer.c         # NEW: tokenizer-specific tests
│   ├── test_dispatch.c          # NEW: command dispatch tests
│   ├── test_interactive.c       # NEW: char-by-char tests
│   ├── test_scripting.c         # NEW: scripting tests (optional)
│   ├── test_edge_cases.c        # NEW: adversarial input tests
│   ├── maketest.sh              # Keep
│   └── run_coverage_test.sh     # Keep
│
├── examples/
│   ├── arduino/
│   │   ├── arduino_example.ino  # RENAME (underscore)
│   │   └── README.md
│   ├── posix/                   # RENAME from posix-simple
│   │   ├── xelp_example.c       # RENAME (underscore)
│   │   └── makefile             # NEW: standalone makefile
│   ├── minimal/                 # NEW: bare minimum example
│   │   ├── minimal_example.c
│   │   └── makefile
│   └── scripts/                 # NEW: example xelp scripts
│       ├── blink.xelp
│       ├── menu.xelp
│       └── config.xelp
│
├── tools/
│   ├── xelp_parser_sm_gen.py    # Keep
│   ├── compactbuilds.sh         # MOVE from src/
│   └── size_report.sh           # NEW: generate size comparison
│
├── docs/
│   ├── API.md                   # NEW: complete API reference
│   ├── CONFIGURATION.md         # NEW: all compile-time options
│   ├── PORTING.md               # NEW: platform porting guide
│   ├── SCRIPTING.md             # NEW: scripting language reference
│   ├── ARCHITECTURE.md          # NEW: internal design notes
│   └── ASCII_Table.md           # Keep
│
├── dev/                         # Development/experimental
│   ├── DESIGN_NOTES.md          # CONSOLIDATE from lang_design.md, etc.
│   └── TODO.md                  # RENAME from xelp-TODO.md
│
├── img/                         # Keep as-is (branding)
│
├── libs/                        # Keep as-is (web dependencies)
│
└── .github/
    └── workflows/
        └── ci.yml               # NEW: GitHub Actions CI
```

### Changes Summary

**Renames (consistency):**
- `xelpcfg.h` → `xelp_cfg.h` (underscore convention)
- `posix-simple/` → `posix/` (simpler)
- `arduino-example.ino` → `arduino_example.ino`
- `xelp-example.c` → `xelp_example.c`

**Moves:**
- `src/compactbuilds.sh` → `tools/compactbuilds.sh`

**New Files:**
- Documentation: API.md, CONFIGURATION.md, PORTING.md, SCRIPTING.md, ARCHITECTURE.md
- Tests: Multiple focused test files
- Examples: minimal/, scripts/
- CI: .github/workflows/ci.yml
- Root: CHANGELOG.md, CONTRIBUTING.md

**Cleanup:**
- Add `*.o`, `*.gcda`, `*.gcno`, `*.out` to .gitignore
- Consolidate dev/ fragments into DESIGN_NOTES.md
- Remove or archive obsolete files

### .gitignore Additions

```gitignore
# Build artifacts
*.o
*.out
*.gcda
*.gcno
*.gcov

# Generated files (regenerate with tools/)
tools/xelp_psm_tables.c
tools/xelp_psm_tables_brkts.c
tools/parser-sm-table.c

# Editor
*.swp
*~
.vscode/

# OS
.DS_Store
Thumbs.db
```

---

## 3. Documentation Plan

### README.md Updates

Current README needs:
- Quick start section (3 steps: include, init, feed chars)
- Feature summary with code sizes
- Platform support table
- Build badges (CI, coverage)
- Links to detailed docs

**Structure:**
```markdown
# xelp - Embedded CLI Library

[badges: CI status, coverage, license]

One-paragraph description emphasizing char-at-a-time, minimal footprint.

## Features
- Bullet list with code size impacts

## Quick Start
- 3-step example

## Supported Platforms
- Table with tested platforms and sizes

## Documentation
- Links to docs/

## Building
- Basic make instructions

## License
```

### docs/API.md

Complete API reference:

**Types:**
- `Xelp` - instance structure
- `XelpResult` - return codes
- `XelpCmd` - command entry (new unified structure)
- `XelpBuf` - buffer wrapper

**Functions:**
- `xelp_init()` - initialize instance
- `xelp_char()` - feed single character
- `xelp_parse_key()` - legacy name, same as xelp_char
- `XelpOut()` - output string
- `XelpHelp()` - print help
- `xelp_dispatch()` - command dispatch (new)
- Token functions: `XelpTokN()`, `XelpNumToks()`, `xelp_streq()`, `xelp_atoi()`

**Macros:**
- `XELP_SET_FN_OUT()` - set output function
- `XELP_SET_FN_CLI()` - set CLI command table
- `XELP_SET_FN_KEY()` - set KEY command table
- `XELP_SET_VAL_CLI_PROMPT()` - set prompt string
- All other SET macros

### docs/CONFIGURATION.md

All compile-time options:

**Feature Flags:**
- `XELP_ENABLE_CLI` - command line mode
- `XELP_ENABLE_KEY` - single key mode
- `XELP_ENABLE_THR` - passthrough mode
- `XELP_ENABLE_HELP` - built-in help
- `XELP_ENABLE_LCORE` - peek/poke/go (dangerous)
- `XELP_ENABLE_VARS` - variables (new)
- `XELP_ENABLE_SCRIPT` - scripting (new)
- `XELP_ENABLE_MATH` - math operations (new)
- `XELP_ENABLE_FORTH` - FORTH mode (new)

**Size Parameters:**
- `XELP_CMDBUFSZ` - command buffer size
- `XELP_REGS_SZ` - register count
- `XELP_STACK_DEPTH` - stack depth
- `XELP_VAR_COUNT` - variable slots (new)
- `XELP_VAR_NAME_LEN` - max variable name (new)
- `XELP_LABEL_COUNT` - label slots (new)

**Platform Settings:**
- Key mappings (XELPKEY_CLI, etc.)
- Escape characters
- Prompt configuration

**Example Configurations:**
- Minimal (CLI only): ~1.5KB
- Default: ~2.5KB
- Full: ~4KB

### docs/PORTING.md

How to port to new platform:

**Required:**
- Implement output function: `void putc(char c)`
- Call `xelp_init()` at startup
- Feed characters via `xelp_char()`

**Optional:**
- Backspace handler
- Mode change callback
- Error output function
- Platform-specific config in xelp_cfg.h

**Platform Notes:**
- AVR: Use PROGMEM for strings
- 8051: Use __code, __reentrant
- ARM: Usually just works
- MSP430: Watch for int size

**Tested Platforms:**
- Table with compiler, settings, notes

### docs/SCRIPTING.md

*(New - for when scripting is implemented)*

**Syntax:**
- Variables: `_set`, `$var`, `$r0`
- Control: `_if`, `_else`, `_endif`, `_goto`, `label:`
- Math: `_inc`, `_dec`, `_add`, `_sub`, `_and`, `_or`, `_xor`, `_not`, `_shl`, `_shr`
- Comparison: `_eq`, `_lt`, `_gt`
- Debug: `_vars`, `_regs`

**Examples:**
- Simple loop
- Conditional logic
- State machine

**Memory Usage:**
- Per-variable cost
- Per-label cost
- Configuration options

### docs/ARCHITECTURE.md

*(For contributors)*

**Parser Design:**
- State machine explanation
- Why Python generator
- State transition table format

**Instance Design:**
- Why no globals
- Memory layout
- HAL abstraction

**Why Char-at-a-Time:**
- Serial/BLE compatibility
- No buffering required
- Interrupt-safe feeding

---

## 4. Test Coverage Strategy

### Current State

The existing `xelp_unit_tests.c` with jumpbug framework provides a foundation. Coverage files (.gcda, .gcno, .gcov) indicate some coverage testing exists.

### Target: 100% Coverage

**Line Coverage:** Every line of code executed at least once
**Branch Coverage:** Every conditional branch taken both ways
**State Coverage:** Every parser state visited, every transition exercised

### Test Organization

#### test_tokenizer.c - Parser State Machine Tests

**Basic Tokenization:**
```
test_tok_empty              ""                    → 0 tokens
test_tok_whitespace         "   "                 → 0 tokens
test_tok_single             "hello"               → 1 token
test_tok_multiple           "a b c"               → 3 tokens
test_tok_leading_space      "  hello"             → 1 token (trimmed)
test_tok_trailing_space     "hello  "             → 1 token (trimmed)
test_tok_multiple_spaces    "a    b"              → 2 tokens
test_tok_tabs               "a\tb\tc"             → 3 tokens
test_tok_mixed_whitespace   "a \t b"              → 2 tokens
```

**Quoted Strings:**
```
test_tok_quoted_simple      '"hello"'             → 1 token with quotes
test_tok_quoted_spaces      '"hello world"'       → 1 token (space inside)
test_tok_quoted_empty       '""'                  → 1 token (empty string)
test_tok_quoted_adjacent    '"a""b"'              → 2 tokens
test_tok_quoted_mixed       'echo "hello" done'   → 3 tokens
test_tok_quoted_unterminated '"hello             → defined behavior (error or partial)
```

**Escape Sequences (CLI level - backtick):**
```
test_tok_esc_space          'a` b'                → "a b" single token
test_tok_esc_quote          'a`"b'                → 'a"b'
test_tok_esc_semicolon      'a`;b'                → "a;b"
test_tok_esc_hash           'a`#b'                → "a#b"
test_tok_esc_escape         'a``b'                → "a`b"
test_tok_esc_newline        'a`\nb'               → "a\nb" (or behavior?)
test_tok_esc_at_end         'hello`'              → defined behavior
```

**Escape Sequences (inside quotes - backslash):**
```
test_tok_qesc_quote         '"a\"b"'              → string containing quote
test_tok_qesc_backslash     '"a\\b"'              → string containing backslash
test_tok_qesc_at_end        '"hello\'             → defined behavior
```

**Comments:**
```
test_tok_comment_full       '# comment'           → 0 tokens
test_tok_comment_after      'cmd # comment'       → 1 token
test_tok_comment_hash_quoted '"#notcomment"'      → 1 token (hash preserved)
test_tok_comment_hash_esc   'a`#b'                → 1 token "a#b"
test_tok_comment_no_newline 'cmd #comment'        → 1 token (no trailing \n)
```

**Statement Separators:**
```
test_tok_semicolon          'a;b'                 → 2 statements
test_tok_newline            'a\nb'                → 2 statements
test_tok_multi_semi         'a;;b'                → 2 statements (skip empty)
test_tok_semi_only          ';;;'                 → 0 statements
test_tok_mixed_sep          'a;b\nc'              → 3 statements
test_tok_crlf               'a\r\nb'              → 2 statements
```

**Token Extraction:**
```
test_tok_get_0              'a b c'               → token 0 is "a"
test_tok_get_1              'a b c'               → token 1 is "b"
test_tok_get_2              'a b c'               → token 2 is "c"
test_tok_get_beyond         'a b'                 → token 5 fails gracefully
test_tok_count              'a b c d'             → count is 4
test_tok_count_empty        ''                    → count is 0
```

#### test_strings.c - String Utility Tests

**xelp_strlen:**
```
test_strlen_empty           ""                    → 0
test_strlen_one             "a"                   → 1
test_strlen_normal          "hello"               → 5
test_strlen_with_null       "hel\0lo"             → 3 (stops at null)
```

**xelp_streq:**
```
test_streq_equal            ("hello", 5, "hello") → OK
test_streq_diff             ("hello", 5, "world") → NOT_FOUND
test_streq_len_short        ("hello", 4, "hello") → NOT_FOUND
test_streq_len_long         ("hello", 6, "hello") → NOT_FOUND
test_streq_empty            ("", 0, "")           → OK
test_streq_one_empty        ("a", 1, "")          → NOT_FOUND
test_streq_prefix           ("hello", 5, "helloX")→ NOT_FOUND
```

**xelp_atoi (decimal):**
```
test_atoi_zero              "0"                   → 0
test_atoi_positive          "123"                 → 123
test_atoi_negative          "-456"                → -456
test_atoi_plus              "+789"                → 789
test_atoi_max               "2147483647"          → INT_MAX (32-bit)
test_atoi_min               "-2147483648"         → INT_MIN (32-bit)
test_atoi_leading_zero      "007"                 → 7
test_atoi_empty             ""                    → 0 (defined behavior)
test_atoi_nonnumeric        "abc"                 → 0 (defined behavior)
test_atoi_partial           "123abc"              → 123 (stops at non-digit)
```

**xelp_atoi (hex):**
```
test_atoi_hex_0x            "0x10"                → 16
test_atoi_hex_0X            "0X10"                → 16
test_atoi_hex_lower         "0xff"                → 255
test_atoi_hex_upper         "0xFF"                → 255
test_atoi_hex_mixed         "0xAbCd"              → 43981
test_atoi_hex_h_suffix      "10h"                 → 16
test_atoi_hex_H_suffix      "FFH"                 → 255
test_atoi_hex_0x_only       "0x"                  → 0
test_atoi_hex_max           "0xFFFFFFFF"          → -1 or overflow behavior
```

#### test_dispatch.c - Command Dispatch Tests

**Basic Dispatch:**
```
test_dispatch_found         "help"                → help function called
test_dispatch_notfound      "unknown"             → XELP_NOT_FOUND
test_dispatch_empty         ""                    → no dispatch
test_dispatch_whitespace    "   "                 → no dispatch
test_dispatch_case          "HELP" vs "help"      → case sensitive (not found)
```

**Arguments:**
```
test_dispatch_with_args     "echo hello world"    → function receives full line
test_dispatch_arg_ptr       verify args pointer points to start
test_dispatch_arg_len       verify args length is correct
```

**Return Values:**
```
test_dispatch_return_ok     command returns OK    → r[0] = OK
test_dispatch_return_err    command returns ERR   → r[0] = ERR
test_dispatch_return_val    command returns 42    → r[0] = 42
```

**Table Operations:**
```
test_dispatch_null_table    NULL table            → no crash, NOT_FOUND
test_dispatch_empty_table   just terminator       → NOT_FOUND
test_dispatch_swap          change table          → new commands work
test_dispatch_swap_old      after swap            → old commands not found
```

**KEY Mode (single char):**
```
test_key_dispatch_found     'h'                   → key function called
test_key_dispatch_notfound  'z'                   → NOT_FOUND
test_key_all_printable      test 0x20-0x7E range
test_key_control_chars      test 0x01-0x1F range
```

#### test_interactive.c - Char-at-a-Time Tests

**Basic Input:**
```
test_char_command_lf        "help\n" char by char → command executes
test_char_command_cr        "help\r"              → command executes
test_char_command_crlf      "help\r\n"            → executes once (not twice)
test_char_incremental       feed h, e, l, p, \n   → same result
```

**Backspace:**
```
test_char_backspace_mid     "helx" + BS + "p\n"   → "help" executes
test_char_backspace_start   BS at empty line      → ignored, no crash
test_char_backspace_multi   "abcd" + 4×BS + "x\n" → "x" executes
test_char_backspace_past    10×BS on "abc"        → buffer empty, no crash
test_char_bs_callback       verify backspace callback called
```

**Buffer Limits:**
```
test_char_buffer_fill       fill to XELP_CMDBUFSZ-1 → OK
test_char_buffer_exact      fill to XELP_CMDBUFSZ   → at limit
test_char_buffer_over       exceed buffer          → truncate, no overflow
test_char_buffer_bs_over    fill, BS, add          → still within limit
```

**Mode Switching:**
```
test_mode_to_cli            send CTRL-P           → mode changes to CLI
test_mode_to_key            send ESC              → mode changes to KEY
test_mode_to_thru           send CTRL-T           → mode changes to THRU
test_mode_callback          verify callback called with correct mode
test_mode_mid_command       start command, switch, switch back → ?
```

**Echo:**
```
test_echo_enabled           with echo on          → chars echoed
test_echo_disabled          with echo off         → chars not echoed
test_prompt                 after command         → prompt printed
```

#### test_edge_cases.c - Adversarial Input Tests

**Null/Empty:**
```
test_edge_null_instance     NULL Xelp*            → no crash (or assert)
test_edge_uninit_instance   uninitialized Xelp    → defined behavior
test_edge_zero_length       zero length strings   → handled gracefully
test_edge_null_callbacks    NULL function pointers→ no crash on call
```

**Long Inputs:**
```
test_edge_long_command      1000 char command     → truncate safely
test_edge_long_token        single 200 char token → handled
test_edge_many_tokens       100 tokens            → handled or limited
```

**Binary/Special:**
```
test_edge_null_byte         0x00 in input         → terminates or handled
test_edge_all_bytes         each 0x00-0xFF        → no crash
test_edge_high_bit          0x80-0xFF chars       → passed through
test_edge_control           control chars 0x01-0x1F → defined behavior
```

**Resource Exhaustion (when scripting enabled):**
```
test_edge_fill_vars         fill all variable slots → defined behavior on next
test_edge_fill_labels       fill all label slots    → defined behavior on next
test_edge_deep_if           max nesting depth       → handled or error
```

**Memory Safety:**
```
test_edge_buffer_canary     check for buffer overflows
test_edge_stack_depth       verify bounded stack usage
test_edge_no_malloc         verify no dynamic allocation
```

#### test_integration.c - Complete Scenarios

**Multi-Command:**
```
test_int_multi_line         multiple commands via \n
test_int_multi_semi         multiple commands via ;
test_int_mixed              mixed separators
```

**Multi-Instance:**
```
test_int_two_instances      two Xelp instances    → independent state
test_int_shared_tables      shared command tables → no interference
test_int_separate_tables    separate tables       → correct dispatch each
```

**Real Scripts (when scripting enabled):**
```
test_int_blink_script       LED blink with loop
test_int_config_script      multiple variable sets
test_int_menu_script        state machine with table swap
```

### Test Infrastructure Updates

**Extend jumpbug or add helpers:**

```c
/* Output capture */
static char g_test_output[512];
static int g_test_output_pos;

void test_putc(char c) {
    if (g_test_output_pos < sizeof(g_test_output) - 1)
        g_test_output[g_test_output_pos++] = c;
    g_test_output[g_test_output_pos] = '\0';
}

void test_reset_output(void) {
    g_test_output_pos = 0;
    g_test_output[0] = '\0';
}

#define ASSERT_OUTPUT(expected) \
    ASSERT_STR_EQ(g_test_output, expected)

#define ASSERT_OUTPUT_CONTAINS(substr) \
    ASSERT(strstr(g_test_output, substr) != NULL)

/* Char-by-char feeding */
void test_feed_str(Xelp* x, const char* s) {
    while (*s) xelp_char(x, *s++);
}

void test_feed_line(Xelp* x, const char* s) {
    test_feed_str(x, s);
    xelp_char(x, '\n');
}
```

### Coverage Tooling

**Existing:** gcov files present, run_coverage_test.sh exists

**Enhancements:**
- Add lcov for HTML reports
- Add coverage badge generation
- CI uploads to codecov.io
- Fail CI if coverage drops below threshold

**Makefile target:**
```makefile
coverage:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) --coverage" test
	gcov src/xelp.c
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_html
	@echo "Coverage report: coverage_html/index.html"
```

---

## 5. Unified Command Structure

### Current Structures

```c
/* Single-key commands */
typedef struct {
    XELPRESULT (*fn)(int);
    char key;
    char* help;
} XELPKeyFuncMapEntry;

/* CLI commands */
typedef struct {
    XELPRESULT (*fn)(const char*, int);
    char* cmd;
    char* help;
} XELPCLIFuncMapEntry;
```

### Proposed Unified Structure

```c
typedef struct {
    XelpResult (*fn)(Xelp* x, const char* args, int len);
    const char* name;   /* "h" for key, "help" for CLI */
    const char* help;   /* NULL to save space */
} XelpCmd;
```

### Rationale

**Why unify structure:**
- Single dispatch function (code size savings)
- Single function signature for all commands
- Commands can access instance state via `Xelp* x`
- Simpler mental model and documentation

**Why keep two table pointers:**
- Separate namespaces: "h" (key) vs "help" (CLI) don't collide
- Independent table swapping (e.g., different key menus)
- Some commands only make sense in one mode

**Why new function signature:**
- `Xelp* x` parameter gives access to instance (registers, variables, output)
- Uniform `(args, len)` works for both (KEY: args is single char)
- Enables commands to call xelp functions

### Migration Path

**Phase 1: Add new alongside old**

```c
/* New structure */
typedef struct {
    XelpResult (*fn)(Xelp* x, const char* args, int len);
    const char* name;
    const char* help;
} XelpCmd;

/* Legacy aliases (deprecated but working) */
#define XELPKeyFuncMapEntry XelpCmd_LEGACY_KEY
#define XELPCLIFuncMapEntry XelpCmd_LEGACY_CLI
```

**Phase 2: Add unified dispatch**

```c
XelpResult xelp_dispatch(Xelp* x, const XelpCmd* table,
                         const char* name, int name_len,
                         const char* args, int args_len)
{
    while (table->fn) {
        if (xelp_streq(name, name_len, table->name) == XELP_OK) {
            XelpResult r = table->fn(x, args, args_len);
            x->r[0] = r;
            return r;
        }
        table++;
    }
    return XELP_NOT_FOUND;
}
```

**Phase 3: Compatibility wrappers**

```c
/* For users who have old-style KEY functions: XELPRESULT fn(int key) */
#define XELP_WRAP_KEY_FN(name, old_fn) \
    XelpResult name(Xelp* x, const char* args, int len) { \
        (void)x; (void)len; \
        return (XelpResult)old_fn((int)*args); \
    }

/* For users who have old-style CLI functions: XELPRESULT fn(char*, int) */
#define XELP_WRAP_CLI_FN(name, old_fn) \
    XelpResult name(Xelp* x, const char* args, int len) { \
        (void)x; \
        return (XelpResult)old_fn(args, len); \
    }
```

**Phase 4: Deprecation warnings (future major version)**

```c
#if defined(__GNUC__)
#define XELP_DEPRECATED __attribute__((deprecated("Use XelpCmd instead")))
#elif defined(_MSC_VER)
#define XELP_DEPRECATED __declspec(deprecated("Use XelpCmd instead"))
#else
#define XELP_DEPRECATED
#endif

typedef struct { ... } XELPKeyFuncMapEntry XELP_DEPRECATED;
```

### New Usage Pattern

```c
/* Command implementation */
XelpResult cmd_echo(Xelp* x, const char* args, int len)
{
    XelpBuf buf, tok;
    int i, n;
    
    XELP_BUF_INIT(buf, args, len);
    XelpNumToks(&buf, &n);
    
    /* Skip command name, print rest */
    for (i = 1; i < n; i++) {
        XELP_BUF_RESET(buf);
        XelpTokN(&buf, i, &tok);
        XelpOut(x, tok.s, tok.p - tok.s);
        if (i < n - 1) XelpOut(x, " ", 1);
    }
    XelpOut(x, "\n", 1);
    
    return XELP_OK;
}

/* KEY commands - note single-char names */
const XelpCmd my_key_cmds[] = {
    {cmd_help_key,  "h", "Help"},
    {cmd_exit,      "x", "Exit"},
    {cmd_toggle,    "t", "Toggle LED"},
    {NULL, NULL, NULL}
};

/* CLI commands */
const XelpCmd my_cli_cmds[] = {
    {cmd_help_cli,  "help",  "Show help"},
    {cmd_echo,      "echo",  "Print arguments"},
    {cmd_set,       "_set",  "Set variable"},   /* Underscore = built-in */
    {NULL, NULL, NULL}
};

/* Setup */
Xelp x;
xelp_init(&x, about_string);
XELP_SET_FN_OUT(x, uart_putc);
XELP_SET_KEY_CMDS(x, my_key_cmds);
XELP_SET_CLI_CMDS(x, my_cli_cmds);
```

### Convenience Macros

```c
/* Table terminator */
#define XELP_CMD_END  {NULL, NULL, NULL}

/* Entry without help (saves ROM) */
#define XELP_CMD(fn, name)  {fn, name, NULL}

/* Entry with help */
#define XELP_CMD_HELP(fn, name, help)  {fn, name, help}

/* Verify table is sorted (for optional binary search) */
#ifdef XELP_USE_BSEARCH
#define XELP_CMD_TABLE_SORTED  /* marker for documentation */
#endif
```

---

## 6. Scripting Strategy

### Design Philosophy

Scripting in xelp must respect the core constraints:
- **All memory static:** No malloc, everything in instance struct
- **Multi-instance safe:** Each instance has independent state
- **Compile-time optional:** Don't pay for what you don't use
- **Readable syntax:** Bash-style, not exotic

### Approach: Direct Interpreter with Static Storage

Scripts are parsed and executed line-by-line, like bash. Variables and labels are stored in fixed-size tables within the Xelp instance.

### Built-in Command Namespace

All scripting built-ins use underscore prefix to avoid collision with user commands:

```
_set    _if     _else   _endif   _goto
_inc    _dec    _add    _sub
_and    _or     _xor    _not     _shl    _shr
_eq     _lt     _gt
_echo   _vars   _regs
```

Users are free to define `set`, `if`, etc. for their own purposes without conflict.

### Memory Structures

**Variables:**
```c
typedef struct {
    char name[XELP_VAR_NAME_LEN];  /* e.g., 8 chars */
    XelpReg value;                  /* Platform int */
} XelpVar;

/* In Xelp instance: */
XelpVar vars[XELP_VAR_COUNT];      /* e.g., 8 slots */
uint8_t var_count;                  /* Current count */
```

Memory cost: 8 vars × 12 bytes = 96 bytes

**Labels:**
```c
typedef struct {
    char name[XELP_LABEL_NAME_LEN];  /* e.g., 8 chars */
    const char* pos;                  /* Pointer into script */
} XelpLabel;

/* In Xelp instance: */
XelpLabel labels[XELP_LABEL_COUNT];  /* e.g., 8 slots */
uint8_t label_count;
```

Memory cost: 8 labels × (8 + ptr_size) = ~96 bytes

**Execution State:**
```c
/* In Xelp instance: */
const char* script_pos;    /* Current position */
const char* script_end;    /* End of script */
uint8_t if_depth;          /* Nesting level */
uint8_t skip_depth;        /* Skip if false */
uint8_t in_else;           /* In else branch */
```

Memory cost: ~12 bytes

**Total Scripting Overhead:** ~200-250 bytes RAM per instance

### Variable Syntax

**Setting:**
```
_set count 10          # Integer literal
_set count 0xFF        # Hex literal
_set count $other      # Copy from variable
_set r0 99             # Set register directly
```

**Reading:**
```
$count                 # Variable value
$r0, $r1, ... $r7      # Register values
```

**Built-in names:**
- `$r0` through `$r7` - registers
- `$?` - last command result (same as $r0)

### Control Flow

**Labels (suffix colon, natural style):**
```
loop:                  # Define label
    ...
```

**Goto:**
```
_goto loop             # Jump to label
```

**Conditionals:**
```
_if $count             # Execute block if non-zero
    echo "count is set"
_else
    echo "count is zero"
_endif
```

**Nested conditionals work correctly:**
```
_if $a
    _if $b
        echo "both"
    _endif
_endif
```

### Math Operations

**Increment/Decrement:**
```
_inc count             # count = count + 1
_dec count             # count = count - 1
```

**Binary Operations (3-operand):**
```
_add result $a $b      # result = a + b
_sub result $a $b      # result = a - b
_and result $a $b      # result = a & b
_or  result $a $b      # result = a | b
_xor result $a $b      # result = a ^ b
_not result $a         # result = ~a
_shl result $a $n      # result = a << n
_shr result $a $n      # result = a >> n
```

**Why 3-operand:** No expression parser needed. Simple, clear, assembly-like.

**Why no multiply/divide:** Too expensive on 8-bit MCUs (pulls in library code). Users can provide their own if needed.

### Comparison Operations

```
_eq result $a $b       # result = (a == b) ? 1 : 0
_lt result $a $b       # result = (a < b) ? 1 : 0
_gt result $a $b       # result = (a > b) ? 1 : 0
```

**Usage:**
```
_lt tmp $count 10
_if $tmp
    echo "count less than 10"
_endif
```

### Example Script

```bash
# Blink LED 5 times
_set count 5

loop:
    led on              # User command
    delay 500           # User command
    led off
    delay 500
    _dec count
    _if $count
        _goto loop
    _endif

_echo "Done blinking"
```

### FORTH Mode (Separate Option)

For users who want stack-based computation, a separate FORTH mode can be enabled. This is NOT a modified FORTH—it's standard FORTH syntax, because:

1. FORTH users expect FORTH syntax
2. Trying to make FORTH "readable" creates a new language nobody knows
3. Standard FORTH has decades of documentation

**FORTH mode would add:**
```c
XelpReg stack[XELP_STACK_DEPTH];  /* e.g., 16 deep */
uint8_t sp;
```

Memory cost: 16 × sizeof(int) + 1 = ~65 bytes

**FORTH built-ins:**
```
dup drop swap over rot
+ - * / mod
and or xor not
= < >
. emit cr
```

**Example (standard FORTH):**
```forth
5 0 do led-toggle 500 delay loop
```

**Not recommended to mix:** Keep bash-style and FORTH as separate modes. Mixing creates confusion.

### Configuration Flags

```c
/* Enable bash-style scripting */
#define XELP_ENABLE_SCRIPT

/* Enable math operations (requires SCRIPT) */
#define XELP_ENABLE_MATH

/* Enable comparison operations (requires SCRIPT) */
#define XELP_ENABLE_CMP

/* Enable FORTH stack machine (independent of SCRIPT) */
#define XELP_ENABLE_FORTH

/* Sizing */
#define XELP_VAR_COUNT       8
#define XELP_VAR_NAME_LEN    8
#define XELP_LABEL_COUNT     8
#define XELP_STACK_DEPTH     16
```

### String Variables (Deferred)

Integer-only variables are sufficient for most embedded scripting. String handling adds significant complexity:

- String pool management
- Fragmentation
- Concatenation
- Comparison

**Recommendation:** Defer string variables to a future version. For now:
- Strings come from ROM (command arguments, prompts)
- `_echo` can print literals: `_echo "hello"`
- Variables are integers only

### Error Handling

**Philosophy:** Scripts are usually short and hand-written. Fail fast with clear errors.

**Error conditions:**
- Undefined variable: return 0 (or error if strict mode)
- Undefined label: error, stop execution
- Mismatched if/endif: error at runtime
- Variable table full: error
- Label table full: error

**Error reporting:**
```c
/* Error messages via error output function */
x->fn_err("ERR: undefined var\n");
```

---

## 7. Implementation Phases

### Phase 1: File Reorganization (No Code Changes)

**Duration:** 1-2 hours

1. Create new directory structure
2. Move/rename files as planned
3. Update #include paths
4. Update makefile for new paths
5. Verify build works
6. Update .gitignore
7. Commit

**Deliverable:** Clean repo structure, same functionality

### Phase 2: Documentation

**Duration:** 2-4 hours

1. Update README.md with new structure
2. Create CHANGELOG.md (start with current state)
3. Create docs/API.md (document existing API)
4. Create docs/CONFIGURATION.md (existing options)
5. Create docs/PORTING.md (capture existing platform knowledge)
6. Create CONTRIBUTING.md

**Deliverable:** Comprehensive documentation

### Phase 3: Test Infrastructure

**Duration:** 4-8 hours

1. Review existing jumpbug framework
2. Add output capture helpers
3. Add char-by-char feeding helpers
4. Create test file stubs for each category
5. Add coverage reporting to makefile
6. Add CI workflow (.github/workflows/ci.yml)

**Deliverable:** Test framework ready for expansion

### Phase 4: Test Coverage - Foundation

**Duration:** 8-16 hours

1. Implement test_tokenizer.c (all cases from plan)
2. Implement test_strings.c
3. Implement test_dispatch.c
4. Run coverage, identify gaps
5. Add tests until 100% of existing code covered

**Deliverable:** 100% coverage of existing functionality

### Phase 5: Test Coverage - Edge Cases

**Duration:** 4-8 hours

1. Implement test_interactive.c
2. Implement test_edge_cases.c
3. Verify no crashes on adversarial input
4. Add memory safety checks (buffer canaries)
5. Run under valgrind (if POSIX)

**Deliverable:** Robust edge case handling verified

### Phase 6: Unified Command Structure

**Duration:** 4-8 hours

1. Add new XelpCmd structure
2. Add xelp_dispatch() function
3. Update internal dispatch to use new function
4. Add compatibility macros
5. Update examples to new style
6. Add tests for new dispatch
7. Update documentation

**Deliverable:** New command structure working, backward compatible

### Phase 7: Scripting - Variables

**Duration:** 4-8 hours (if desired)

1. Add XelpVar structure and storage
2. Implement `_set` command
3. Implement variable expansion in `_echo`
4. Implement `$varname` parsing
5. Add tests for variables
6. Update documentation

**Deliverable:** Working variables

### Phase 8: Scripting - Control Flow

**Duration:** 4-8 hours (if desired)

1. Add label structure and storage
2. Implement label detection (`name:`)
3. Implement `_goto`
4. Implement `_if`, `_else`, `_endif`
5. Add execution state tracking
6. Add tests for control flow
7. Update documentation

**Deliverable:** Working control flow

### Phase 9: Scripting - Math

**Duration:** 2-4 hours (if desired)

1. Implement `_inc`, `_dec`
2. Implement `_add`, `_sub`
3. Implement bitwise: `_and`, `_or`, `_xor`, `_not`, `_shl`, `_shr`
4. Add tests
5. Update documentation

**Deliverable:** Working math operations

### Phase 10: Polish


1. Final documentation review
2. Code cleanup (consistent style)
3. Performance check (code sizes)
4. Example updates
5. CHANGELOG update
6. Version bump
7. Tag release

**Deliverable:** Release-ready xelp 2.0

---

## Summary

This plan provides a systematic path to modernize xelp while preserving its core strengths. The phased approach allows stopping at any point with a working, improved codebase.

**Key Decisions:**
- Underscore prefix for built-in script commands (`_if`, `_set`)
- Suffix colon for labels (`loop:`)
- No multiply/divide (users bring their own)
- Integers only, no string variables (initially)
- Standard FORTH if FORTH mode desired (not a hybrid)
- All memory static, all features compile-time optional

**Estimated Total Effort:**
- Phases 1-5 (modernization without scripting): 20-40 hours
- Phases 6-10 (with scripting): additional 15-30 hours

The most valuable improvements come from Phases 1-5: clean structure, documentation, and comprehensive tests. Scripting can be added later based on actual need.
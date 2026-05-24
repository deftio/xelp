# XELP Script: Requirements

XELP Script is a small, no-malloc, instance-local scripting and orchestration layer for embedded C and C++ systems. It is not trying to become Lua, MicroPython, Tcl, Forth, or a general-purpose VM. Its purpose is narrower: give firmware a tiny live command surface that can sequence, compose, and lightly control native C and C++ functions.  

XELP’s core power is that C functions are not foreign bindings. They are the native vocabulary of the system. Script procs (when present), CLI commands, key commands where applicable, and C functions participate in one command-shaped fabric—as much as feasible without inventing parallel parsers or registries per transport.

Below, the prose sections explain intent and ergonomics first. Tables at the bottom are normative checklist items only: each row has an ID usable in reviews, hazards analysis, tests, and change control.

## Design center

XELP Script preserves the original XELP identity while adding just enough language support to make command streams programmable.

* **C/C++ is the fast path** - Hardware access, drivers, protocol handling, math-heavy routines, timing-sensitive behavior, control loops, parsing, storage, networking, and application-specific work remain compiled native functions.
* **XELP Script is orchestration** - Configuration, command sequencing, branching, diagnostics, factory tests, calibration flows, scripted bring-up, and field workflows. Bounded procedural flow uses `_if`, `_goto`, and labels initially—no mandated `while` in the first milestone.
* **Scripts are ROM-able** - Script source may live in ROM or flash. The interpreter must not mutate the source text to execute scripts.
* **No heap allocation** - Runtime footprint is bounded by fixed per-instance arenas and scratch prescribed at integration time—not `malloc` or growable interpreters.
* **Multiple instances feel natural** - A serial console and a BLE console can coexist on one MCU with separate buffers, output hooks, mode, command tables, and optional capability policy.
* **Command compression matters** - CLI-shaped scripts shorten repeated C-call sequences on a wire humans read. Narrower transports can still target the same verbs without a second semantics.
* **Coexistence with core XELP** - Comments (`#`), statement boundaries (`;`, newline where the tokenizer allows), escapes, and quoting philosophy should remain teachable beside today’s CLI and `XelpParse` story—fewer duplicated rules in handlers.
* **Safe Execution** - If the user makes an infinite loop in xelp script we should have a key sequence we listen for (e.g. ESC ESC or CTRL-C or something) which allows the script to be interrupted.  This means the Xelp interpreter needs to stay live between executing statements.

Positioning:

```text
XELP Script is a no-malloc, ROM-able, instance-local command script for embedded C/C++ systems.
It runs at command speed. C/C++ remains the native fast path.
```

Informative scenarios (non-exhaustive): factory calibration ladders, scripted bring-up, regression harness hooks on peripherals, field diagnostics macros over UART or BLE.

## XELP background

xelp is a lightweight, embeddable command-line interpreter written in
pure C89. It has been in continuous use since approximately 2005–2006,
originally developed as a debug shell for bare-metal embedded projects
targeting 8051 and MSP430 microcontrollers. The codebase migrated through
Subversion and Bitbucket before being open-sourced on GitHub in 2024
under the BSD 2-Clause license.

### What xelp is

A character-at-a-time parser that processes input one byte at a time —
no line buffering required, no OS, no malloc, no stdlib dependencies.
It provides three operating modes:

* **CLI mode** — line-buffered command prompt with backspace, cursor
  movement (left/right, Home/End), insert-at-cursor, Delete, and
  optional command history (UP/DOWN arrow recall). Commands are
  dispatched via a function pointer table: each registered command name
  maps to a C function with the signature
  `XELPRESULT fn(XELP *ths, int argc, const char **argv)`.
* **KEY mode** — single-keypress dispatch. Each key (including multi-byte
  ANSI escape sequences) triggers a function immediately, no ENTER
  needed. Ideal for debug menus and hardware test jigs.
* **THRU mode** — pass-through. All input is forwarded to another
  peripheral or handler. Used for bridging between UARTs or redirecting
  input to a subsystem.

Mode switching is done via configurable hotkeys (default: CTRL-P for CLI,
ESC for KEY, CTRL-T for THRU).

### Key properties

* **No dynamic allocation.** All state lives in a fixed-size struct
  (`XELP`). Buffer sizes are compile-time constants. Zero malloc, zero
  free, zero heap.
* **Multi-instance.** Multiple independent xelp instances can run on
  one MCU — e.g. one on USB Serial, one on BLE, one on a debug UART.
  Each has its own buffers, mode, command tables, output function, and
  optional capability policy. No global mutable state.
* **ROM-able scripts.** The existing `XelpParse` function executes
  command sequences from a `const char *` buffer — script source can
  live in ROM or flash. The parser does not mutate the source text.
* **Compile-time feature selection.** Features are controlled by
  `#define` flags in `xelpcfg.h`. Unused features compile out
  completely. Configurations range from ~550 bytes (KEY-only on ARM
  Thumb) to ~3–5 KB (full CLI + KEY + THRU + HELP + LINE_EDIT +
  HISTORY).
* **Platform abstraction.** Five function pointers in the XELP struct
  (`mpfOut`, `mpfIn`, `mpfInReady`, `mpfGetTicks`, `mpfDelay`) abstract
  all platform dependencies. Porting to a new target is implementing
  these five functions.
* **Cross-architecture.** Tested on 8-bit (AVR, 8051), 16-bit (MSP430),
  32-bit (ARM Cortex-M, ESP32, RISC-V rv32, Xtensa), and 64-bit (x86-64)
  targets. CI runs 18 cross-compilation targets via Docker.

### Current size (v0.4.0, ARM Thumb, full config)

~3,079 bytes code. This includes CLI mode with argc/argv dispatch, KEY
mode, THRU mode, HELP, line editing, command history, tokenizer with
quoted strings and escape sequences, and multi-instance support.

### What xelp is not

xelp is not a scripting language, an operating system, a scheduler, or a
protocol stack. It is a command dispatcher. C functions are the commands.
The XELP struct is shared state between the parser and the application —
there is no foreign function interface, no bindings, no marshaling. A
command handler reads `argv`, calls hardware drivers directly, and
returns a status code.

### Why XELP Script

The existing `XelpParse` executes flat command sequences from ROM — but
with no variables, no conditionals, no composition. Every non-trivial
flow requires a C function. XELP Script adds just enough language
support to make command streams programmable without replacing C as the
primary implementation path. See [Design center](#design-center) above.

## Language shape

XELP is command-shaped. It can resemble Tcl from a distance—words in a row—but does not inherit Tcl’s full text-substitution model. Values behave as typed cells with explicit rules (`INT`, `STR`, and sentinel kinds), not interchangeable string soup rewired before every dispatch.

Core surface:

```text
command arg arg arg
```

Example commands mixing firmware verbs with language builtins:

```text
led 1
read_adc 0
_set x 10
_if (> $x 5) _then high
```

This keeps the lexical layer small:

* No infix algebraic grammar—no precedence table for `1 + 2 * 3`.
* No AST interpreter for generalized expressions.

Math and predicates use Lisp-style prefixes: **`+`** and **`>`** act as ordinary command names, not lexer magic.

```text
_set x (* (+ $foo 4) 3)
_if (_eq $x 20) _then big
```

```text
(+ 1 2 3)
```

means "invoke command `+` with arguments `1`, `2`, and `3`."
Parentheses are automatically space-separated by a pre-tokenization pass (PE-02).

Invalid as core math syntax (heavy work belongs in C):

```text
1 + 2 * 3
```

## Invocation model

Exactly two outward invocation shapes matter to script authors:

```text
foo arg arg               # statement: run foo for side effects / registers / outcome
(foo arg arg)             # nested value context: foo must yield one typed value for the caller
```

The parenthesized form evaluates its inner command first and substitutes the returned typed value as a single argument to the enclosing command. A pre-tokenization pass automatically space-separates parens (PE-02).

Worked example:

```text
_set x (* (+ $foo 4) 3)
```

```text
(+ $foo 4)       -> value A
(* A 3)          -> value B
_set x B         -> store B into variable x
```

Parentheses are explicit nesting without operator precedence parsers:

```text
_set x (+ 1 (* 2 3))     # intention: 7
_set y (* (+ 1 2) 3)     # intention: 9
```

Authoring rules worth memorizing:

```text
Arguments evaluate left-to-right.
When an argument is a parenthesized subexpression, evaluate it before passing outward.
Each parenthesized subexpression yields exactly one typed value (or a defined ERR/NIL sentinel).
```

## Addressing model

Keep variables, call parameters, and command invocation visually distinct.

```text
$name     expand variable — get the value of "name"
@1        first positional argument of innermost scripted callable
@2        second positional argument
@name     named argument, if supported
@#        argument count (if supported)
foo       verb in command position / literal name in argument position
(foo ...) nested value-producing call
```

Three domains:

* **Variables via `$`** - `$temperature` resolves in the active variable scope for this instance/script artifact. `$` is an expansion operator, not part of the variable name — it means "get the value of."
* **Parameters via `@`** - `@1` is always positional parameter one; avoids bash confusion where `$1` means something else entirely.
* **Invocation by name** - `motor 50` is a statement. `(motor 50)` participates as a nested value expression only where grammar allows.

### `$` sigil rule — expand semantics

**One rule: `$name` always means "expand to the value of name." Always. Everywhere. No exceptions.**

Without `$`, a token is a literal — the text itself, not a variable reference. This is the Tcl/bash model, not the PHP/Perl model. The sigil tells the reader whether a token is being expanded or used literally. The question "do I need `$` here?" becomes: "do I want the value or the name?"

```text
_set x 10         # bare "x" = the name to bind, "10" = literal value
_set y $x         # bare "y" = name to bind, $x = expand x → 10
_print $x         # $x = expand → "10"
_if (> $x 5) ...  # $x = expand → 10, compare to 5
```

**Indirect assignment falls out naturally:**

```text
_set target "temperature"
_set $target 42   # $target expands → "temperature", so this sets variable "temperature" to 42
_print $temperature   # → "42"
```

Both `$` tokens in `_set $target 42` are expanded because both have `$`. The user wrote `$` on both because they *want* both expanded. If they wanted to set the literal variable `target`, they'd write `_set target 42`.

If a `$` reference is undefined, expansion fails → error (undefined variable). No silent empty-string substitution.

## Bare word rule

Behavior should be predictable for first-time readers:

In command position, the first word is the verb (`foo a b` — `foo` is looked up).

In argument position, an unquoted word is a literal, not automatic variable dereference (`print hello` sends the token hello to the print function).

* To interpolate a variable, use **`$`** (`print $hello`).
* To call a command for a value slot, parenthesize (`print (hello)`).
* To refer to positional parameters inside a scripted callable, use **`@`** (`print @1`).

Recommended decoding after tokenization:

```text
unquoted digits / numeric      -> INT (XelpParseNum succeeds)
"quoted …"                     -> STR (always, even if content is numeric)
bare word (unquoted, non-num)  -> STR literal
$name                         -> typed variable value (expand)
@1 / @name                    -> parameter value
(... )                        -> typed return from nested call
```

### Quotes as type annotation

Quotes resolve the "3" vs 3 ambiguity. The evaluator can see raw tokens
(before quote stripping) for builtins. The rule:

```text
_set x 3        # unquoted, XelpParseNum("3") succeeds → INT(3)
_set x "3"      # quoted → STR("3"), regardless of content
_set x hello    # unquoted, XelpParseNum("hello") fails → STR("hello")
_set x "hello"  # quoted → STR("hello")
_set x 0xFF     # unquoted, XelpParseNum("0xFF") succeeds → INT(255)
_set x "0xFF"   # quoted → STR("0xFF")
```

This gives explicit control when it matters. For the common case (numbers
are numbers, strings are strings), inference does the right thing. For the
edge case of a string that looks like a number, quotes force STR. The cost
is ~zero: one `if (*tok == '"')` branch in the builtin evaluation path.

### Two-path type handling

Type information flows through **two separate channels** depending on
the dispatch path:

**User C handlers** (normal dispatch) — quotes are stripped, everything
in argv is a plain null-terminated string. Exactly like today. `_foo $x 3 "3"` → handler sees `argv = ["_foo", "10", "3", "3"]`. The two "3"s are
indistinguishable. This is the existing contract (EX-02). C handlers don't
know or care about script types. If the handler wants an int, it calls
`XelpArgvInt`.

**Builtins** (`_set`, `_if`, etc.) — the evaluator processes raw tokens
*before* building argv. It sees quotes, `$` sigils, `()` nesting. Type
decisions happen during evaluation, not from argv strings. The type tag
is stored in the arena entry.

**Type info does not round-trip through argv.** When `$x` (INT 3) and
`$y` (STR "3") are expanded into argv for a C handler, both become the
string `"3"`. But inside the script variable system, they have different
type tags and behave differently under operations like `+` or `_eq`.

```text
_set a 3          # INT(3)
_set b "3"        # STR("3")
_print $a $b      # both print "3" — no visible difference in output
(+ $a 1)          # → 4, INT + INT works
(+ $b 1)          # → type error: STR + INT (see comparison semantics)
```

## Normative requirements (how to read this section)

Requirements are grouped by engineering concern—not by alphabetical ID. Before each group, short paragraphs spell out why the cluster exists.

Each table row mixes RFC-style keywords (**MUST**, **SHOULD**, **MAY**) with identifiers like **C-01** for traceability. Implementation details (`struct XELP_tag`, `_xelp…` internals, exact `XelpCall` shapes) belong in design proposals—especially `dev/xelp_script_proposal3.md`—not here.

### Constraints **(C‑\*)**

These rows exist so ports to eight-bit MCUs through 64-bit bare metal remain honest: auditors and integrators can verify **bounded RAM**, **determinism**, and **multiple consoles** without rereading the whole interpreter.

| ID | Requirement |
| --- | --- |
| C-01 | MUST NOT rely on heap allocation (malloc, calloc, growable arenas, etc.). All runtime footprint MUST be bounded by compile-time or documented integration-time sizing. |
| C-02 | MUST support multiple independent instances without mandating global interpreter mutable state required for correctness. |
| C-03 | Script source MAY live in ROM/flash; execution MUST NOT rely on rewriting those source bytes to run. |
| C-04 | SHOULD NOT force parallel “script-only” registries for the same product verbs CLI already exposes unless policy demands it—the same canonical names SHOULD work from ROM scripts and typed lines except where deliberately restricted (see M‑01). |

### Relationship to native code **(N‑\*)**

Keeps scripting subordinate to product truth implemented in compiled code; discourages creeping interpretive workloads into script.

| ID | Requirement |
| --- | --- |
| N-01 | MUST treat registered C/C++ verbs as primary capability—not optional FFI glue. |
| N-02 | SHOULD keep interactive CLI ingestion, script buffers, and future script VM dispatch aligned wherever practical so quoting, errors, and help stay teachable (“one fabric” story). |
| N-03 | DSP, cryptography, parsers, persistence, protocols, deterministic motor control, tight loops—these SHOULD remain primarily C/C++; script marketing MUST NOT imply replacement by default. |

### Anti-goals **(A‑\*)**

Declarative exclusions—prevents stakeholder drift toward “tiny Lua”.

| ID | Anti-goal |
| --- | --- |
| A-01 | No Tcl-grade macro substitution rewriting arbitrary text blobs before lexing completes. |
| A-02 | No infix algebraic core, heavyweight expression AST layer, or general compile-to-bytecode toolchain requirement. |
| A-03 | No mandated while/for in the first milestone (see Control flow requirements). |
| A-04 | No dynamic interpreter heap growth masquerading as optional; hostile eval by default discouraged. |

### Language surface **(L‑\*, B‑\*)**

What authors type and integrators forbid in naming—keeps parsers tiny and manuals honest.

#### Command skeleton

Why: retain CLI continuity and predictable tokens.

| ID | Requirement |
| --- | --- |
| L-01 | MUST stay command-first: whitespace-separated command …arguments…. |
| L-02 | Core math MUST NOT depend on precedence parsers (optional symbolic commands still occupy command namespace). |

#### Built-in naming and symbolic profiles

Why: carve namespace so `_set` / `_if` stay obviously “language,” while firmware keeps short verbs like `led` and `read_adc`.

**Settled naming rule for math (so authors are not guessing):**

* **`_add`, `_mul`, `_gt`, …`** — **Canonical builtins** whenever the builtin has an **alphanumeric spelling**. The leading **`_`** marks **reserved language**. Use these in manuals, grammar tables, size-stingy profiles, and anywhere you want one obvious reserved namespace (`_*` identifiers only).

* **`+`, `>`, … (symbol as the whole command token)** — **Optional ergonomic aliases**, same precedence story as today: **not infix lexer magic**, **just shorter command names** bound to the same handler as **`_add`**, **`_gt`**, etc. **Symbols do not get a **`_`** prefix** — punctuation commands are **`+`**, not **`_plus`** disguised as symbol, and definitely not **`_+`**.

* **`_+`** — **Avoid / do not endorse.** **`_`** was chosen to glue to **identifiers** (`_set`). Combining **`_` + punctuation** buys little over plain **`+`**, reads badly in docs (“underscore plus”), and invites tokenizer edge cases. If you ship symbols at all, ship **`+`**; if you strip symbols for ROM, ship **`_add`** only.

| ID | Requirement |
| --- | --- |
| B-01 | **Identifier-shaped** language builtins **SHOULD** use the **`_name`** pattern (`_set`, `_if`, `_goto`, `_add`, `_gt`, …). |
| B-02 | **`_<identifier>` MUST** remain reserved; application commands **MUST NOT** collide—integrators **SHOULD** catch collisions at integrate time (registration or lint), not ambiguously at runtime. |
| B-03 | Compile profiles **MAY** register **symbol-only** commands (`+`, `*`, `>`) **as aliases** to the same implementations as **`_add`**, `_mul`, **`_gt`**, etc.—still dispatched as ordinary commands (**never** infix precedence parsing). |
| B-04 | **SHOULD NOT** canonically spell punctuation builtins as **`_+`**, **`_>`**, etc.—use **`+`** (**B‑03** alias) or **`_add`** (**B‑01**), not **`_`** glued to punctuation. |

#### Comments / statement breaks

Why: reduce parallel mental models versus today’s scripting strings.

| ID | Requirement |
| --- | --- |
| L-03 | SHOULD align # … comments and newline/; statement boundaries with the shipped tokenizer contract so textbooks stay coherent. |

#### Typed evaluation

Why: preempt silent string-only semantics.

| ID | Requirement |
| --- | --- |
| L-04 | SHOULD ground evaluation in bounded tagged kinds—INT and STR minimally documented. |
| L-05 | $name MUST mean variable retrieval—not unstructured macro splice into unrelated tokens (truthy/if annex controls conversions). |

### Arity and variadic invocation **(V‑\*)**

Laboratory traffic often ships extra positional noise; ergonomics favors ignoring extras intentionally rather than hard failure—except documented strict builtins.

| ID | Requirement |
| --- | --- |
| V-01 | SHOULD: language builtins (_*) and idiomatic handlers ignore excess documented-safe arguments whenever semantics tolerate it. |
| V-02 | MUST NOT allow unchecked argv reads, recursion blowups, or undefined memory state when extras arrive—responses remain deterministic per handler contract. |
| V-03 | Exact-arity builtins (expected _if family) MUST declare strictness and SHOULD surface structured failures without silent success. |
| V-04 | Silent ignore versus warning-bearing ignore SHOULD remain observable (status lineage / documented introspection) when profiles differ. |

Informative shorthand: aggregated addition **MAY** stay variadic; `_if` **SHOULD remain** strict absent broader grammar allowances.

### Invocation and nesting **(I‑\*)**

Pins the authoring contract for parentheses—root of script composition.

| ID | Requirement |
| --- | --- |
| I-01 | Statements foo … MUST execute sequentially as today’s CLI verbs expect. |
| I-02 | `(foo …)` MUST only appear where grammar permits value slots; nesting MUST contribute exactly one typed value or sanctioned ERR sentinel. |
| I-03 | SHOULD evaluate neighbors left-to-right with inner parentheses ahead of enclosing arguments—mechanical precedence minus algebra sugar. |

Recap examples:

```text
_set x (* (+ $foo 4) 3)
_set x (+ 1 (* 2 3))
```

### Return model **(R‑\*)**

Separates chatter on the UART from machine-readable compose semantics—critical once BLE hosts automation.

#### Two-channel return contract

Every function invocation (C handler or script proc) produces two results:

1. **Status channel: `XELPRESULT`** — did the function succeed? The existing
   C return value. `XELP_S_OK` (0) = success, positive = warning, negative =
   error. `_if` checks this channel for command truthiness.

2. **Value channel: one `XelpVal`** — what did the function produce? A single
   tagged value (INT or STR). Pushed onto a bounded result stack. Defaults to
   NIL if the handler doesn't set it.

Status and value are orthogonal. A function can succeed and return a value,
succeed and return NIL, or fail (value is undefined/ignored on failure).

#### Script function returns

Script procs return one value via `_return`:

```text
_proc double
  _return (_mul @1 2)       # return INT

_proc greet
  _return "hello"           # return STR

_proc do_stuff              # no _return = implicit NIL, still success
  led 1
  delay 100
```

`_return` sets the value channel and exits the proc with `XELP_S_OK`.
Falling off the end of a proc = success with NIL.

#### C handler returns for script consumption

C handlers already return `XELPRESULT`. To make a value visible to script
callers, the handler calls `XelpSetResultInt` or `XelpSetResultStr` — a new
API that pushes onto the result stack:

```c
XELPRESULT cmd_read_adc(XELP *ths, int argc, const char **argv) {
    int channel = 0;
    if (argc > 1) XelpArgvInt(argv, argc, 1, &channel);
    int val = adc_read(channel);
    XelpSetResultInt(ths, val);     /* value channel for script */
    return XELP_S_OK;              /* status channel */
}
```

Script side:

```text
_set x (read_adc 0)                # captures value channel into $x
_if (_gt $x 100) _then _next :hot
```

C handlers that don't call `XelpSetResult*` return NIL to script callers.
Existing handlers work unchanged — they just return NIL values until updated
with `XelpSetResult*` calls. Migration is gradual.

#### Result stack

The result stack is a fixed-size, compile-time bounded array
(`XELP_RESULT_STACK_SZ`, e.g. 8–16 entries). Each entry holds one `XelpVal`
(tagged INT or STR). Frame entry records the stack pointer; frame exit pops
back. Overflow = hard error.

The result stack is the universal mechanism for script value composition:

```text
_set x (_mr 2)                    # _mr pushes mR[2] onto result stack
_set y (_add $x 10)               # _add pushes sum onto result stack
_if (_gt $y 100) _then _next :big # _gt pushes comparison result
```

Every builtin is just a function — same return contract, same result stack,
same composition rules. No special cases.

#### `mR[]` mailbox: `_mr`

`mR[]` (the four per-instance integer registers) remains unchanged as the
direct C-to-C communication channel. It is not the script return path.

Script accesses `mR[]` via the `_mr` builtin, which follows the standard
two-channel contract like any other function:

* **Read:** `_mr <index>` — status = OK if index valid, value = `mR[index]`
  as INT on the result stack.
* **Write:** `_mr <index> <value>` — sets `mR[index]`, status = OK, value =
  the value written.

```text
# read mR[0] left by a C handler
_set x (_mr 0)

# set mR[] for a C handler that reads it
_mr 0 $gain
_mr 1 $direction
motor_update                  # C handler reads mR[0], mR[1]
```

`_mr` is not special — it's a builtin function that happens to read/write
the mailbox. Same result stack, same status channel, same composition:

```text
_set x (_mr 2)                    # capture into variable
_if (_gt (_mr 0) 100) _then :hot  # nested in expression
_return (_mr 0)                   # pass through as proc return
```

#### C calling script, script calling C

**C calling script proc:**

```c
XELPRESULT cmd_run_cal(XELP *ths, int argc, const char **argv) {
    XelpVal result;
    XELPRESULT r = XelpCallProc(ths, "double", "21", &result);
    if (r == XELP_S_OK && result.kind == XELP_VAL_INT)
        ths->mR[0] = result.v.i;   /* stash in mR[] for C-side use */
    return r;
}
```

**Script calling C (value-producing):**

```text
_set x (read_adc 0)       # C handler called XelpSetResultInt
```

**Script calling C (side-effect only):**

```text
led 1                      # C handler returns XELP_S_OK, no value
```

**Script ↔ C via mailbox (legacy interop):**

```text
_mr 0 $gain                # script → mR[0] → C reads ths->mR[0]
motor_update
_set x (_mr 0)             # C wrote ths->mR[0] → script reads it
```

#### Relationship between `mR[]` and result stack

`mR[]` and the result stack are independent. A C handler can set both —
`mR[]` for C callers, `XelpSetResult*` for script callers. Or just one.
They don't interfere.

* **`mR[]`** — per-instance, flat, no nesting, no types. Cheap. Best for
  C-to-C or low-level script↔C integer passing.
* **Result stack** — per-frame, typed, composable. Supports `( )` nesting.
  The script-native return path.

#### Identity principle

xelp script is not a language with C bindings. C functions *are* the
commands. The dispatch table is function pointers. `mR[]` is the same four
ints whether read from C or from script via `_mr`. The XELP struct is shared
state — C writes a field, script reads it on the next line. No marshaling,
no serialization, no foreign function interface.

`_mr` embodies this: it's not "script accessing C memory through a bridge."
It's script reading the same struct field that C reads.

| ID | Requirement |
| --- | --- |
| R-01 | Every function invocation (C handler or script proc) MUST produce a status (`XELPRESULT`) and at most one typed value (`XelpVal`). Status and value are orthogonal channels. |
| R-02 | `( … )` MUST deliver exactly one `XelpVal` from the value channel or deterministic NIL — not garbage across nested evaluations. |
| R-03 | `XelpOut` MUST NOT silently become the scripted return channel unless a documenting command declares that contract. |
| R-04 | `mR[]` registers MUST remain available as the direct C-to-C communication channel. They are independent of the result stack. |
| R-05 | Script procs MUST return one value via `_return`. No `_return` = implicit NIL with `XELP_S_OK`. |
| R-06 | C handlers SHOULD call `XelpSetResultInt` / `XelpSetResultStr` to make values visible to script callers. Handlers that don't call `XelpSetResult*` MUST return NIL (not garbage). |
| R-07 | The script arena MUST be a single fixed-size, compile-time bounded buffer (`XELP_SCRIPT_ARENA_SZ`). Overflow MUST be a hard error. Frame exit MUST pop results and local variables back to the frame entry point. |
| R-08 | `_mr <index>` MUST be an ordinary builtin that follows the two-channel return contract. Read: pushes `mR[index]` onto result stack. Write: sets `mR[index]` and pushes the written value. Out-of-bounds index MUST error. |
| R-09 | `_mr` is not special. It is a builtin function with the same return contract, result stack behavior, and composition rules as every other builtin. |
| R-10 | The script engine MUST NOT use `mR[]` for its own return values. `XELPRESULT` status for script functions lives on the result stack, not in `mR[0]`. `mR[]` is exclusively for C handler communication. |

### Value types **(VT‑\*)**

The type tag is a single byte (256 possible values). MVP implements only
INT (native `int`) and STR. All other types are reserved enum constants
that exist in the header but return "unsupported type" at runtime until
a future profile enables them. Reserving slots now costs nothing — no
renumbering, no versioning headaches when a type ships later.

#### Type enum

```c
enum {
    XELP_VAL_NIL    = 0x00,     /* no value / unset                */

    /* Signed integers */
    XELP_VAL_I8     = 0x01,     /* int8_t                          */
    XELP_VAL_I16    = 0x02,     /* int16_t                         */
    XELP_VAL_I32    = 0x03,     /* int32_t                         */
    XELP_VAL_I64    = 0x04,     /* int64_t                         */

    /* Unsigned integers */
    XELP_VAL_U8     = 0x05,     /* uint8_t                         */
    XELP_VAL_U16    = 0x06,     /* uint16_t                        */
    XELP_VAL_U32    = 0x07,     /* uint32_t                        */
    XELP_VAL_U64    = 0x08,     /* uint64_t                        */

    /* Floating point */
    XELP_VAL_F16    = 0x09,     /* half precision (16-bit float)   */
    XELP_VAL_F32    = 0x0A,     /* float                           */
    XELP_VAL_F64    = 0x0B,     /* double                          */

    /* String / blob */
    XELP_VAL_STR    = 0x10,     /* string (pointer + length)       */

    /* Internal / structural */
    XELP_VAL_FRAME  = 0xF0,     /* frame marker (arena internal)   */
    XELP_VAL_PROC   = 0xF1,     /* procedure reference             */

    /* Platform-width alias: INT = native int */
    XELP_VAL_INT    = XELP_VAL_I32,  /* I16 on 16-bit targets      */
};
```

#### Design notes

**`XELP_VAL_INT` as an alias.** On 32-bit targets, `INT` = `I32`. On
16-bit (MSP430, AVR), `INT` = `I16`. Follows xelp's existing pattern
where `XELPREG` defaults to `int`. Script code that uses `_set x 42`
gets the native width. Explicit `I32` is for when 32 bits are needed
on a 16-bit target.

**Floating point.** F16 is relevant for DSP-adjacent embedded work.
F32 is the common embedded float. F64 is rare on MCUs but the slot is
free. For types wider than the stack entry union (F64 on 32-bit), the
entry stores an arena-heap offset to an 8-byte slot rather than inline,
keeping entry size fixed.

**PROC as a type.** A variable holding a reference to a callable proc.
Enables `_set callback my_handler` then `_call $callback`. First-class
function references for event-driven patterns. Reserved, not in MVP.

**What NOT to implement now.** The enum reservation is free. The union
widening, conversion rules, and truthiness table expansion are not.
Reserve the namespace now, implement on demand when a real use case
arrives.

| ID | Requirement |
| --- | --- |
| VT-01 | The type tag MUST be a single byte. All types in the enum are reserved. Unimplemented types MUST return a clean error at runtime, not garbage. |
| VT-02 | MVP MUST implement NIL, INT (native `int` width), and STR. All other types are reserved for future profiles. |
| VT-03 | `XELP_VAL_INT` MUST alias the platform-native `int` width (`I32` on 32-bit, `I16` on 16-bit). Script code using `_set` with integer literals MUST use this alias. |
| VT-04 | Types wider than the stack entry union (e.g. I64, F64 on 32-bit) SHOULD store data via arena-heap offset, not inline, to keep entry size fixed. |
| VT-05 | The FRAME and PROC type tags are reserved for arena-internal and future use respectively. Script code MUST NOT create values of these types directly. |
| VT-06 | Adding a new implemented type in a future version MUST NOT require renumbering existing type tags. Enum values are stable across versions. |

### Script arena design **(AR‑\*)**

The script runtime state lives in a single per-instance buffer: the
**arena**. One buffer, two pointers, one overflow check. All internal
plumbing — the arena layout, entry format, frame marker representation —
is behind the API wall. Scripts, C handlers, and the public API
(`_return`, `_set`, `XelpSetResultInt`, `XelpCallProc`) are unaffected
by internal reorganization.

#### Arena layout

```text
                          arena (XELP_SCRIPT_ARENA_SZ bytes, e.g. 2048)

[ stack grows →                                          ← heap grows ]
[ entry | entry | entry | ....... free ....... | strdata | var | var | var ]
  ^                                                                     ^
  SP (result entries + frame markers)                    HP (variables + string data)

  overflow check: SP >= HP → arena full → hard error
```

The **stack** (growing upward from the arena start) holds result entries
and interleaved frame markers. The **heap** (growing downward from the
arena end) holds variable headers and inline string data. They share
the same buffer and meet in the middle.

#### Byte-stream encoding

The arena is a **managed byte stream**, not an array of fixed-size
structs. Every record — frame, result, variable — is a variable-length
tagged entry. You pack exactly what you need, no wasted space.

Each record starts with a `kind` byte (from the VT enum) followed by
a payload whose layout depends on the kind. Records are tightly packed
with no alignment padding.

**Result entries** (stack, growing upward):

```text
[ kind | status (XELPRESULT) | payload... ]

NIL:   [ 0x00 | status_4B ]                                = 5 bytes
INT:   [ 0x03 | status_4B | value_4B ]                     = 9 bytes
STR:   [ 0x10 | status_4B | len_2B | string_bytes ]        = 7 + len bytes
```

**Frame entries** (stack, growing upward):

```text
[ 0xF0 | varHP_2B | retAddr_4B | scriptEnd_4B | argc_1B |
  argv[0]\0 argv[1]\0 ... argv[argc-1]\0 |
  ptr0 ptr1 ... ptr(argc-1) ]
```

The frame packs everything it needs contiguously:

1. Frame header: tag + saved heap pointer + return address +
   script end bound (~11 bytes).
2. Packed argv strings: each token (literal or expanded variable)
   null-terminated, written end-to-end. Size = sum of token lengths
   + argc null terminators.
3. Argv pointer array: argc pointers, each pointing back into the
   packed strings region. Written once at frame entry after
   tokenization.

A call like `motor 75 1` (argc=3) produces:

```text
[ 0xF0 | varHP | retAddr | scriptEnd | 0x03 |
  'm''o''t''o''r'\0 '7''5'\0 '1'\0 |
  ptr0 ptr1 ptr2 ]

= 11 + 13 + 12 = 36 bytes  (on 32-bit)
```

Compare to a fixed-struct approach (108 bytes). The byte-stream frame
is sized to the actual content.

**Positional parameters** (`@1`, `@2`, ...) read directly from the
pointer array: `@n` = `pointers[n]`. No scanning, no separate storage.

**Variable entries** (heap, growing downward):

```text
INT:   [ kind_1B | nameHash_2B | value_4B ]                = 7 bytes
STR:   [ kind_1B | nameHash_2B | len_2B | string_bytes ]   = 5 + len bytes
```

Variable lookup is a linear scan of the heap region — bounded by the
number of variables (small in embedded scripts). Hash comparison
avoids strcmp on every entry.

#### Frame lifecycle

**Enter proc:** push a FRAME marker onto the stack. It records the
current heap pointer (varHP) and return address. Everything above this
marker on the stack, and everything below the saved varHP on the heap,
belongs to the callee.

**During execution:** commands push result entries on the stack.
`_set` creates variables on the heap. String data is stored inline.
`( )` nesting pushes intermediate results that get consumed.

**Return:** `_return` captures the top result entry. The stack is
popped down to (and including) the FRAME marker. The heap is reset to
the saved varHP (releasing local variables). The captured return value
is pushed onto the caller's stack. Execution resumes at retAddr.

```text
Before return:
[ caller_stuff | FRAME | local_result | local_result | return_val ]
                  ^

After return:
[ caller_stuff | return_val ]
```

All temporaries and local variables are reclaimed in one SP/HP reset.
No fragmentation. The arena space is immediately reusable.

#### Zero overhead when not using procs

Flat scripts (`_set`, `_if`, `_next`, `_mr`, C commands) never push
frame markers. The full arena is available for results and variables.
Frames are opt-in overhead — only `_proc` calls create them.

#### Sizing budget (2 KB arena, 32-bit target)

Example: 4 frames deep, typical command sizes, 16 result entries,
24 variables.

| Component | Typical per-unit | Count | Bytes |
|---|---|---|---|
| Frames (header + argv strings + pointers) | ~36 | 4 | 144 |
| Result entries (INT) | ~9 | 16 | 144 |
| Variable entries (INT) | ~7 | 24 | 168 |
| Variable entries (STR, avg 8 chars) | ~13 | 8 | 104 |
| Free / headroom | — | — | ~1488 |

Because records are variable-length, there are no hard partitions.
A script with many variables but shallow nesting uses more heap. Deep
nesting with few variables uses more stack. The arena self-balances.

| ID | Requirement |
| --- | --- |
| AR-01 | The script arena MUST be a single contiguous buffer of `XELP_SCRIPT_ARENA_SZ` bytes (default 2048). One buffer per instance. |
| AR-02 | The stack (results + frame markers) MUST grow upward from the arena start. The heap (variables + string data) MUST grow downward from the arena end. Overflow is detected when they meet. |
| AR-03 | Frame markers MUST be interleaved in the stack as variable-length tagged records (kind = `XELP_VAL_FRAME`). A frame record includes the header, packed argv strings, and the argv pointer array — sized to actual content, not a fixed struct. |
| AR-04 | Frame exit MUST pop the stack to the frame marker position and reset the heap to the saved varHP, reclaiming all callee-local results and variables in one operation. |
| AR-05 | The arena layout is internal to the engine. The public API (`_return`, `_set`, `XelpSetResultInt`, `XelpCallProc`) MUST NOT expose arena offsets, stack pointers, or entry formats. Internal layout MAY change without affecting scripts or C handler code. |
| AR-06 | Flat scripts that do not use `_proc` MUST NOT incur frame marker overhead. The full arena is available for results and variables. |
| AR-07 | Arena overflow MUST be a hard error. The engine MUST NOT silently corrupt memory or wrap pointers. |

### Variable expansion and argv scratch **(EX‑\*)**

When script dispatches a command that contains variable references
(`$name`, `@n`), the variables must be expanded into string form before
the C handler sees argv. This section defines where expansion happens
and where the scratch buffer lives.

#### Expansion flow

Example: `motor $gain $dir` where `$gain` = 75 (INT), `$dir` = 1 (INT).

1. Script engine encounters the command line `motor $gain $dir`.
2. Each token is processed into the argv scratch buffer:
   - `motor` → literal copy → `argv[0] = "motor"`
   - `$gain` → variable lookup → INT 75 → format `"75"` into scratch → `argv[1]`
   - `$dir` → variable lookup → INT 1 → format `"1"` into scratch → `argv[2]`
3. `argc = 3`, argv pointers reference positions in the scratch buffer.
4. Dispatch: `cmd_motor(ths, 3, argv)`.

The C handler receives normal argc/argv — it has no idea variables were
involved. Expansion is invisible to handlers.

**Number formatting:** INT values are formatted as decimal strings into
the scratch buffer using `_xelpIntToStr` — a shared internal int-to-buffer
utility. Hex output is not automatic — if a handler needs hex, it parses
the decimal string. This matches the existing `XelpStr2Int` convention
where input accepts both formats but output is always decimal.

**String expansion:** STR variables are copied verbatim into the scratch
buffer. The copied string is null-terminated like any other argv token.

#### Int-to-string utility: `_xelpIntToStr`

xelp core has `XelpStr2Int`/`XelpParseNum` (string → int) but no int →
string. The script engine needs one for `$var` expansion, and the same
function is useful in multiple contexts:

```c
/* Write signed decimal integer to dst buffer.
   Returns number of bytes written (not null-terminated).
   Caller must ensure dst has room — 12 bytes covers INT_MIN on 32-bit. */
int _xelpIntToStr(char *dst, int n);
```

Algorithm: count digits, emit sign if negative, MSB-first digit output
via power-of-10 division (same approach as `FR_printNumD` in the sister
library `fr_math`). No reverse buffer needed. ~30–40 bytes ARM Thumb.

**Reuse cases:**

| Consumer | How it uses `_xelpIntToStr` |
|---|---|
| **`$var` expansion** | Writes directly into arena scratch during argv packing. Primary use case. |
| **`_print` builtin** | Does NOT need it — by the time `_print` sees argv, `$var` is already expanded to a string. `_print` just prints argv strings via `XelpOut`. |
| **C handler helpers** | Write to a small stack buffer (`char buf[12]`), then `XelpOut(ths, buf, len)`. Eliminates the need for `snprintf` in handlers that print integers. |
| **Debug / diagnostic** | Print `mR[]` values, error codes, step counts, arena usage. Same pattern as handler helpers. |

**Public API consideration:** Exposing as `XelpIntToStr` (public) would
give handler authors a portable itoa without `snprintf` — useful on
bare-metal targets with no stdio. Pairs with the existing `XelpStr2Int`
to complete the int↔string round-trip. Decision deferred to implementation.

#### Argv storage: packed in the frame, part of the byte stream

The argv strings and pointer array are packed into the frame record
in the arena, not on the C stack. This is required because the script
evaluator is iterative (a loop, not recursive C calls). When proc A
calls proc B, there is only one set of C-stack locals — B's dispatch
would clobber A's argv.

**Why C stack doesn't work:**

```text
A: motor $gain ; _set x (B 10 20) ; other_cmd $x $gain
                                     ^
                                     A needs its argv intact here
```

A dispatches `motor $gain` — argv strings are `"motor\075\01\0"`.
Then A calls proc B. B needs its own argv for `_add @1 @2`. If B
writes into the same scratch buffer, A's argv strings are gone. When
B returns and A continues with `other_cmd`, `@1` and `@2` (which
point into A's argv) are clobbered.

Each frame owns its argv in the arena byte stream — sized to actual
content, not a fixed buffer.

**Frame packing at entry:**

1. Write frame header (tag + varHP + retAddr + scriptEnd): ~11 bytes.
2. Write argc (1 byte).
3. Tokenize/expand each argument into a packed null-terminated string,
   appended to the arena. Literal tokens are copied. `$name` tokens
   are looked up and formatted (INT → decimal string, STR → verbatim).
4. Write argc pointers, each pointing back into step 3's region.

A call like `motor $gain $dir` (gain=75, dir=1) produces:

```text
[ header_11B | argc=3 | "motor\0" "75\0" "1\0" | ptr0 ptr1 ptr2 ]
                         10 bytes                  12 bytes (32-bit)
total: 11 + 1 + 10 + 12 = 34 bytes
```

No hardcoded `XELP_ARGVBUFSZ` or `XELP_ARGV_MAX` in the frame — it
holds exactly the strings and pointers for this call's actual argc.

**Positional parameters** (`@1`, `@2`, ...) read directly from the
pointer array in the frame: `@n` = `pointers[n]`. O(1), no scanning.

**Dispatch to C handler:** pass the pointer array address and argc
from the frame directly to the handler. The pointers already point
into the frame's packed strings. No copying, no temp arrays.

**Non-script path is unchanged.** The existing `XelpParseXB` (no script
engine, no procs) continues to use C-stack locals for argv scratch.
The arena-based scratch is only used when the script evaluator is
active. This means non-script builds pay zero RAM for the arena.

| ID | Requirement |
| --- | --- |
| EX-01 | Variable expansion (`$name`, `@n`) MUST produce string representations packed into the frame's argv region in the arena byte stream. INT values MUST be formatted as decimal strings. STR values MUST be copied verbatim. |
| EX-02 | Expansion MUST be invisible to C handlers. Handlers receive standard `(ths, argc, argv)` with null-terminated strings regardless of whether arguments came from literals, variables, or nested evaluation. |
| EX-03 | Each frame MUST own its argv strings and pointer array in the arena byte stream, sized to the actual argc and token lengths of that call. No hardcoded `XELP_ARGVBUFSZ` or `XELP_ARGV_MAX` per frame. Callee frames cannot corrupt caller argv. |
| EX-04 | Positional parameters (`@1`, `@2`, ...) MUST be pointers into the frame's argv pointer array. `@n` = `pointers[n]`. No separate storage for positional parameters. |
| EX-05 | The non-script `XelpParseXB` path MUST continue to use C-stack locals for argv scratch. Arena-based argv is only used when the script evaluator is active. Non-script builds pay zero arena cost. |
| EX-06 | The script dispatch path MUST reuse the existing `_xelpBuf2Argv` tokenization logic with variable expansion added as a pre-copy step, writing packed strings directly into the arena at the current stack pointer. |
| EX-07 | INT-to-string formatting during `$var` expansion MUST use a shared internal utility (`_xelpIntToStr`) that writes decimal digits to a destination buffer. The same utility SHOULD be reusable by C handler helpers and diagnostic output. Exposing it as public API (`XelpIntToStr`) is recommended but deferred to implementation. |
| EX-08 | The `$var` expansion path MUST be type-dispatched (switch on value kind). Adding a new type (e.g. float) MUST only require adding a formatter case and the formatter function — no structural changes to the expansion loop or arena layout. |

### Sigil semantics and type inference **(SG‑\*, TI‑\*)**

The `$` sigil uses **expand semantics**: `$name` always means "get the
current value of the variable named `name`." Without `$`, a token in
argument position is the literal text — the name itself, not a reference.
This is the Tcl/bash model. The complete rule is one sentence: **`$` means
expand, always.**

The consequence: in `_set x 10`, bare `x` is the target name and `10` is a
literal value. In `_set y $x`, bare `y` is the target name and `$x` is
expanded to the value of `x`. Both uses of `$` and bare tokens are
consistent because the rule never changes — `$` = expand, bare = literal.

Indirect assignment (`_set $ptr value` where `$ptr` holds a variable name)
works because `$ptr` expands to a name string which `_set` uses as its
target. No extra machinery needed.

**Type inference for `_set`:** the evaluator inspects the raw token (before
quote stripping) to determine the stored type. Quotes are the explicit type
annotation:

* Unquoted token that passes `XelpParseNum` → **INT**
* Quoted token (`"..."`) → **STR**, regardless of content
* Unquoted token that fails `XelpParseNum` → **STR**
* Value from `$` expansion → **preserves the source variable's type**
* Value from `()` subexpression → **preserves the return type**

This means `_set x 3` stores INT(3), `_set x "3"` stores STR("3"), and
`_set x $y` copies whatever type `$y` has. The quote mark is the escape
hatch when the user needs a string that looks like a number.

**Two-path type handling:** type information lives in **arena entries**
(typed variable system), not in argv strings. When a typed variable is
expanded into argv for a C handler, it becomes a plain string — the type
tag does not survive the argv boundary. C handlers see strings. Builtins
see types.

| ID | Requirement |
| --- | --- |
| SG-01 | `$name` MUST always mean "expand the variable named `name` to its current value." This rule MUST have no exceptions — `$` is the expansion operator in every context. |
| SG-02 | A bare word in argument position MUST be the literal text, not an implicit variable reference. `_set x 10` means the target name is the string `"x"`, not "the variable x." |
| SG-03 | Expansion of an undefined variable MUST produce an error (`XELP_E_ERR`), not a silent empty string. |
| SG-04 | Indirect assignment (`_set $ptr value` where `$ptr` holds a name string) MUST work by expanding `$ptr` first, then using the resulting string as the target name. No special syntax needed. |
| TI-01 | `_set` MUST infer stored type from the raw token. Unquoted tokens that pass `XelpParseNum` → INT. Quoted tokens → STR. Unquoted tokens that fail `XelpParseNum` → STR. |
| TI-02 | Values arriving from `$` expansion or `()` subexpressions MUST preserve the source type. `_set y $x` copies x's type tag. `_set z (+ 1 2)` stores INT(3). |
| TI-03 | Builtins (`_set`, `_if`, etc.) MUST have access to raw tokens before quote stripping for type inference. This is provided by the builtin dispatch path (DA-02). |
| TI-04 | C handlers MUST receive argv with quotes stripped and all values as plain strings (EX-02). Type information MUST NOT leak into the C handler argv — types live in arena entries only. |
| TI-05 | `0xFF`, `0b1010`, and other `XelpParseNum`-compatible formats MUST parse as INT when unquoted. `"0xFF"` (quoted) MUST store as STR. |
| TI-06 | When `XELP_ENABLE_FLOAT` is disabled, unquoted tokens like `3.14` that fail `XelpParseNum` MUST store as STR (not error). When `XELP_ENABLE_FLOAT` is enabled, they MUST parse as the appropriate float type. |

### Variable lifecycle **(VL‑\*)**

Variables are declared by first `_set` and live until their frame pops
(or instance reset for root frame). Type is fixed at declaration — like
C, not like Python.

**Type immutability:** the first `_set x 10` declares x as INT. All
subsequent `_set x <value>` must produce INT. `_set x "hello"` after
x is INT → `XELP_E_ERR`. This eliminates arena entry resizing for
type changes and makes memory usage predictable.

**INT reassignment:** overwrite value in place. Entry size never changes.
Zero arena cost.

**STR reassignment, same length or shorter:** overwrite in place. Entry
size unchanged, tail bytes unused but bounded.

**STR reassignment, longer:** `memmove` the current frame's heap entries
between HP and the target entry toward the free space by `delta` bytes.
Update HP. Write new string data into the expanded entry. Overflow check
(`SP + delta <= HP`) before the move — `XELP_E_ERR` if arena full.

```text
Before:  ... free ... | var_c | var_b | var_a("hi") | parent_vars
                        ^
                        HP

_set var_a "hello world"    # needs 9 more bytes, delta = 9

After:   ... free | var_c | var_b | var_a("hello world") | parent_vars
                    ^
                    HP (moved 9 bytes toward stack)
```

The moved data is only current-frame entries between HP and the target
— typically 30-100 bytes for a frame with a few variables. Lookup is
by scan (name hash), so no pointers need updating — just HP. The cost
is in the same neighborhood as the lookup scan that already found the
variable.

**No `_unset`.** Variables die when their frame pops (automatic). Root
frame variables persist until `XelpInit` (instance reset). Dynamic
`_func` entries persist until instance reset. This matches C semantics
— declaration is lifetime, scope is cleanup.

| ID | Requirement |
| --- | --- |
| VL-01 | The first `_set` for a name MUST declare the variable with the inferred type (TI-01). Subsequent `_set` to the same name MUST preserve the type. Type-changing reassignment (`_set x 10` then `_set x "hello"`) MUST produce `XELP_E_ERR`. |
| VL-02 | INT reassignment MUST overwrite the value in place. Zero arena growth. |
| VL-03 | STR reassignment with same-length or shorter string MUST overwrite in place. |
| VL-04 | STR reassignment with longer string MUST `memmove` current-frame heap entries between HP and the target to make room. HP MUST be updated. Arena overflow (`SP + delta > HP`) MUST produce `XELP_E_ERR`. |
| VL-05 | Variables MUST be scoped to their declaring frame. Frame pop reclaims all frame-local variables automatically (AR-04). Root frame variables persist until instance reset (`XelpInit`). |
| VL-06 | No `_unset` or `_clear` builtin. Scope-based lifetime is the only cleanup mechanism. |

### Truthiness, predicates, and regression tests **(TH‑\*, TR‑\*)**

Coercion bugs (`0` vs **`"0"`**, **`""`**, **`"zero"`**, NUL bytes, **`NULL`/3VL**) ship hidden until field failure. **`_truthy` must have one frozen table.** Cross-language foot-guns and suggested extra **`TR-*`** IDs live in **`dev/xelp_truthiness_trap_catalog.md`**.

Why canonical helper: **`_truthy`** centralizes coercion so **`_if`** cannot sprout one-off spooky rules without documentation.

#### Normative anchors

| ID | Requirement |
| --- | --- |
| TH-01 | truthy SHOULD define the canonical typed-value → boolean mapping for if; if MUST NOT diverge without documenting any intentional split from truthy. |
| TH-02 | INT zero MUST behave falsy; nonzero MUST behave truthy. |
| TH-03 | Empty string MUST be falsy; nonempty strings SHOULD default truthy unless numeric narrowing (TH-05) applies. Strings MUST NOT fall through stray atoi paths accidentally. |
| TH-04 | Pure whitespace strings MUST declare falsy vs truthy intentionally (recommended falsy, no trimming unless _trim). |
| TH-05 | Optional narrowing SHOULD be explicit globally: strings that fully parse via XelpParseNum/CLI-compatible rules obey INT logic; malformed strings revert to STR truthiness—with tests documenting "0", prefixes, radix forms. |

#### Sentinel interplay

| ID | Requirement |
| --- | --- |
| TH-06 | NIL / unset $var MUST pick falsy-vs-error semantics once; interplay with downstream ERR propagation documented beside EH-04. |
| TH-07 | ERR-valued nested expressions occupying _if predicate slots MUST declare policy (falsy+warn, scripted abort, or hard stop)—see EH-*. |

#### Mandatory scripted regression matrix

Implementations MUST cover at least:

| Case ID | Kind | Fixture | Expected judgement |
| --- | --- | --- | --- |
| TR-INT-001 | INT | literal/bind 0 | falsy |
| TR-INT-002 | INT | -1, 42, etc. | truthy |
| TR-STR-001 | STR | empty | falsy |
| TR-STR-002 | STR | "hello" | truthy when TH-05 disabled |
| TR-STR-003 | STR | "0" | matches declared TH-05 profile |
| TR-STR-004 | STR | spaces/tabs-only | matches TH-04 |
| TR-CMP-001 | cross | _eq or equivalent compares stored INT zero | aligns with INT truth table without hidden coercion |

Optional expansion: **`TR-NIL-*`**, **`TR-ERR-NEST-*`**. Tests SHOULD live beside JumpBug harnesses referencing ROM-literal parity strings.

### Error handling model **(EH‑\*)**

Field engineers need deterministic stop/continue semantics; manufacturing lines may widen tolerances deliberately.

| ID | Requirement |
| --- | --- |
| EH-01 | Unknown commands / strict arity breaches MUST NEVER masquerade as silent success—classify failures for script introspection/policy (API TBD). |
| EH-02 | Nested invocation failures MUST leave parent framing deterministic (ERR/abort aligns with R‑02). |
| EH-03 | Default SHOULD halt current script statement yet leave engine ready for subsequent interactive input; Manufacturing-style profiles MAY allow continue-on-error. |
| EH-04 | Tie TH‑06/TH‑07 documentation to executable tests (TR‑ERR‑* optional suite). |
| EH-05 | Warning-class outcomes SHOULD permit continued execution unless callee documents halt-on-warn symmetry with XELPWWARN philosophy. |

### Addressing requirements **(D‑\*)**

Translates ergonomics bullets into contractual clarity for tooling and training.

| ID | Requirement |
| --- | --- |
| D-01 | $name MUST denote named variables scoped per script/instance rules. |
| D-02 | @n MUST refer exclusively to scripted positional parameters—not bash $1. |
| D-03 | @name, @# SHOULD exist together or not at all; partial shipping drops cleanly without confusing half-features. |
| D-04 | Bare words MUST follow deterministic literal-vs-command distinctions between statement head and argument tails. |

### Control flow (initial milestone) **(F‑\*)**

First ship focuses on Turing-light procedural charts without looping sugar.

| ID | Requirement |
| --- | --- |
| F-01 | MUST ship set, if, next, goto, labeled lines scoped inside one artifact. Duplicate label detection is a tooling/lint concern (see F-18), not an interpreter requirement. |
| F-02 | Structured loops remain optional later—they MUST NOT gate MVP readiness (paired with Anti-goal A‑03). |
| F-03 | Truth tables above plus mandated TR fixtures gate if; goto must not accidentally span unrelated blobs. |

#### `_if` concrete syntax

Grammar:

```text
_if <condition> _then <true-cmd> [_else <false-cmd>]
```

`_if`, `_then`, and `_else` are ordinary tokens to the existing PSM — no new
parser states required. The `_if` handler receives the full argv, locates
`_then` and `_else` token indices, slices, and dispatches each sub-command
string via `XelpParse`.

Examples:

```text
_if $x _then led 1                         # variable truthiness
_if $x _then led 1 _else led 0             # with else clause
_if check_sensor 3 _then log ok            # command truthiness (mR[0])
_if $err _then :error_handler              # jump to label
_if $mode _then motor $gain _else stop     # with variable expansion
```

Condition evaluation:

* **Command as condition** — `_if` dispatches the condition tokens via
  `XelpParse`. The condition is truthy when `ths->mR[0] == XELP_S_OK` (0)
  after the command returns.
* **Variable as condition** — when the condition is a single `$name` token,
  evaluate per the truthiness table (TH-01 through TH-07): INT nonzero =
  truthy, nonempty string = truthy, zero / empty / NIL = falsy.
* `_else` is optional. When absent and condition is falsy, execution
  continues at the next statement.

Design constraints:

* `_then` / `_else` keywords cost line budget (~11 chars) but are unambiguous
  tokens with no collision risk. `:` was rejected as a delimiter because it
  collides with label syntax (`:name`). `?` was rejected because it may appear
  in command arguments.
* On a 64-byte command buffer, `_if x _then y _else z` is 25 chars — leaves
  39 for embedded commands which tend to be short (`led 1`, `adc 0`).
* `_else` clause is a single command (with its arguments). Compound else
  requires `_goto :label` or semicolon-separated script.
* Nesting via labels: `_if a _then :handle_a _else _if b _then :handle_b` is
  valid but discouraged for readability — prefer labels for multi-way branching.

| ID | Requirement |
| --- | --- |
| F-04 | `_if` syntax MUST be: `_if <condition> _then <true-cmd> [_else <false-cmd>]`. |
| F-05 | `_if` MUST evaluate condition truthiness per TH-01 through TH-07 when the condition is a variable. When the condition is a command, truthiness MUST be determined by `mR[0] == XELP_S_OK`. |
| F-06 | `_if` handler MUST NOT require new PSM states — `_if`, `_then`, `_else` are ordinary tokens dispatched by the existing tokenizer. |
| F-07 | `_then` and `_else` clauses each contain one command with its arguments. No implicit compound statements within a clause. |

#### Jump keywords and labels

##### Label syntax: `:name`

Labels use a colon prefix: `:name`. The colon is already a valid token
character — no tokenizer changes needed.

Sigil summary for the script surface:

```text
$name     variable
@n        positional parameter
_keyword  reserved language builtin
:name     label
```

##### Label definitions vs references

A **label definition** is `:name` appearing as the **first token on a line**
(or after `;`). It marks a jump target in the script buffer.

A **label reference** is `:name` appearing as an argument to `_next`, `_goto`,
`_if _then`, or `_if _else`. It names the target to jump to.

```text
:start                        # definition (first token on line)
  read_sensor
  _if $val _then :done        # reference (argument to _if _then)
  delay 100
  _goto :start                # reference (argument to _goto)
:done                         # definition
  log "finished"
```

This distinction is structural, not syntactic — the same token `:name`
serves both roles. The scanner identifies definitions by position (first
token on a line), so these are naturally excluded:

* `:name` inside a comment (`# see :foo`) — tokenizer already skips comments.
* `:name` as a mid-line argument (`_goto :foo`) — not first-token, so not a
  definition site, only a reference.
* `:name` inside a quoted string (`"jump to :foo"`) — inside quotes, not a
  token.

##### Two jump keywords: `_next` and `_goto`

Two keywords with permanently distinct semantics — the grammar never changes
between versions:

**`_next`** — **execute or skip forward.**

* `_next command arg arg` — execute the command. `_next` dispatches whatever
  follows it.
* `_next :label` — the argument starts with `:`, so scan forward from the
  current parse position to find the label definition. Stop at the first
  match. If `:label` is not found ahead, error.

`_next` is always forward, always from the current position. No step budget
required. Ships in MVP.

```text
  read_sensor
  _if $val _then _next :done    # skip ahead
  _next led 1                   # execute command
  _next :done                   # skip ahead unconditionally
:done
  log "finished"
```

**`_goto :label`** — **scan from frame start (beginning of the function
body).** Always scans from the top. Can reach any label in the function,
including those before the current position. Enables loops. Requires S-01
step budgeting to prevent runaway scripts. Ships after MVP.

```text
:top                             # label at top of function
  read_sensor
  _if $val _then _next :done     # forward skip with _next
  delay 100
  _goto :top                     # backward jump with _goto (loop)
:done
  log "finished"
```

The frame's XelpBuf provides the anchor — `.s` is always the top of the
function body, so `_goto` resets the scan position to `frame.s` and searches
from there. `_goto` always finds the **first** matching label from the top.

**Why two keywords instead of one:** A single keyword that sometimes scans
forward and sometimes from frame start would create version-dependent or
context-dependent behavior. Two keywords make the scan direction explicit in
the source text. `_next` is always forward-from-current, `_goto` is always
from-frame-start — both definitions are permanent.

##### `_if` interaction

`_if` clauses can use either keyword or a bare label reference:

```text
_if $x _then _next :done        # forward skip
_if $err _then _goto :retry     # backward jump (when _goto ships)
_if $x _then :done              # bare label = implicit _next :done
_if $x _then led 1              # execute command
```

Bare `:label` in an `_if` clause is syntactic sugar for `_next :label`
(forward-only). To jump backward from an `_if`, use `_goto` explicitly.

##### Block-skip pattern with `:_end`

`_next :label` with a reserved repeatable label provides a lightweight
block-comment / block-skip idiom:

```text
_next :_end
  calibrate_adc 0
  calibrate_adc 1
  set_gain 12
  verify_output
:_end
run_main
```

`:_end` is a **reserved label name** that is explicitly allowed to appear
multiple times in the same script. Each `_next :_end` finds the nearest
`:_end` ahead of it — forward-only scan guarantees deterministic matching.

Multiple blocks:

```text
_next :_end
  skip block 1
:_end
do_something
_next :_end
  skip block 2
:_end
do_more
```

To re-enable a skipped block, comment out or delete `_next :_end`. The
`:_end` label can stay — labels are no-ops when nothing jumps to them:

```text
#_next :_end
  calibrate_adc 0            # runs again
  calibrate_adc 1            # runs again
:_end                        # harmless, costs nothing
run_main
```

`:_end` works with `_next` because `_next` scans forward and stops at the
first match. `_goto :_end` would find the **first** `:_end` from the top of
the function, which is almost certainly wrong — using `_goto` with `:_end`
is a script bug, not something the interpreter polices.

##### Duplicate labels

The interpreter does **not** detect or reject duplicate label definitions.
It is a simple forward-scanning byte parser — no symbol table, no pre-pass,
no label registry. Each jump keyword scans from its defined start point
(`_next`: current position; `_goto`: frame start) and stops at the first
match.

Consequences:

* **`_next`** with duplicate labels: always correct. Finds the nearest match
  ahead. This is the basis of the `:_end` block-skip pattern.
* **`_goto`** with duplicate labels: always finds the first definition from
  the top of the function. If that's not the intended target, it's a script
  bug.

Duplicate label detection is a **tooling/lint concern**, not an interpreter
responsibility. Host-side linters, test harnesses, or IDE integrations
SHOULD flag duplicate labels (except `:_end`) as warnings. The interpreter
stays simple.

##### Label scoping

Labels are scoped to a single frame / script buffer. A jump MUST NOT cross
frame boundaries or reach into a different script artifact.

| ID | Requirement |
| --- | --- |
| F-08 | Labels MUST use `:name` syntax. `:` prefix is reserved for labels in the script namespace. |
| F-09 | Label definitions MUST be the first token on a line (or after `;`). Mid-line `:name` tokens are references, not definitions. |
| F-10 | Jump keywords MUST scan for label definitions by position (first-token-on-line), not by substring match, to avoid false matches in comments, arguments, or quoted strings. |
| F-11 | `_next :label` MUST scan forward only from the current parse position. Stops at the first match. If the label is not found ahead, it MUST error. No step budget required. |
| F-12 | `_next command arg ...` MUST dispatch the command. `_next` executes whatever follows it — a label reference triggers a forward jump, anything else is a command invocation. |
| F-13 | `_goto :label` MUST scan from the frame start (beginning of function body). Stops at the first match. Can reach labels before or after the current position. `_goto` MUST NOT ship without S-01 step budgeting. |
| F-14 | Labels MUST be scoped to a single frame / script buffer. Cross-frame jumps MUST NOT be supported. |
| F-15 | `_if` clauses MAY use label references (`:name`) as the true/false target, equivalent to `_next :label` (forward-only). Backward jumps from `_if` require explicit `_goto`. |
| F-16 | `_next` and `_goto` have permanently fixed semantics. `_next` is always forward-from-current. `_goto` is always from-frame-start. These definitions MUST NOT change between versions. |
| F-17 | `:_end` is a reserved label name. It MAY appear multiple times in a script. It is the canonical block-skip terminator for use with `_next :_end`. |
| F-18 | The interpreter MUST NOT detect or reject duplicate label definitions at runtime. Duplicate detection is a tooling/lint responsibility, not an interpreter responsibility. |

#### Jumps inside parenthesized expressions

Jumps (`_next`, `_goto`) can appear inside `()` subexpressions. People will
type it — the behavior must be explainable, not undefined.

When the evaluator processes `()`, it pushes **continuation markers** (CONT
records) onto the arena stack to track partially-built outer expressions. If
a jump executes inside `()`, those continuations are orphaned:

```text
_set x (+ 1 (_goto :somewhere))

After pre-pass:  _set x ( + 1 ( _goto :somewhere ) )
stack before _goto:
[ ... | CONT{_set, argc=2} | CONT{+, argc=2} | ... ]

_goto executes → jump to :somewhere
orphaned CONTs left on stack
```

**Policy: clean up on jump (approach B).** When `_next` or `_goto` executes,
pop any pending CONT records from the stack before jumping. The partially
evaluated expression is cleanly abandoned. No arena space leak.

**Invariant:** After any jump, the stack contains only FRAME markers and
result entries — no orphaned CONT records. If fuzzing finds a case where
this invariant breaks, fix the cleanup loop, not the invariant.

The behavior is explainable to users: "jumps abandon any in-progress
expression evaluation. The expression result is discarded."

Edge cases:

* **`_goto` landing between `(` and `)`** — e.g. jumping to a label that
  happens to sit on a line between `(` and `)` in the source text. The
  interpreter doesn't know it's "inside" an expression — it just sees tokens
  at the landing site. If those tokens aren't valid statements, the evaluator
  errors normally (CMDNOTFOUND). Bad script, explainable behavior.

* **`_next` scanning past `)`** — `_next :label` scans forward in the raw
  script text. It doesn't track `()` nesting. If `:label` is past a `)`,
  the jump works, continuations are cleaned up, the expression is abandoned.
  Same as any other jump-out-of-expression.

* **`:label` inside `()`** — a `:name` mid-line inside `()` is not a label
  definition (F-09: labels must be first-token-on-line). Jump scans won't
  find it. Already handled.

| ID | Requirement |
| --- | --- |
| F-19 | When `_next` or `_goto` executes, the engine MUST pop any pending CONT (continuation) records from the arena stack before jumping. After any jump, the stack MUST contain only FRAME markers and result entries — no orphaned continuations. |
| F-20 | Jumps inside `()` subexpressions MUST be allowed. The partially evaluated expression is abandoned. The behavior is defined, not undefined. |
| F-21 | Fuzzing and edge-case testing MAY reveal cleanup scenarios not anticipated at design time. The CONT cleanup mechanism MUST be tunable without changing the jump semantics (F-16). |

### Variable scoping **(SC‑\*)**

Variables are local to the frame that created them. A child proc cannot see
or modify its parent's variables. Data flows through arguments (`@1`, `@2`)
and return values — explicit, traceable, no invisible coupling.

Why local-only:

* Matches C's mental model — local variables are local.
* The arena design supports it naturally: frame exit resets HP to the saved
  varHP, releasing all variables created in that frame. Parent variables
  live below the saved varHP and are untouched but also unreachable.
* Dynamic scoping (Tcl-style `upvar`) makes function behavior depend on who
  calls them — a debugging nightmare on a UART terminal with no stack traces.
* `mR[]` already provides a simple shared-state channel for cases where
  caller and callee need to exchange data outside of arguments/returns.

For shared persistent state (e.g. configuration values that survive frame
exits), a future `_global` keyword could store variables in a reserved region
at the bottom of the heap, below all frames. Not MVP — `mR[]` and explicit
arguments cover the common cases.

```text
_proc calibrate
  _set gain 12          # local to calibrate's frame
  _set offset 3         # local to calibrate's frame
  motor_setup $gain $offset
  _return $gain

_proc main
  _set x (calibrate)    # $x = 12 (return value)
  # $gain is NOT visible here — it was local to calibrate
  # $offset is NOT visible here
  _mr 0 $x              # use mR[] if C code needs the value
```

| ID | Requirement |
| --- | --- |
| SC-01 | Variables created by `_set` MUST be scoped to the frame that created them. Variable lookup MUST NOT walk parent frames. |
| SC-02 | Frame exit MUST release all variables created in that frame (HP reset to saved varHP). Parent variables MUST survive child frame exit. |
| SC-03 | Data between frames MUST flow through arguments (`@n`), return values (`_return`), or the `mR[]` mailbox. No implicit sharing. |
| SC-04 | A future `_global` keyword MAY store variables in a persistent heap region that survives frame exits. This is not MVP. |

#### CLI as root frame

Script builtins typed at the interactive CLI run in the **root frame** —
no script buffer, no proc, no frame marker on the stack. Behavior:

**Works at the CLI:**

* `_set x 10` — creates a variable in the root frame. Persists until
  instance reset (no frame exit to reclaim it).
* `$x` — variable expansion works. Root frame variables accumulate.
* `(+ 1 2)` — paren pre-pass runs, evaluator handles nesting. The result
  is computed then **discarded** (no consumer at statement level). Side
  effects of the inner command still run. To capture: `_set x (+ 1 2)`.
  To print: `_print (+ 1 2) "\n"`.
* `_if $x _then led 1` — single-line conditional, works.
* `_mr 0 42` — reads/writes mailbox, same as in script.
* Math builtins, `_print`, etc. — all work normally.

**Errors at the CLI:**

* `_goto :label` — no script buffer to scan. Error.
* `_next :label` — no script buffer ahead. Error.
* `_return` — no frame to return from. Error.

**No-ops at the CLI:**

* `:label` as first token — harmless, nothing will jump to it. No-op.
  Allows pasting script fragments that contain labels — the commands
  between labels still run.

**`_next command arg`** works (just dispatches the command). Only the
`:label` form errors.

**Stack cleanup:** After each top-level statement at the CLI, any unclaimed
result entries on the stack MUST be discarded. The stack resets to the arena
base after every statement dispatch. No unbounded growth.

| ID | Requirement |
| --- | --- |
| SC-05 | Script builtins typed at the CLI MUST run in the root frame. Variables persist until instance reset. |
| SC-06 | `_goto`, `_next :label`, and `_return` MUST error at the CLI (no script buffer / no frame). `:label` definitions MUST be silent no-ops. |
| SC-07 | After each top-level CLI statement, unclaimed result entries on the arena stack MUST be discarded. The stack MUST NOT grow unboundedly across interactive commands. |

### Parenthesized subexpressions **(PE‑\*)**

`(command arg arg)` in argument position is nested evaluation — evaluate the
inner command, use the result as the argument. This is the composition
mechanism for script expressions.

#### Evaluation model: continuations on the arena stack

The evaluator builds argv for a command by scanning tokens left-to-right.
When it hits `(`, it pushes a **continuation marker** (CONT record) onto
the arena stack and begins evaluating the inner expression. When the inner
expression completes, its result is on the result stack. The evaluator pops
the continuation, formats the result into the outer command's argv, and
continues.

```text
_set x (+ 1 (* 2 3))

After paren pre-pass:  _set x ( + 1 ( * 2 3 ) )

1. start building argv for _set: argv[0]="_set", argv[1]="x"
2. hit '(' token → push CONT{_set, partial_argc=2}
3. start building argv for +: argv[0]="+", argv[1]="1"
4. hit '(' token → push CONT{+, partial_argc=2}
5. evaluate * 2 3, hit ')' → result INT 6 on result stack
6. pop CONT for +, format 6 → argv[2]="6", dispatch + 1 6 → INT 7
7. hit ')' → pop CONT for _set, format 7 → argv[2]="7", dispatch _set x 7
```

This is iterative — no C recursion. The arena stack holds the pending state.
Max nesting depth is bounded by arena space, not a compile-time constant.

**CONT record** (arena stack, variable-length tagged record):

```text
[ 0xF2 | outer_parse_pos | partial_argc_1B | partial argv data... ]
```

The exact CONT format is internal to the engine (AR-05 applies).

#### Tokenizer architecture: shared token-boundary scanner

The tokenizer's front-half — skip whitespace, track quote state, find
token start/end — is factored out as a shared read-only primitive:

```c
/* Advance *pos past whitespace/comments, report next token span.
   Read-only: does not modify src.  Returns 0 when no more tokens. */
int _xelpNextTokSpan(const char *src, int len, int *pos,
                     int *tok_start, int *tok_end);
```

This is the single source of truth for "what is a token." Two consumers
use it with different back-halves:

1. **`_xelpBuf2Argv`** (existing CLI path): calls `_xelpNextTokSpan` in a
   loop, copies each token span into scratch, null-terminates, handles
   escape expansion, builds the argv pointer array. Same behavior as today,
   just refactored to use the shared front-half.

2. **Paren-expansion pre-pass** (script path): calls `_xelpNextTokSpan` in
   a loop, copies each token span into arena scratch. For each token, checks
   if the first char is `(` or the last char is `)` and splits accordingly
   (emit `(` + space + rest, or prefix + space + `)`). Both can apply:
   `(foo)` → `( foo )`.

Because both paths use the same scanner, there is no risk of divergence —
quote rules, comment detection, and escape handling are defined in one
place. If tokenization rules change, both paths inherit the change.

The scanner is read-only (source may be ROM). The pre-pass writes to arena
scratch; `_xelpBuf2Argv` writes to its own scratch buffer. Neither modifies
the source.

#### Paren-expansion pre-pass

Parentheses do not require manual space separation. The pre-pass uses the
shared token-boundary scanner to find each token, then checks first/last
chars for `(`/`)` and splits them with spaces during the copy to arena
scratch.

```text
_set x (* (+ $foo 4) 3)          # natural form — pre-pass splits parens
_set x ( * ( + $foo 4 ) 3 )      # explicit spaces — also works (idempotent)
```

How splitting works per token:

* Token starts with `(`: emit `(` + space + rest of token.
  `(+` → `( +`
* Token ends with `)`: emit token-minus-`)` + space + `)`.
  `3)` → `3 )`
* Both: `(foo)` → `( foo )`
* Multiple: `((foo` → `( ( foo`, `bar))` → `bar ) )`
* Neither: emit token as-is.
* Already-spaced `(` or `)`: token is just `(` or `)` — no rest to split,
  emitted as-is. Idempotent.

The pre-pass never examines content inside quotes or comments — the shared
scanner already skips those. No parallel quote-tracking logic needed.

Cost: ~60–80 bytes of code on ARM Thumb (the split-and-copy back-half; the
scanner is shared code that already exists), plus ~2–10 bytes of arena
scratch growth per line. The scanner factoring may actually reduce total
code size since `_xelpBuf2Argv` no longer contains its own inline
front-half.

After the pre-pass, `(` and `)` are ordinary whitespace-delimited tokens.
The script evaluator recognizes them by value and uses them to drive the
continuation-based nesting model.

#### Command-position variables

A `$var` in command position (first token) expands to its string value
before command lookup. This enables indirect dispatch:

```text
_set cmd "read_adc"
$cmd 0                    # expands to: read_adc 0
_set x ($cmd 0)           # expands to: (read_adc 0)
```

No special syntax needed — variable expansion happens before dispatch for
all token positions including position 0. This is not `eval` — the variable
holds a command name, not a full command line with arguments.

#### String execution (`_run`) — deferred

`_run $somestring` where `$somestring` = `"read_adc 0"` (a full command line
in a string) is `eval()`. It requires re-tokenizing the string value. This
is powerful but adds complexity:

* Does `_run` create a new frame? (Probably yes — isolates variables.)
* Can `_goto` in the string reach labels outside the string? (No — scoped.)
* Can the string contain `()` nesting? (Presumably yes — full evaluation.)

Deferred past MVP. Command-position `$var` gives the useful 90% case
(indirect dispatch by name) without runtime code generation. `_run` can be
added later without changing any current design decisions.

| ID | Requirement |
| --- | --- |
| PE-01 | `(command arg arg)` in argument position MUST evaluate the inner command and substitute the result as a single argument to the enclosing command. |
| PE-02 | The tokenizer's front-half (whitespace skipping, quote tracking, token boundary detection) MUST be factored into a shared read-only primitive (`_xelpNextTokSpan`). Both `_xelpBuf2Argv` and the paren pre-pass MUST use this primitive. One source of truth for "what is a token." |
| PE-03 | The paren pre-pass MUST use the shared token-boundary scanner to find tokens, then split tokens that start with `(` or end with `)` by inserting spaces during the copy to arena scratch. The pre-pass MUST be idempotent and MUST NOT modify the original source (which may be ROM). |
| PE-04 | The evaluator MUST use continuation markers (CONT records) on the arena stack for `()` nesting. Evaluation MUST be iterative — no C recursion for nested expressions. |
| PE-05 | Max `()` nesting depth MUST be bounded by arena space, not a compile-time constant. Arena overflow during nesting MUST be a hard error (AR-07). |
| PE-06 | `()` handling is a script-evaluator concern. The non-script `_xelpBuf2Argv` path MUST NOT change in behavior (it is refactored to use the shared scanner, but produces identical results). |
| PE-07 | `$var` in command position (first token) MUST expand before command lookup. Variable expansion applies to all token positions. |
| PE-08 | `_run` (string-as-command-line execution) is deferred past MVP. It MUST NOT be required for indirect dispatch — command-position `$var` (PE-07) covers that case. |

### Output builtin **(IO‑\*)**

One output command: `_print`. Newlines are explicit via escape sequences
in quoted strings — the existing `XELP_ESC_MAP` tokenizer machinery
handles `\n` → `0x0A` and `\t` → `0x09` during tokenization, before
`_print` ever sees the argv.

#### `_print`

`_print` concatenates all its arguments with no separator and no trailing
newline. The caller controls all whitespace and line endings explicitly.

```text
_print "hello world\n"                              # hello world + newline
_print "the value is: " $x "\n"                     # the value is: 10 + newline
_print "x=" $x " y=" $y "\n"                        # x=10 y=20 + newline
_print "no newline here"                             # partial output
_print "line1\nline2\n"                              # two lines (tokenizer expands \n)
_print "col1\tcol2\n"                                # tab-separated
_print "this is an update\n" "value: " $x "\n"       # two lines, clean
```

`_print` is a normal command — it receives argc/argv after all expansion
and escape processing has happened. It doesn't interpret its arguments,
perform variable lookup, or reformat values. It prints exactly what's in
argv. Zero arguments = no output.

**No separator, no surprises.** Arguments are concatenated directly. If
you want a space, include it in the string: `_print $x " " $y`. This
avoids the grammar ugliness of trying to suppress an automatic separator
when you don't want one.

**Escape processing is the tokenizer's job, not `_print`'s.** Double-
quoted strings are processed by `XELP_ESC_MAP` during tokenization. By
the time `_print` sees the argv string, `\n` is already a literal `0x0A`
byte. This means every command benefits from escape handling — not just
`_print`.

#### Implementation

Trivial — ~20-30 bytes of ARM Thumb:

```c
XELPRESULT _xelpPrint(XELP *ths, int argc, const char **argv) {
    int i;
    for (i = 1; i < argc; i++)
        XelpOut(ths, argv[i], -1);
    return XELP_S_OK;
}
```

Returns `XELP_S_OK` always (output can't fail — `mpfOut` is void-
returning). Value channel = NIL (side effect, not a value). Using
`_print` inside `()` is legal but useless — the result is NIL.

#### Float interaction

`_print $x` prints whatever the expansion path produced. If
`XELP_FLOAT_EXPAND_HEX` is active, it prints `0f4048F5C3`. If
`XELP_FLOAT_EXPAND_DEC` is active, it prints `3.14`. The expansion
mode is the integrator's choice — `_print` doesn't reformat.

For hand-typed float literals at the CLI, `_print 3.14` just prints the
literal string `3.14` (it's a bare token, not a variable — no expansion
happens).

| ID | Requirement |
| --- | --- |
| IO-01 | `_print` MUST concatenate all arguments (argv[1..argc-1]) with no separator and no trailing newline. Zero arguments MUST produce no output. All whitespace and line endings are the caller's responsibility. |
| IO-02 | `_print` MUST use the instance's `mpfOut` output path, gated by `mOutEnable`. No separate output channel. |
| IO-03 | `_print` MUST print argv strings as-is. No variable lookup, no escape processing, no reformatting. Escape sequences (`\n`, `\t`) are resolved by the tokenizer's `XELP_ESC_MAP` before dispatch — all commands benefit, not just `_print`. |
| IO-04 | `_print` MUST return `XELP_S_OK` with NIL value channel. Using it inside `()` is legal but yields NIL. |
| IO-05 | `_print` SHOULD be registered as a `_`-prefixed builtin. It is available whenever `XELP_ENABLE_SCRIPT` (or equivalent) is enabled. |

#### `_lpad` / `_rpad` — string alignment

`_lpad` is a value-producing builtin that right-aligns a string in a
field of a given width by padding on the left. It's a string operation —
it doesn't know or care whether the argument is a number. By the time
`_lpad` sees it, `$x` is already the string `"42"`.

Naming follows SQL convention: `LPAD` = pad on the **L**eft (right-align),
`RPAD` = pad on the **R**ight (left-align). `_rpad` is reserved for
future use — same interface, padding on the opposite side.

```text
_lpad "42" 6                          # returns "    42"
_lpad "hello" 10                      # returns "     hello"
_lpad "toolong" 3                     # returns "toolong" (no truncation)
```

Composed with `_print` for aligned column output:

```text
:top
  _print "adc0=" (_lpad (read_adc 0) 5) " adc1=" (_lpad (read_adc 1) 5) "\n"
  delay 100
  _goto :top
```

```
adc0=  342 adc1= 1021
adc0=  339 adc1= 1024
adc0=  341 adc1= 1019
```

Implementation: strlen, emit (width - len) spaces, emit string. If
string is longer than width, emit as-is — no truncation. Returns the
padded string as STR on the result stack. ~30-40 bytes ARM Thumb.

Aligns with string pad operations in other languages (Python `rjust`,
JavaScript `padStart`, Rust `format!("{:>w}")`). Operates on strings,
not types.

| ID | Requirement |
| --- | --- |
| IO-06 | `_lpad` MUST right-align its first argument (a string) in a field of width given by its second argument (an integer). Padding character is space. If the string is longer than width, it MUST be returned as-is (no truncation). |
| IO-07 | `_lpad` MUST return the padded string as STR on the result stack. It is a value-producing builtin, intended for use inside `()` composition with `_print`. |
| IO-08 | `_lpad` is a string operation. It MUST NOT perform type detection or numeric parsing. The argument is a string — whatever expansion produced. |

### Dispatch architecture **(DA‑\*)**

The script evaluator has two dispatch paths — one for language builtins,
one for user commands. They are separated by a single-byte test on the
command name.

#### Two-path dispatch

```text
evaluator loop:
    get next command token
    if token[0] == '_':
        _xelpBuiltinDispatch(ths, token, &parseState, &arena)
    else:
        tokenize into argc/argv
        look up in user command table
        call handler(ths, argc, argv)
```

**Builtin path (`_xxx`):** A single internal function that is part of
the evaluator, not a registered handler. It has full access to evaluator
internals — parse position, arena stack/heap pointers, frame state,
continuation markers. Internally dispatches by builtin name (switch,
small hash, or compact table).

**User command path:** Normal C handler dispatch via the command table
(`mpCLIModeFuncs` or equivalent). Handlers receive the standard
`(ths, argc, argv)` signature. No access to evaluator internals.

#### Why builtins can't be normal handlers

Language constructs need evaluator access that `(ths, argc, argv)` doesn't
provide:

| Builtin | Needs |
|---|---|
| `_goto :label` | Modify parse position — scan from frame start |
| `_next :label` | Modify parse position — scan forward |
| `_if ... _then ... _else ...` | Conditional dispatch of sub-commands, control flow branching |
| `_set x 10` | Write to arena heap (variable storage), raw token inspection for type inference (TI-01) |
| `_return` | Pop frame, capture result, reset SP/HP |
| `_proc` | Register callable, or push frame and redirect parsing |
| `_print` | Could be a normal handler, but lives in builtin path for namespace consistency |
| `_lpad` | Could be a normal handler, but lives in builtin path for namespace consistency |
| `_mr` | Reads/writes `mR[]`, pushes to result stack |

Some builtins (`_print`, `_lpad`, `_mr`) don't strictly need evaluator
access — they could be normal handlers. But placing them in the builtin
path keeps the `_` namespace unified: all `_xxx` tokens go through one
dispatch point. No split where some `_` commands are in the builtin
function and others are in the user table.

#### Detection is cheap

`token[0] == '_'` — one byte compare. If match, builtin dispatch. If no
match, user command lookup. No ambiguity because `_<identifier>` is
reserved (B-02) — user commands MUST NOT start with `_`.

This check happens before tokenization into argc/argv. For builtins that
need to manipulate parse state (`_goto`, `_next`, `_if`), the evaluator
can process the raw token stream directly without building a full argv
first. For builtins that work with argc/argv (`_print`, `_set`, `_lpad`),
the builtin dispatch tokenizes as needed.

#### User command lookup

Because builtin dispatch is separated out, the user command lookup path
is independent and can use any strategy:

* **Linear scan** (current `mpCLIModeFuncs`): Simple, zero overhead.
  Fine for small command sets (< ~20 commands). O(n) per dispatch.
* **Sorted array + binary search**: O(log n) lookup, zero extra RAM
  beyond the table itself. Good for medium command sets.
* **Prefix trie**: O(k) where k = command name length. Fast for large
  command sets, compact if commands share prefixes (`motor_start`,
  `motor_stop`, `motor_speed`).
* **Hash table**: O(1) amortized. Costs RAM for the table. Best for
  large command sets where lookup speed matters.

The strategy is an integrator decision — controlled by a compile flag
or by which lookup function the integrator provides. The default can
remain linear scan for backward compatibility. The architecture enables
faster strategies without changing the builtin path or the handler
signature.

#### Dispatch order

For the script evaluator:

1. Check `token[0] == '_'` → builtin dispatch (always wins).
2. User script funcs (dynamic `XELP_VAL_PROC` variables + C-registered
   script func table).
3. User C command table (`mpCLIModeFuncs`).
4. If not found → default handler (`mpfDefCLI`) if set, else
   `XELP_E_CMDNOTFOUND`.

User script funcs are checked before C commands so a user can override
a C handler with a script wrapper if needed. A policy flag MAY gate
whether overriding is allowed (e.g. disallow on production builds,
allow on debug builds).

For the non-script CLI path (`XelpParseXB`), behavior is unchanged —
it searches `mpCLIModeFuncs` as today. Builtins are only available when
the script evaluator is active. This means non-script builds pay zero
for builtin dispatch code.

If a future profile wants builtins at the non-script CLI (e.g. `_print`
without the full script engine), the `_` check could be added to
`XelpParseXB` with a compile flag. But that's not MVP.

| ID | Requirement |
| --- | --- |
| DA-01 | The script evaluator MUST split dispatch into two paths: builtin (`_xxx`) and user command. Detection MUST be a single-byte test (`token[0] == '_'`). |
| DA-02 | Builtin dispatch MUST be a single internal function with full access to evaluator internals (parse position, arena, frame state, continuation markers). It is part of the evaluator, not a registered handler. |
| DA-03 | User command dispatch MUST use the existing handler signature `(ths, argc, argv)`. Handlers MUST NOT have access to evaluator internals. |
| DA-04 | All `_`-prefixed tokens MUST route through builtin dispatch. No split where some `_` commands are builtins and others are in the user command table. The `_` namespace is unified. |
| DA-05 | Builtin dispatch MUST be internal to the script evaluator. Non-script `XelpParseXB` MUST NOT change. Non-script builds pay zero for builtin dispatch. |
| DA-06 | The user command lookup strategy (linear scan, sorted array, trie, hash) SHOULD be an integrator decision. The default MAY remain linear scan for backward compatibility. The architecture MUST NOT constrain the lookup strategy. |
| DA-07 | Dispatch order: (1) builtins (token starts with `_`), (2) user script funcs (dynamic PROC variables + C-registered script func table), (3) user C command table, (4) default handler (`mpfDefCLI`), (5) `XELP_E_CMDNOTFOUND`. User script funcs before C commands enables overriding. A policy flag MAY gate whether overriding is allowed. |

### Math and logic builtins **(MA‑\*)**

Integer math builtins use Lisp-style prefix syntax. All operate on INT
values. Symbol aliases are optional ergonomic shortcuts — same handlers,
shorter names (per B-03 convention). All math builtins push their result
onto the result stack for `()` composition.

#### Arithmetic

| Builtin | Alias | Arity | Notes |
|---|---|---|---|
| `_add` | `+` | variadic | `(+ 1 2 3)` → 6. Two or more args. |
| `_sub` | `-` | binary | `(- 10 3)` → 7. |
| `_mul` | `*` | variadic | `(* 2 3 4)` → 24. Two or more args. |
| `_div` | `/` | binary | Integer division. `(/ 10 3)` → 3. Division by zero → `XELP_E_ERR`. |
| `_mod` | `%` | binary | Integer modulo. `(% 10 3)` → 1. |

#### Logical (boolean, operate on truthiness)

| Builtin | Arity | Notes |
|---|---|---|
| `_and` | binary | Boolean AND. Operands evaluated via truthiness (TH-01). |
| `_or` | binary | Boolean OR. |
| `_not` | unary | Boolean NOT. `(_not 0)` → 1, `(_not 5)` → 0. |

#### Bitwise

| Builtin | Alias | Notes |
|---|---|---|
| `_band` | `&` | Bitwise AND. |
| `_bor` | `\|` | Bitwise OR. |
| `_bxor` | `^` | Bitwise XOR. |
| `_bnot` | `~` | Bitwise NOT (unary). |
| `_shl` | `<<` | Shift left. `(_shl 1 4)` → 16. |
| `_shr` | `>>` | Shift right. `(_shr 16 4)` → 1. |

#### In-place mutation

| Builtin | Notes |
|---|---|
| `_inc` | `_inc x` — increment variable x in place (bare name, like `_set`). Pushes the new value onto the result stack. `(_inc x)` both mutates x and yields the new value. |
| `_dec` | `_dec x` — decrement variable x in place. Same semantics as `_inc`. |

`_inc` and `_dec` take bare names (not `$x`) because they need the
variable name to mutate it — same evaluator access as `_set` (TI-03).
They are syntactic sugar for `_set x (_add $x 1)` but avoid the arena
churn of creating a new entry for loop counters.

| ID | Requirement |
| --- | --- |
| MA-01 | All math builtins MUST operate on INT values. Non-INT arguments MUST produce `XELP_E_ERR` (no silent coercion from STR). |
| MA-02 | All math builtins MUST push their result onto the result stack as INT for `()` composition. |
| MA-03 | `_add` and `_mul` MUST accept two or more arguments (variadic). `_sub`, `_div`, `_mod` MUST be binary. |
| MA-04 | `_div` and `_mod` with divisor zero MUST return `XELP_E_ERR`, not crash or invoke undefined behavior. |
| MA-05 | `_inc` and `_dec` MUST take a bare variable name (not `$`), mutate the variable in place, and push the new value onto the result stack. The variable MUST be INT; non-INT MUST produce `XELP_E_ERR`. |
| MA-06 | Symbol aliases (`+`, `-`, `*`, `/`, `%`, `&`, `\|`, `^`, `~`, `<<`, `>>`) are optional ergonomic aliases bound to the same handlers. They do NOT get a `_` prefix (per B-03). A compile flag MAY exclude symbol aliases for ROM-constrained builds. |
| MA-07 | Logical builtins (`_and`, `_or`, `_not`) MUST evaluate operands via the canonical truthiness mapping (TH-01). Results MUST be INT 0 or 1. |

#### Comparison

| Builtin | Alias | Types | Notes |
|---|---|---|---|
| `_eq` | `==` | INT, STR | Type-aware. Same type → compare values. Different types → false (no coercion). |
| `_neq` | `!=` | INT, STR | Inverse of `_eq`. Different types → true. |
| `_gt` | `>` | INT only | Non-INT → `XELP_E_ERR`. |
| `_lt` | `<` | INT only | Non-INT → `XELP_E_ERR`. |
| `_ge` | `>=` | INT only | Non-INT → `XELP_E_ERR`. |
| `_le` | `<=` | INT only | Non-INT → `XELP_E_ERR`. |

String `_eq` / `_neq` is byte comparison — same length + same bytes =
equal. No locale, no case folding.

| ID | Requirement |
| --- | --- |
| MA-08 | All comparison builtins MUST return INT 0 or 1 and push the result onto the result stack for `()` composition. |
| MA-09 | `_eq` and `_neq` MUST accept both INT and STR operands. Same-type comparison: INT uses numeric equality, STR uses byte-for-byte comparison. Cross-type (`_eq 3 "3"`) MUST return false — no coercion. |
| MA-10 | `_gt`, `_lt`, `_ge`, `_le` MUST accept INT operands only. Non-INT MUST produce `XELP_E_ERR`. |
| MA-11 | Comparison symbol aliases (`==`, `!=`, `>`, `<`, `>=`, `<=`) follow the same rules as math aliases (MA-06): optional, no `_` prefix, same handlers. |

### Multi-instance capability policy **(M‑\*)**

Answers "why two shells differ": security or business policy—not accidental forked languages.

| ID | Requirement |
| --- | --- |
| M-01 | Instances MAY vary transport, builtin availability, capability masks (e.g. disallow raw poke over BLE)—policy articulated per product deployment. |

### Observability and safety envelopes **(S‑\*)**

Prevents unattended scripts wedging MCU unless manufacturing unlock explicitly allows it.

The core mechanism is a single function pointer callback — `mpfBreakpoint`
— called at the top of the evaluator loop every time the tokenizer fires
to get the next statement. One hook point, one code location,
deterministic. The callback receives the full XELP struct and can
implement any combination of: step budgeting, break character detection,
trace logging, variable inspection, single-step debugging. If the
callback returns anything other than `XELP_S_OK`, the evaluator stops.

If `mpfBreakpoint` is NULL, no call is made — scripts run without limits.
The evaluator contains zero policy code. All safety, debugging, and
tracing behavior is the integrator's responsibility via the callback.

```c
/* in XELP struct */
XELPRESULT (*mpfBreakpoint)(XELP *ths);

/* evaluator loop */
while (XELP_S_OK == XelpTokLineXB(args, &line, XELP_TOK_LINE)) {
    if (ths->mpfBreakpoint) {
        XELPRESULT r = ths->mpfBreakpoint(ths);
        if (r != XELP_S_OK) return r;
    }
    /* dispatch statement */
}
```

Nothing is special about `_goto`, `_next`, or `:label` — they are
statements like any other. The tokenizer processes them, the callback
fires, the debugger decides what to do.

Example integrator callback:
```c
XELPRESULT myBreakpoint(XELP *ths) {
    if (--stepCount <= 0) return XELP_E_BUDGET;       /* budget */
    if (ths->mpfInReady && ths->mpfInReady()) {        /* break */
        if (ths->mpfIn() == 0x03) return XELP_E_BREAK;
    }
    traceLog(ths);          /* log statement to debug UART */
    inspectVars(ths);       /* dump arena / variable state */
    waitForKey(ths);        /* single-step: block on mpfIn */
    return XELP_S_OK;
}
```

| ID | Requirement |
| --- | --- |
| S-01 | The evaluator MUST call `mpfBreakpoint(ths)` at the top of the evaluator loop, before each statement, every time the tokenizer fires. If the callback returns anything other than `XELP_S_OK`, the evaluator MUST stop and return that status. |
| S-02 | If `mpfBreakpoint` is NULL, no call is made. Scripts run without limits. The evaluator contains zero policy code — no built-in budget, no built-in break detection. |
| S-03 | The `mpfBreakpoint` callback receives the full XELP struct. It MAY inspect variables, frame state, arena, registers, parse position, and input readiness. It MAY block (for single-step debugging). |
| S-04 | Step budgeting, break character detection, trace logging, and all other safety/debug behavior are integrator policy implemented in the callback. xelp provides the hook and example implementations. |
| S-05 | `XELP_BREAK_CHAR` SHOULD be provided as a compile-time define (default CTRL-C / 0x03) for use by integrator callbacks. |

**Footnote — cross-instance debugging:** because xelp supports multiple
independent instances and `mpfBreakpoint` receives the full XELP struct,
one xelp instance can single-step another. Instance A (debug console on
UART) runs C commands that inspect and control instance B (target on BLE).
B's `mpfBreakpoint` callback blocks and waits for commands from A. All
with existing multi-instance machinery — no extra debug protocol needed.

### Deferred specificity **(T‑\*)**

Token-level grammar annexes deliberately avoid bloating checklist prose.

| ID | Requirement |
| --- | --- |
| T-01 | Tokenizer tables, exhaustive reserved builtin rosters, precedence among builtin/script/C dispatch, @name details live in annex documents—not omitted from project, merely not duplicated here row-by-row. |

### Document precedence

Requirements here constrain exploratory drafts (`dev/xelp_script_proposal1.md`, `dev/xelp_script_proposal2.md`, `dev/xelp_script_proposal3.md`). If proposal text clashes with numbered IDs (`A‑01`, `C‑01`, …), revise either the proposal or this document deliberately—silent divergence is unacceptable for safety reviews.

---

## Open items to resolve

Gaps identified during design. Each needs discussion and either promotion
to numbered requirements or explicit deferral.

- [ ] **`_func` definition mechanics.** Partially settled. Name is
  `_func`. A func is a named string with execute permission — the
  evaluator doesn't care whether statements came from a func body or
  a ROM script. The frame gives it local scope, `@n` gives it arguments,
  `_return` gives it a result.
  **C-side registration:** array of `{name, body, help}` structs, same
  pattern as `mpCLIModeFuncs`. Bodies are `const char *` — ROM-able.
  Help text is a string in the struct (free, lives in ROM).
  ```c
  typedef struct {
      const char *mpCmd;   /* function name */
      const char *mpBody;  /* script text, ROM-able */
      const char *mpHelp;  /* help string, or NULL */
  } XELPScriptFuncEntry;
  ```
  C-registered bodies can use `\n` or `;` as statement separators
  (C compiler turns `\n` into 0x0A, tokenizer sees it as separator).
  **Dynamic definition (from script/CLI):** `_func` stores the body
  as a `XELP_VAL_PROC`-typed entry (reserved in type enum, VT-05) in
  the root frame. `;` is the practical statement delimiter — ENTER
  submits the CLI line so real newlines can't be embedded in a quoted
  string interactively.
  ```text
  _func square "_set r (* @1 @1) ; _return $r"
  ```
  **Help for dynamic procs:** not worth arena cost. If help is needed,
  register from C where the help string is free (ROM). Dynamic procs
  are throwaway/debug — help not expected.
  **Dispatch order settled (DA-07):** builtins → user script funcs →
  user C commands → default handler. User script funcs win over C
  commands, enabling overrides. Policy flag MAY gate this.
  **Redefinition rules settled:** dynamic PROC variables (defined via
  `_func` at CLI or in script) MAY be redefined or unset — they're
  entries in the root frame. C-registered script funcs and `_` builtins
  MUST NOT be modified or unset — they live in ROM/tables outside the
  arena. A dynamic `_func` with the same name as a C command will be
  found first in dispatch (per DA-07) effectively shadowing the C
  command, but the C registration is unchanged.
  **Scope settled:** `_func` ALWAYS writes a PROC-typed entry to the
  root frame's heap region, regardless of call depth. This is the one
  exception to "writes go to current frame." The root frame heap is
  the bottom of the arena — never reclaimed by frame pops — so funcs
  persist until `_unset` or instance reset. No separate global heap
  or arena partition needed. Variables remain frame-local (SC-01);
  funcs are instance-global via root frame targeting. Lookup during
  dispatch: linear scan of root heap for PROC entries. This is a
  dispatch step (DA-07), not a variable lookup.
  **Lookup strategy:** linear scan of root heap PROC entries. Left to
  implementation — fine for the handful of funcs embedded scripts
  define.

- [x] **Step budgeting + interrupt + trace mechanism (S-01 through S-05).**
  Settled. Single `mpfBreakpoint` callback called every time the tokenizer
  fires. NULL = no limits, no overhead. Callback receives full XELP struct,
  integrator implements all policy (budget, break, trace, single-step).
  Zero policy code in the evaluator.

- [x] **`_set` type inference and `$` sigil semantics.** Resolved: `$` is
  expand-only (Tcl/bash model, SG-01 through SG-04). Type inference uses
  quotes as type annotation (TI-01 through TI-06). Unquoted numeric →
  INT, quoted → STR, `$` expansion preserves source type. `0xFF` unquoted
  → INT(255). `3.14` without `XELP_ENABLE_FLOAT` → STR. Two-path type
  handling: builtins see raw tokens with types; C handlers get stripped
  argv strings only (TI-04).

- [x] **Math builtins roster (MA-01 through MA-07).** Settled.
  Arithmetic: `_add`(+), `_sub`(-), `_mul`(*), `_div`(/), `_mod`(%).
  Logical: `_and`, `_or`, `_not`. Bitwise: `_band`(&), `_bor`(|),
  `_bxor`(^), `_bnot`(~), `_shl`(<<), `_shr`(>>). In-place: `_inc`,
  `_dec`. `_add`/`_mul` variadic, others binary or unary. All push INT
  result. `_inc`/`_dec` take bare names, mutate in place, push new value.
  Symbol aliases optional, no `_` prefix. Comparisons still open (item 5).

- [x] **Comparison and equality semantics (MA-08 through MA-11).**
  Settled. `_eq`/`_neq` work for INT and STR. Cross-type → false, no
  coercion. `_gt`/`_lt`/`_ge`/`_le` INT-only, non-INT → error. String
  comparison is byte-for-byte. All return INT 0 or 1.

- [x] **`_unset` / `_clear` → no `_unset` (VL-01 through VL-06).**
  Settled. Variables are C-like: type fixed at first `_set`, live until
  frame pops (or instance reset for root). No `_unset`. INT overwrites
  in place. STR same-or-shorter overwrites in place. STR longer →
  memmove current-frame heap entries to make room. No arena
  fragmentation, no tombstones.

- [x] **Builtin registration mechanism.** Resolved: two-path dispatch
  (DA-01 through DA-07). `_xxx` tokens route to a single internal
  dispatch function with full evaluator access. User commands go through
  the existing handler table. Detection is `token[0] == '_'`.

- [ ] **C→xelp calling convention (`XelpCallProc`).** Not settled —
  needs external review. C can already run a script via
  `XelpParse(ths, "...")`. The gap is narrow: frame scoping and result
  capture. Proposed shape:
  `XelpCallProc(ths, "procname arg1 arg2", &result)` — same string
  format as the CLI, tokenized by the existing tokenizer. Essentially
  `XelpParse` but pushes a frame (local scoping, `@n` access) and
  captures the return value. Two return paths under consideration:
  **(1) `mR[0]`** for the quick integer case (90% of use, already
  works today, zero ceremony). **(2) `XelpResult` typed struct** for
  when C needs to inspect what came back (INT, STR, NIL):
  `XelpResult { kind, intVal, strVal, strLen }`. String pointer points
  into arena, valid until next xelp API call on that instance — caller
  copies if needed (standard C convention). `NULL` result pointer =
  fire-and-forget. Provides symmetry with `XelpSetResultInt`/
  `XelpSetResultStr` (C handler → result stack) and `_return` (xelp →
  result stack). Concerns: string lifetime tied to arena stability is
  fragile if caller isn't careful. Interaction with `mR[]` — does
  `_return` also write to `mR[0]`, or are they separate channels?
  Needs review for edge cases.

- [ ] **Float proposal sign-off.** Hex vs decimal expansion switch,
  `XelpArgvFloat` API, `XELP_ENABLE_FLOAT` gating. Currently
  exploratory — needs promotion or deferral.

- [ ] **Script input (`inkey`, `input`).** Exploratory. Both should be
  **C handlers** (no `_` prefix), not builtins — input is fundamentally
  platform-specific and blocking behavior is the integrator's problem.
  xelp ships example implementations; integrator copies and adapts.
  Script captures values via `()` composition: `_set k (inkey "prompt")`.
  Requires `XelpSetResultInt`/`XelpSetResultStr` public API for C
  handlers to push typed values onto the result stack.
  **`inkey "prompt"`** — MVP input primitive. C handler prints prompt
  via `mpfOut`, spins on `mpfInReady`/`mpfIn` (integrator controls the
  spin loop — yield, service DMA, pet watchdog, etc.), returns keycode
  as INT via `XelpSetResultInt`. ~20-30 bytes of user code. Capability
  policy: don't register `inkey` on restricted instances.
  **`input "prompt" [esc_key]`** — line input. Lower priority, not MVP,
  but not deferred. C handler with line buffer, backspace/echo, ENTER
  detection. Returns STR via `XelpSetResultStr`. Optional escape key
  argument. Buffer is on the C stack or integrator's static allocation —
  not xelp's concern. Building `input` from `inkey` in script is
  theoretically possible but practically ugly — backspace requires
  `\b \b` console output, cursor tracking, prompt-boundary protection.
  That plumbing belongs in C.

- [x] **Debug / trace / single-step system.** Subsumed by `mpfBreakpoint`
  callback (S-01 through S-05). The callback receives the full XELP
  struct and can implement trace logging, variable inspection, and
  single-step debugging — all as integrator-provided policy, zero
  interpreter code. Trace output SHOULD go to `mpfErr` or a separate
  debug output function pointer so it doesn't pollute user-facing
  output. xelp MAY ship example callbacks in the examples directory
  (trace logger, single-step debugger). Potentially unique among
  sub-10 KB interpreters.

---

## Float passing — proposal for discussion

**Status: exploratory — not normative. Requires sign-off before promotion
to numbered requirements.**

### The problem

When a float variable is expanded into argv (`$x` where x is F32), it must
become a string token that the C handler parses back. The expansion path
must produce a string, and the handler must recover the original value.

Decimal float-to-string (`ftoa`/`dtoa`) is problematic:

* **Code size:** A correct `dtoa` is 500+ bytes. Even a minimal fixed-
  precision version is 200+. Significant on an 8 KB budget.
* **Precision loss:** `0.1` has no exact binary representation. Decimal
  round-trip: `0.1` → F32 → `"0.100000001"` → parse → slightly different
  value. For control loops, calibration, and sensor thresholds this matters.
* **Inconsistent output:** How many digits? 6? 9? Configurable? Fixed?
  Each choice trades precision against string length.
* **Parsing cost:** `strtof`/`atof` may not exist on bare-metal targets.
  xelp avoids stdlib.

### Proposed approach: hex-float format for expansion

Use IEEE 754 hex encoding for the `$var` → argv string path. The float's
bit pattern is written as a hex integer with a type prefix.

**Expansion:**

```text
_set x 3.14                          # parse decimal → store as F32
motor $x                             # expand $x → "0f4048F5C3"
                                     #   "0f" prefix = F32, then 8 hex digits
```

**Format:**

```text
F32:  "0f" + 8 hex digits    (4 bytes = 8 nibbles)     = 10 chars
F64:  "0d" + 16 hex digits   (8 bytes = 16 nibbles)    = 18 chars
F16:  "0h" + 4 hex digits    (2 bytes = 4 nibbles)     = 6 chars
```

The prefix disambiguates from integer hex (`0x1234`) and tells the parser
which float width to reconstruct.

**Why hex-float:**

* **Lossless round-trip.** The bit pattern is preserved exactly. No
  precision loss, no decimal approximation. `0.1` → `0f3DCCCCCD` →
  parse → identical F32 bit pattern.
* **Tiny formatter.** Writing 4/8 bytes as hex nibbles is ~20-30 bytes of
  code. No division, no digit counting, no rounding. The int-to-hex code
  is a tight nibble loop.
* **Tiny parser.** Reading hex nibbles back into a 4/8 byte value is
  equally small. `XelpParseNum` already handles `0x` hex — extending it
  to recognize `0f`/`0d`/`0h` prefixes is a small addition.
* **Fixed width.** F32 is always 10 chars. No variable-length output, no
  precision decisions. Predictable argv buffer consumption.
* **No stdlib.** No `strtof`, no `atof`, no `snprintf`. Pure byte
  manipulation.

**User-facing decimal remains for literals and display:**

```text
_set x 3.14                   # user types decimal — parsed to F32
_print $x "\n"                # prints whatever expansion mode produces
motor $x                      # argv gets expanded float string
```

Users always write decimal literals. The expansion format is an internal
concern controlled by a compile-time switch.

### Expansion mode: compile-time switch

The integrator chooses how float variables expand into argv strings:

**`XELP_FLOAT_EXPAND_HEX` (default) — lossless, tiny code:**

```text
_set x 3.14
motor $x                      # handler receives argv[1] = "0f4048F5C3"
```

Hex-encoded IEEE 754 bit pattern. Lossless round-trip, ~20-30 bytes for
the formatter, fixed-width output. The handler uses `XelpArgvFloat` to
decode. Opaque to humans — handler authors need a doc note about the
format and the helper function.

**`XELP_FLOAT_EXPAND_DEC` — human-readable, larger code:**

```text
_set x 3.14
motor $x                      # handler receives argv[1] = "3.14"
```

Decimal string. Familiar to C handler authors — looks like what they'd
type at the CLI. Handler can use `atof`, `strtof`, or `XelpArgvFloat`
(which accepts both formats). Costs ~200-400 bytes for the ftoa formatter.
Subject to binary float precision artifacts (`0.1` may expand as
`"0.100000001"`).

**Trade-off summary:**

| | Hex (default) | Decimal (opt-in) |
|---|---|---|
| Formatter code | ~20-30 bytes | ~200-400 bytes |
| Round-trip | Lossless | Precision artifacts |
| Output width | Fixed (10 chars F32) | Variable (1-15+ chars) |
| Handler readability | Opaque — needs `XelpArgvFloat` | Familiar — looks like a number |
| stdlib dependency | None | None (custom ftoa) |

The expansion mode only affects the `$var` → argv path. Everything else
(arena storage, type tags, expansion dispatch, frame layout) is identical
regardless of the switch.

**`_print` output:** `_print` prints argv strings as-is — it does not
reformat. If the expansion mode is hex, `_print $x` prints
`0f4048F5C3`. If the expansion mode is decimal, `_print $x` prints
`3.14`. The expansion mode controls what all commands see, including
`_print`. If human-readable float output matters, use
`XELP_FLOAT_EXPAND_DEC`.

**C handler side:**

```c
XELPRESULT cmd_motor(XELP *ths, int argc, const char **argv) {
    float gain;
    if (argc > 1) XelpArgvFloat(argv, argc, 1, &gain);  /* accepts both formats */
    set_motor_gain(gain);
    return XELP_S_OK;
}
```

`XelpArgvFloat` checks for the `0f`/`0d`/`0h` prefix first (hex decode),
then falls back to decimal parse. Handlers work correctly regardless of
the expansion mode — and also when the user types `motor 3.14` directly
at the CLI (no variable expansion, plain decimal literal).

**Arena storage:**

Float variables in the heap use the same byte-stream encoding as integers:

```text
F32:  [ 0x0A | nameHash_2B | value_4B ]     = 7 bytes  (same as INT)
F64:  [ 0x0B | nameHash_2B | value_8B ]     = 11 bytes
F16:  [ 0x09 | nameHash_2B | value_2B ]     = 5 bytes
```

No structural change to the arena. The kind byte distinguishes float from
int. The expansion switch (EX-08) adds a case for each float kind.

**What needs to exist for float support:**

| Component | Size estimate | Gated by |
|---|---|---|
| Decimal → F32 parser (`_set x 3.14`) | ~150-250 bytes | `XELP_ENABLE_FLOAT` |
| F32 → hex-string formatter | ~20-30 bytes | `XELP_ENABLE_FLOAT` |
| Hex-string → F32 parser (`XelpArgvFloat`) | ~30-40 bytes | `XELP_ENABLE_FLOAT` |
| F32 → decimal formatter (for `_print` + opt-in expansion) | ~200-400 bytes | `XELP_ENABLE_FLOAT` + `XELP_FLOAT_EXPAND_DEC` |
| Expansion switch case | ~10 bytes | `XELP_ENABLE_FLOAT` |

Minimum float support (hex expansion): ~210-330 bytes.
With decimal expansion: ~410-730 bytes.
Without `XELP_ENABLE_FLOAT`: zero cost — compile flag gates everything.

**Open questions:**

1. Is F16 worth supporting early? DSP-adjacent embedded work uses it, but
   hardware F16 support varies. Reserve the slot, implement later.
2. Should `XelpArgvFloat` live in xelp core or in a companion header
   (`xelp_float.h`) that integrators include only if needed?

---

## Possible tiers (exploratory—not committed)

A **staging** idea under discussion—not normative unless promoted into numbered **`ID`** rows later.

* **Subset chain (concept):** **`Xelp CLI`** ⊆ **Tier 1** ⊆ **Tier 2**, each step additive so scripts written for an earlier tier keep working when a later tier ships.

* **Tier 1 (inspector / bring-up posture, sketch)** — Same flat **`command arg …`** shape as today's CLI (**no parentheses**, **no `$` variables**, minimal or no flow control). Optionally route tokens whose name starts with **`_`** through a small **reserved builtin table** (**`_peek`**, **`_poke`**, **`_print`**, integer math/bit helpers, …) before the ordinary user **`mpCLIModeFuncs`** search—thin glue on **`argc`/`argv`**, not a second interpreter.

* **Tier 2 (full script posture, sketch)** — Adds **`( … )` nested value calls**, **`$` and `@` addressing**, procedural control (**`_if`**, **`_goto`**, labels when specified), scripted procedures where designed, fuller truthiness, and bounded script arena state—a larger lexer/eval/memory story than a **`_*` builtin table** on **`argc`/`argv`** alone.

Tier boundaries might align with **`xelpcfg.h`** (**`XELP_*`** knobs) plus grammar annexes—but this section is **exploratory**. No **`MUST`** here obligates Tier 1, Tier 2, or both until promoted into numbered requirements above.

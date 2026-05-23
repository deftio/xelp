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
TODO: put deeper xelp background here (what it is its 15 years old, its 3KB parser, cli, key, thru, rommable scripts etc.)

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
$name     named variable lookup
@1        first positional argument of innermost scripted callable
@2        second positional argument
@name     named argument, if supported
@#        argument count (if supported)
foo       verb in command position
(foo ...) nested value-producing call
```

Three domains:

* **Variables via `$`** - `$temperature` resolves in the active variable scope for this instance/script artifact.
* **Parameters via `@`** - `@1` is always positional parameter one; avoids bash confusion where `$1` means something else entirely.
* **Invocation by name** - `motor 50` is a statement. `(motor 50)` participates as a nested value expression only where grammar allows.

## Bare word rule

Behavior should be predictable for first-time readers:

In command position, the first word is the verb (`foo a b` — `foo` is looked up).

In argument position, an unquoted word is a literal, not automatic variable dereference (`print hello` sends the token hello to the print function).

* To interpolate a variable, use **`$`** (`print $hello`).
* To call a command for a value slot, parenthesize (`print (hello)`).
* To refer to positional parameters inside a scripted callable, use **`@`** (`print @1`).

Recommended decoding after tokenization:

```text
digits / numeric literals     -> INT (same parsing spirit as CLI)
"quoted …"                     -> STR
bare word                     -> literal (SYM / STR convention per profile)
$name                         -> typed variable value
@1 / @name                    -> parameter value
(... )                        -> typed return from nested call
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

### Multi-instance capability policy **(M‑\*)**

Answers “why two shells differ”: security or business policy—not accidental forked languages.

| ID | Requirement |
| --- | --- |
| M-01 | Instances MAY vary transport, builtin availability, capability masks (e.g. disallow raw poke over BLE)—policy articulated per product deployment. |

### Observability and safety envelopes **(S‑\*)**

Prevents unattended scripts wedging MCU unless manufacturing unlock explicitly allows it.

| ID | Requirement |
| --- | --- |
| S-01 | SHOULD ship statement/step budgeting default enabled; MAY document explicit unlimited mode for jig-only builds. |

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

- [ ] **`_proc` definition mechanics.** How are procs registered, stored,
  and looked up? ROM-resident vs runtime-defined? Can procs be defined
  at the CLI (multi-line input)? What's the storage format — name →
  XelpBuf pointing into script source? Registration in a table or linear
  scan of script text?

- [ ] **Step budgeting details (S-01).** `_goto` requires this but it's
  not fleshed out. Where does the counter live — XELP struct field?
  Default limit value? How to override (`XELP_STEP_BUDGET` compile
  flag)? What happens on budget exhaustion — error + stop? Resume
  mechanism?

- [ ] **Safe execution / interrupt mechanism.** Design center mentions
  ESC-ESC or CTRL-C to break runaway scripts. How does the interpreter
  check for break between statements? Poll `mpfIn`? Separate callback?
  Key sequence detection while script is running?

- [ ] **`_set` type inference.** `_set x 3` → INT, `_set x "hello"` →
  STR. What about `_set x 3.14` when `XELP_ENABLE_FLOAT` is disabled?
  Error? Store as STR? What about `_set x 0xFF` — INT (parsed as hex)
  or STR?

- [ ] **Math builtins roster.** `_add`, `_mul`, `_sub`, `_div`, `_mod`,
  `_gt`, `_lt`, `_eq`, `_neq`, `_ge`, `_le`, `_and`, `_or`, `_not`,
  `_band`, `_bor`, `_bxor`, `_bnot`, `_shl`, `_shr`. Which are MVP?
  Arity (binary only, or variadic for `_add`/`_mul`)? Symbol aliases
  (`+`, `*`, `>`, etc.) — which ship?

- [ ] **Comparison and equality semantics.** `_eq` across types — is
  `_eq 3 "3"` true? Does it coerce? Or strict type match required?
  Ties into truthiness (TH-05).

- [ ] **`_unset` / `_clear`.** Root frame variables accumulate at the CLI.
  Need a way to release them? Or just document that instance reset is
  the cleanup mechanism?

- [ ] **Builtin registration mechanism.** Script builtins (`_set`, `_if`,
  `_print`, etc.) — are they a separate function table from
  `mpCLIModeFuncs`? Searched first (reserved namespace priority)? Or
  interleaved? How does the dispatch order work: builtins → user
  commands → default handler?

- [ ] **`_proc` calling convention.** How does `XelpCallProc` (C calling
  script) work? It needs to push a frame, set up argv from C-provided
  strings, run the proc body, and return the result. What's the API
  shape?

- [ ] **Float proposal sign-off.** Hex vs decimal expansion switch,
  `XelpArgvFloat` API, `XELP_ENABLE_FLOAT` gating. Currently
  exploratory — needs promotion or deferral.

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

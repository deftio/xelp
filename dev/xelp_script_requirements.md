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
_if (> $x 5) high
```

This keeps the lexical layer small:

* No infix algebraic grammar—no precedence table for `1 + 2 * 3`.
* No AST interpreter for generalized expressions.

Math and predicates use Lisp-style prefixes: **`+`** and **`>`** act as ordinary command names, not lexer magic.

```text
_set x (* (+ $foo 4) 3)
_if (_eq $x 20) big
```

```text
(+ 1 2 3)
```

means “invoke command `+` with arguments `1`, `2`, and `3`.”

Invalid as core math syntax (heavy work belongs in C):

```text
1 + 2 * 3
```

## Invocation model

Exactly two outward invocation shapes matter to script authors:

```text
foo arg arg           # statement: run foo for side effects / registers / outcome
(foo arg arg)         # nested value context: foo must yield one typed value for the caller
```

The parenthesized form evaluates its inner command first and substitutes the returned typed value as a single argument to the enclosing command.

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

In argument position, an unquoted word is a literal, not automatic variable dereference (`print hello` prints the token `hello`).

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
| I-02 | (foo …) MUST only appear where grammar permits value slots; nesting MUST contribute exactly one typed value or sanctioned ERR sentinel. |
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
| R-07 | The result stack MUST be fixed-size and compile-time bounded (`XELP_RESULT_STACK_SZ`). Overflow MUST be a hard error. Frame exit MUST pop results back to the frame entry point. |
| R-08 | `_mr <index>` MUST be an ordinary builtin that follows the two-channel return contract. Read: pushes `mR[index]` onto result stack. Write: sets `mR[index]` and pushes the written value. Out-of-bounds index MUST error. |
| R-09 | `_mr` is not special. It is a builtin function with the same return contract, result stack behavior, and composition rules as every other builtin. |

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

## Possible tiers (exploratory—not committed)

A **staging** idea under discussion—not normative unless promoted into numbered **`ID`** rows later.

* **Subset chain (concept):** **`Xelp CLI`** ⊆ **Tier 1** ⊆ **Tier 2**, each step additive so scripts written for an earlier tier keep working when a later tier ships.

* **Tier 1 (inspector / bring-up posture, sketch)** — Same flat **`command arg …`** shape as today's CLI (**no parentheses**, **no `$` variables**, minimal or no flow control). Optionally route tokens whose name starts with **`_`** through a small **reserved builtin table** (**`_peek`**, **`_poke`**, **`_echo`**, integer math/bit helpers, …) before the ordinary user **`mpCLIModeFuncs`** search—thin glue on **`argc`/`argv`**, not a second interpreter.

* **Tier 2 (full script posture, sketch)** — Adds **`( … )` nested value calls**, **`$` and `@` addressing**, procedural control (**`_if`**, **`_goto`**, labels when specified), scripted procedures where designed, fuller truthiness, and bounded script arena state—a larger lexer/eval/memory story than a **`_*` builtin table** on **`argc`/`argv`** alone.

Tier boundaries might align with **`xelpcfg.h`** (**`XELP_*`** knobs) plus grammar annexes—but this section is **exploratory**. No **`MUST`** here obligates Tier 1, Tier 2, or both until promoted into numbered requirements above.

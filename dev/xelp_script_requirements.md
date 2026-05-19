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

### Return paths vs **`XelpOut`** **(R‑\*)**

Separates chatter on the UART from machine-readable compose semantics—critical once BLE hosts automation.

| ID | Requirement |
| --- | --- |
| R-01 | Every C handler invoked via script pathways MUST expose deterministic success/warn/error lineage comparable in spirit to XELPRESULT/mR[0]—exact ABI left to implementation docs. |
| R-02 | ( … ) MUST deliver exactly one sanctioned typed return or deterministic ERR/NIL—not garbage across nested evaluations. |
| R-03 | XelpOut MUST NOT silently become the scripted return channel unless a documenting command declares that contract. |
| R-04 | MAY keep narrow mailbox registers alongside typed cells if semantics stay reconciled for script callers. |

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
| F-01 | MUST ship set, if, goto, labeled lines scoped inside one artifact; duplicate labels must error cleanly. |
| F-02 | Structured loops remain optional later—they MUST NOT gate MVP readiness (paired with Anti-goal A‑03). |
| F-03 | Truth tables above plus mandated TR fixtures gate if; goto must not accidentally span unrelated blobs. |

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

# XELP Script Requirements — Architectural Feedback

**Reviewer posture:** senior embedded-systems architect  
**Date:** 2026-05-24  
**Inputs reviewed:**

- `README.md` (xelp v0.4.0 positioning, size claims, scripting story)
- `src/xelp.c`, `src/xelp.h`, `src/xelpcfg.h`, `src/XelpArduino.h` (shipped implementation)
- `dev/xelp_script_requirements.md` (normative requirements draft)
- Cross-checks against `dev/xelp_script_proposal3.md`, `dev/xelp_script_impl_v1.md`, `dev/xelp_truthiness_trap_catalog.md`

This document is **review feedback**, not a revised spec. It is written to support a design review conversation before implementation begins.

---

## 1. Executive summary

The requirements document is ** unusually mature for an embedded scripting layer**. It shows real restraint (no Lua creep), honest anti-goals, traceable IDs, and several design choices that respect xelp’s existing identity: command-shaped surface, ROM-safe parsing, no heap, multi-instance, C-as-native-vocabulary.

That said, the spec is **not yet implementation-ready** as a single coherent architecture. It contains **internal contradictions** (result storage model, `mR[0]` semantics, procedure naming), **drift from the shipped v0.4.0 codebase** (platform hooks, buffer limits, dispatch path), and **optimistic size/RAM assumptions** relative to README’s “under 3–5 KB fully featured” story.

**Recommendation:** treat the requirements as a strong **design north star**, but run a **narrowing pass** before coding:

1. Reconcile **one** memory model (arena byte-stream vs separate result-stack array).
2. Reconcile **one** status/return contract for `mR[0]`, result stack, and `_if` truthiness.
3. Pick **one** procedure keyword (`_func` vs `_proc`) and one C-call API shape (`XelpCallProc` + `XelpGetResult` vs status-only).
4. Define an **MVP cut** that ships value without `_goto` loops *or* explicitly accepts that MVP is “sequencing + forward branch only.”
5. Add a **script profile** section to `xelpcfg.h` with explicit RAM/code budget numbers—not just feature flags.

If those five items are resolved, the architecture is defensible and buildable within xelp’s ethos. Without them, implementers will fork the design in the first sprint.

---

## 2. What xelp is today (baseline the spec must respect)

v0.4.0 is a **flat command dispatcher**, not a language runtime:

| Property | Shipped reality |
|----------|-----------------|
| Script execution | `XelpParse` / `XelpParseXB`: sequential lines, linear command-table match, `_xelpBuf2Argv` → `(ths, argc, argv)` |
| Return path | `mR[0]` = last command status; `mR[1..3]` = handler-specific ints (engine does not touch 1–3) |
| Tokenizer | Table-driven PSM (`gPSMStates` / `XelpTokLineXB`); ROM-safe read-only source |
| Per-instance RAM (CLI full) | `mCmdMsgBuf[XELP_CMDBUFSZ]` + `mArgvBuf[XELP_ARGVBUFSZ]` + optional history |
| Platform hooks | `mpfOut`, `mpfErr`, `mpfEditModeChg`, `mpfBksp`, `mpfPassThru` — **no** `mpfIn`, `mpfInReady`, `mpfGetTicks`, `mpfDelay` |
| argv limits | `XELP_ARGV_MAX` defaults to **8** in `xelpcfg.h` (AGENTS.md says 16 — doc drift) |
| Label search | `XelpFindTok` exists with `XELP_TOK_LINE` “first token on line” semantics — **directly reusable** for `:label` jumps |

The README’s core promise—“scripts are ROM-able const strings, parser never mutates input”—is **true today** and must remain non-negotiable. XELP Script should extend dispatch, not replace the PSM.

---

## 3. Architectural strengths (keep and protect)

### 3.1 Command-first orchestration, not a second language

The design center (C fast path, script as glue) is correct. Requiring prefix builtins (`_set`, `_if`) and optional symbol **aliases as command names**—not infix—is the right compromise for embedded size and teachability.

**Why this matters:** every successful embedded DSL dies when it tries to be “80% of Python.” This spec mostly avoids that.

### 3.2 Two-path dispatch (`_` vs user)

`token[0] == '_'` → internal builtin evaluator; else → existing command table is **cheap, unambiguous, and ROM-friendly**. It also preserves the integrator’s ability to keep firmware verbs short (`led`, `motor`) without reserving entire punctuation namespaces unless a profile opts in.

### 3.3 Iterative evaluation with continuations (PE-04, AR-*)

Choosing arena-resident CONT/FRAME records over C recursion is **architecturally correct** for:

- bounded stack on MCUs,
- re-entrancy (`XelpCallProc` → C handler → `XelpCallProc`),
- future “run from ISR-adjacent contexts” claims (provided handlers cooperate).

This aligns with how xelp already avoids deep C call chains in the tokenizer.

### 3.4 Policy-free safety hook (`mpfBreakpoint`, S-*)

Pushing step budget, break characters, trace, and single-step **out of the interpreter** and into an integrator callback is the best decision in the safety section. It matches xelp’s pattern: tiny core, behavior via function pointers.

The cross-instance debugging footnote (instance A steps instance B) is a **genuine differentiator** if documented with an example—rare in sub-10 KB runtimes.

### 3.5 Gradual C handler migration

`XelpSetResultInt` / `XelpSetResultStr` as opt-in, with NIL default for legacy handlers, is the right migration story. It avoids a flag day breaking every example and every integrator.

### 3.6 Truthiness as a first-class test contract (TH-*, TR-*)

Referencing `dev/xelp_truthiness_trap_catalog.md` and mandating a frozen `_truthy` table is **professional-grade** requirements work. Coercion bugs are where script layers lose trust in factory and field tools.

### 3.7 `$` expand-only sigil (SG-*)

One rule—“`$` means expand, bare word means literal”—is teachable and matches Tcl/bash mental models without PHP’s ambiguous bare variables. Indirect assignment (`_set $ptr 42`) falling out naturally is elegant **if** `_set` is implemented with raw-token access on the builtin path only (TI-03).

### 3.8 Reuse of `XelpFindTok` for labels

The label definition rule (first token on line) matches **`XelpFindTok(..., XELP_TOK_LINE)`** already in tree. Forward `_next :label` can ship without a symbol table. That is aligned with “no pre-pass, no registry” and should be called out explicitly as intentional reuse.

### 3.9 `_next` as multi-line block disable (not just control flow)

The spec’s `:_end` block-skip idiom (F-17, lines ~1306–1347 in the requirements doc) is easy to under-read as a minor convenience. It is not. **`_next :_end` is the multi-line comment story** for ROM-resident factory scripts—and that makes `_next` load-bearing for ergonomics, not optional sugar.

Today’s tokenizer only has **`#` to end-of-line** (PSM `_PS_CMNT`). There is no `/* */`, no heredoc, no backslash continuation. On a UART calibration script twenty lines long, “comment out this whole block” cannot mean “add `#` to every line” without ugly diffs and merge pain.

The block-skip pattern:

```text
_next :_end
  calibrate_adc 0
  calibrate_adc 1
  set_gain 12
:_end
run_main
```

…gives **forward skip over a region** without touching the skipped lines. Re-enable by `#_next :_end` (or delete one line); leave `:_end` in place—it is a no-op when nothing jumps to it. That matches how field engineers actually work: toggle whole bring-up sections, not re-tokenize the file.

**Why this design is clever (and fragile if implemented wrong):**

| Property | Implication |
|----------|-------------|
| **Forward-only `_next`** | Nearest `:_end` ahead = deterministic block boundary. Multiple blocks in one script work without a symbol table. |
| **Repeatable `:_end` (F-17)** | Duplicate labels are *forbidden* for `_goto` semantics but *required* for this idiom. The two-keyword split exists partly to preserve this. |
| **Runtime skip, not `#` parse skip** | Skipped lines must **not execute** (no motor pulses, no ADC writes during the scan). Jump sets parse position; intermediate statements never dispatch. |
| **`#` + `_next :_end` compose** | Line comments for notes; block skip for disabling executable regions. One mental model, two mechanisms (L-03 alignment). |

**Architectural opinion:** treat `_next :_end` as a **first-class authoring feature** in docs and tests, on par with `_if`. It is arguably more valuable than `_goto` for factory/bring-up scripts where you disable whole calibration ladders without loops. Underselling it as “control flow helper” misses why forward-only jump exists at all.

**Stink if wrong:** skipped block still runs side effects; `_next :_end` inside a nested proc skips to the wrong frame’s `:_end`; `#_next :_end` re-enable path breaks because `#` handling eats the wrong span; step budget counts skipped lines as executed statements (should not, if skip is a single jump op).

---

## 4. Critical concerns (must resolve before implementation)

### 4.1 Contradictory result storage model (R-* vs AR-*)

**Problem:** Section R-* describes a **separate fixed array** (`XELP_RESULT_STACK_SZ`, array of `XelpVal` structs). Section AR-* describes a **unified arena byte-stream** where the stack grows upward from SP and variables downward from HP, with variable-length tagged records.

These are not the same design. An implementer cannot satisfy both literally.

**Recommendation:** **Pick the arena byte-stream (AR-*) as canonical** and revise R-02/R-07 to say “result entries live in the arena stack region,” not a parallel `XelpVal[]`. The earlier `xelp_script_impl_v1.md` sketch (`stack[]` + `arena[]`) is a third variant—retire it or mark it superseded.

**Justification:** one buffer, one overflow check, one SP/HP invariant is simpler to audit on 8-bit targets than two independently bounded structures.

---

### 4.2 `mR[0]` has three incompatible jobs

The spec simultaneously says:

| Source | Claim |
|--------|-------|
| F-05 | Command-as-condition in `_if`: truthy when `mR[0] == XELP_S_OK` |
| R-10 | Script engine MUST NOT use `mR[]` for its own return values; status for script functions lives on result stack |
| CC-04 (open) | `_return <int>` SHOULD also write `mR[0]` for backward compatibility |
| Today’s xelp | `mR[0]` **is** the command status register for **all** dispatch |

**Problem:** If `_return` writes `mR[0]`, script procs **do** use `mR[0]`. If `_if (> $x 5)` evaluates a predicate builtin that pushes INT 0/1 to the result stack but leaves `mR[0]` as OK, command-truthiness and predicate-truthiness **diverge** unless `_if` has two distinct condition evaluators.

**Recommendation:** adopt a **single published contract**:

1. **`mR[0]` remains “last dispatched command status”** for C handlers and statement-level dispatch (preserve v0.4.0).
2. **Value composition uses the arena result stack only** (`XelpSetResult*`, `_return`).
3. **`_if` conditions are typed into exactly two forms:**
   - **Variable/predicate form:** one `$var` or one parenthesized predicate `(_gt $x 5)` → truthiness from `_truthy` / comparison result on value channel.
   - **Command form:** `_if cmd args _then ...` → truthiness from **`mR[0]` after cmd** (status channel only).

Revise R-10 to remove “script functions live status on result stack.” Status is always `XELPRESULT` return + mirror in `mR[0]` for statement commands; **values** are stack-only.

**CC-04:** writing `_return` int to `mR[1]` (not `mR[0]`) may be cleaner—`mR[0]` stays execution status, `mR[1]` becomes “last script int return” for C callers who refuse `XelpGetResult`. Document one blessed convention; do not overload `mR[0]` further.

---

### 4.3 `_if` sub-dispatch via `XelpParse` is underspecified and risky

F-06 says `_if` locates `_then`/`_else` slices and dispatches sub-commands “via `XelpParse`.”

**Problems:**

1. **Re-entrancy:** `XelpParse` runs a full line loop. If `_if` is evaluated **inside** the script evaluator (nested in a proc or during `XelpParse` of a ROM script), calling `XelpParse` again may reset parse cursor state, clobber `mArgvBuf`, or recurse into the script engine unless guarded.
2. **Interactive CLI:** `_if` at the prompt should not re-enter `XelpParseKey` state machines.
3. **Partial lines:** sub-clauses like `motor $gain` are not full scripts; they still need the **script evaluator** (variable expansion), not plain `XelpParseXB`.

**Recommendation:** specify **`XelpEvalLine(ths, const char *s, int len)`** (internal or public)—one statement, script-aware, no outer loop— and require `_if`/`_next` command forms to call **that**, not raw `XelpParse`. Reserve `XelpParse` for C→script entry at top level only.

This is not a nit; it is the difference between a composable control-flow builtin and a latent re-entrancy bug.

---

### 4.4 Procedure naming: `_proc` vs `_func`

Examples and return-model section use `_proc`. Open items settle on `_func`. **Sixteen occurrences of each**—implementers will grep the wrong keyword.

**Recommendation:** **`_func` only** in normative text (matches “function not coroutine” and C `XelpCallProc`). Global search-replace in the requirements doc before any code lands.

---

### 4.5 Spec describes platform hooks that do not exist

Background section (lines ~79–85) lists five HAL hooks including `mpfIn`, `mpfInReady`, `mpfGetTicks`, `mpfDelay`. **None are in `XELP` today.**

Safety examples (`myBreakpoint` polling `mpfIn`) and future `inkey`/`input` handlers depend on them.

**Recommendation:**

- Either **add these as optional struct members** behind `XELP_ENABLE_SCRIPT` (or a dedicated `XELP_ENABLE_HAL_INPUT`), with NULL = unsupported;
- Or **revise the spec** to say break/input/timing are **integrator callbacks supplied only to `mpfBreakpoint`**, not xelp core fields.

Do not leave phantom APIs in normative background—they propagate into AGENTS.md and AI-generated ports.

---

### 4.6 `_next`-only phase is weaker for *loops*, not for *bring-up scripts*

Anti-goal A-03 (no mandated `while`) is fine. **`_goto` is still required for loops** and must ship in the DV build with step budgeting (F-13, S-01).

But framing “`_next` without `_goto`” as broadly crippled **understates `_next`’s other job**: **multi-line block disable** via `_next :_end` (see §3.9). For factory calibration, scripted bring-up, and field diagnostics, the common operation is not “loop forever”—it is **“run this whole section or don’t.”** That is exactly what forward block skip provides, without `#` on every line.

| Need | `#` alone | `_next :_end` | `_goto` |
|------|-----------|---------------|---------|
| Line note | yes | — | — |
| Disable 10-line executable block | painful | yes | overkill / wrong tool |
| Conditional forward branch | — | yes (`_if … _then _next :L`) | yes |
| Backward loop | — | no | yes |

**Recommendation for docs and tests:** teach **`#` + `_next :_end` as the comment pair**—single-line vs multi-line (executable) block toggle. Do not describe `_next` only as “skip ahead for control flow.”

Loops still need `_goto` in DV; block comment semantics need `_next` + repeatable `:_end` tortured early.

---

### 4.7 RAM and code budget vs README size story

README claims FULL profile ~3 KB `.text` on ARM Thumb. The script design adds, at minimum:

| Addition | Estimated impact |
|----------|------------------|
| `XELP_SCRIPT_ARENA_SZ` (default 2048) | **+2 KB RAM per instance** |
| Evaluator loop + builtin dispatch | +800–2000 bytes `.text` (conservative) |
| Paren pre-pass + CONT/FRAME records | +200–400 bytes `.text` |
| Math/compare builtins (even subset) | +300–800 bytes `.text` |
| Variable heap + expansion formatters | +200–400 bytes `.text` |

**2 KB RAM per instance** is acceptable on ESP32; it is **not** acceptable on ATtiny85 where the whole FULL build is ~5 KB flash and RAM is measured in hundreds of bytes.

**Recommendation:**

- Define **`XELP_ENABLE_SCRIPT`** (or tier flags) that **compile out** arena + evaluator entirely.
- Publish **three script profiles** with measured sizes: Script-Minimal (no procs, no `_goto`), Script-Standard, Script-Full.
- Default `XELP_SCRIPT_ARENA_SZ` should scale by platform macro or document integrator override aggressively (256–512 bytes for small AVR if script is enabled at all).

The exploratory “Tier 1 / Tier 2” section at document end is the right instinct but **contradicts** the normative body, which assumes Tier 2 throughout. Promote tier boundaries into numbered requirements or delete the exploratory section to avoid dual truth.

---

### 4.8 `XELP_CMDBUFSZ=64` vs rich `_if` lines

The spec acknowledges `_if x _then y _else z` fits in 64 bytes with short commands—but **does not require** larger buffers for script profiles. Nested `_if` at CLI, or `_if` with expanded `$variables`, will truncate silently via `XELP_E_CMDBUFFULL` today.

**Recommendation:** script-enabled profiles SHOULD default **`XELP_CMDBUFSZ` ≥ 128** and **`XELP_ARGV_MAX` ≥ 16**, with a normative row (new ID under C-* or L-*) stating minimums when `XELP_ENABLE_SCRIPT` is on. Reconcile AGENTS.md (16) with `xelpcfg.h` (8).

---

### 4.9 Symbol alias commands (`+`, `>`, `==`) and namespace collisions

B-03 allows symbol commands as aliases. Unlike `_`-prefixed builtins, **`+` is not reserved**—a firmware project could already expose `+` or `>` as a user verb (unlikely but possible; `:` **will** collide with label syntax in argument positions).

**Recommendation:**

- Symbol aliases SHOULD be **profile-gated** (`XELP_SCRIPT_SYMOPS`) default **off** on constrained builds.
- Document dispatch order: builtins ( `_` ) → script funcs → **symbol alias table (if enabled)** → user C table.
- Warn integrators that **`:` in argument position** is a label token in script buffers, not a generic character—this differs from plain CLI today.

---

### 4.10 Variable lookup: hash without collision policy

Arena variables use `nameHash_2B` with linear scan (AR-*). **Collision handling is unspecified.** Two distinct names can share a 16-bit hash.

**Recommendation:** on hash match, **confirm with byte comparison** against the variable name stored in the frame argv region or a name suffix in the entry. Hash is an accelerator, not identity. Add AR-08 or VL-07 requiring collision-safe compare.

---

### 4.11 `_func` root-frame exception vs SC-01 purity

Open items say `_func` **always writes PROC entries to root heap**, even when called from nested frames—an explicit exception to frame-local variables.

**Concern:** this is correct pragmatically (persist procs) but **conceptually leaks** “special case” into an otherwise clean frame model. Dynamic redefinition shadowing C commands (DA-07) is powerful and **dangerous on production BLE consoles** unless M-01 capability masks block it.

**Recommendation:**

- Normative **M-01** examples should include “no dynamic `_func` on BLE instance.”
- Consider **`_func` only from C-registered ROM table in production profiles**, dynamic `_func` debug-only—promote to requirement if security matters.

---

### 4.12 Float section is thoughtful but should stay deferred

The hex-float expansion proposal (lossless, tiny) is **architecturally sound** for embedded. It also multiplies type tags, formatters, `XelpArgvFloat`, literal parsing, and truthiness cases.

**Recommendation:** keep **`XELP_ENABLE_FLOAT` off MVP** entirely. VT-* already reserves enum slots—good. Do not let float work block first ship; the INT/STR/NIL path is already large.

---

## 5. Positive alignment with existing code (exploit, don’t rewrite)

| Existing asset | Script spec should leverage |
|----------------|----------------------------|
| `XelpTokLineXB` PSM | Statement boundaries, comments, quotes—extend, don’t fork |
| `_xelpBuf2Argv` | User-command argv path; refactor front-half to `_xelpNextTokSpan` per PE-02 |
| `XelpFindTok` + `XELP_TOK_LINE` | `_next :label` forward scan |
| `XelpParseNum` / `XelpArgvInt` | TI-01 type inference and TH-05 narrowing |
| `mR[1..3]` | Optional “fast int return” for C without result-stack peek |
| `XelpBuf` cursor model | Frame `retAddr` / script bounds map naturally to `XelpBuf` |
| Fuzz tests + JumpBug | TR-* matrix maps directly to new test units |
| Multi-instance | Arena per `XELP`—no change to instance model |

**Avoid:** proposal3’s full `XelpCall` fat-pointer ABI **unless** you are willing to break every handler signature again. The requirements’ choice—keep `(ths, argc, argv)` for C, typed returns via `XelpSetResult*`—is the lower-risk path given v0.4.0 just migrated to argc/argv.

---

## 6. Requirement clusters — targeted verdicts

| Cluster | Verdict | Notes |
|---------|---------|-------|
| C-* constraints | **Approve** | Non-negotiable; add script-profile RAM minimums |
| N-* native code | **Approve** | Strong positioning |
| A-* anti-goals | **Approve** | Hold the line on A-01 (no Tcl substitution) |
| L-* / B-* surface | **Approve with edits** | Resolve `_proc`/`_func`; gate symops |
| I-* nesting | **Approve** | Core value proposition |
| R-* returns | **Revise** | Reconcile with AR-*; fix R-10 / CC-04 |
| VT-* types | **Approve MVP scope** | INT/STR/NIL only for v1 |
| AR-* arena | **Approve** | Make canonical over separate stack array |
| EX-* expansion | **Approve** | Frame-owned argv in arena is correct |
| SG-* / TI-* | **Approve** | Implement raw-token builtin path carefully |
| VL-* lifecycle | **Approve** | Type immutability is right for embedded |
| TH-* / TR-* | **Approve** | Ship tests before `_if` |
| EH-* errors | **Needs API** | “Structured failures / introspection TBD” must become concrete error codes or `mR` slots before field use |
| F-* control flow | **Approve full surface for DV** | `_goto` + `_next` together; fix `_if` dispatch target first |
| SC-* scoping | **Approve** | CLI root-frame rules (SC-05–07) are excellent |
| PE-* parens | **Approve cautiously** | Pre-pass cost is OK; prove idempotence with tests |
| IO-* | **Approve** | `_print` design is minimal and correct |
| DA-* dispatch | **Approve** | Clean separation |
| MA-* math | **Approve full roster for DV** | Bitwise + `_inc/_dec` stress type rules; trim only at *product* profile time |
| M-* policy | **Strengthen** | Add dynamic `_func` restriction examples |
| S-* safety | **Approve** | Best section in the doc |

---

## 7. Open items — reviewer recommendations

| Open item | Recommendation |
|-----------|----------------|
| `_func` mechanics | Promote settled open-item text to normative F-/DA- rows; delete `_proc` |
| `XelpCallProc` / `XelpGetResult` | **Approve CC-01–06 with edits:** `mR[1]` for int mirror, not `mR[0]`; document arena pointer lifetime loudly; check arena headroom before frame push |
| Float | **Include in DV build** — hex expansion path is the stink test for EX-08 / argv round-trip |
| `inkey` / `input` | **Agree:** C handlers, not builtins; requires optional input HAL or documented integrator pattern |
| Debug/trace | **Done** via `mpfBreakpoint` — add one example under `examples/script-debug/` |

---

## 8. Design validation build (revised — test the full story pre-release)

**Context (2026-05-24):** An earlier draft of this feedback recommended deferring `_goto`, float, dynamic `_func`, symbol aliases, and related features to a “minimal MVP.” That advice optimizes for **shipping surface area**, not for **grammar and architecture validation**.

**Correct goal before public release:** build and torture-test the **widest plausible story** on host/POSIX (JumpBug + libFuzzer + scripted corpora), find where the thinking stinks, then **carve product profiles** (Tier 1 / lean AVR / BLE-restricted) from measured evidence—not the other way around.

Deferral belongs at **integrator profile** time (`xelpcfg.h`, `xelp_ovr.h`), not at **design proof** time.

### 8.1 What must be fixed *before* edge-case testing (or tests lie)

These are not “defer”—they are **preconditions**. Running fuzz against contradictory foundations produces false confidence:

1. **One memory model** — arena byte-stream (AR-*) canonical; retire separate `XelpVal[]` wording in R-*.
2. **One status/return contract** — resolve `mR[0]` vs result stack vs `_if` vs `_return` (§4.2); tests must assert a single spec.
3. **One proc keyword** — `_func` everywhere; `_proc` poisons test corpora.
4. **`XelpEvalLine` (or equivalent)** — `_if` / `_next` / `_goto` must not call full `XelpParse` re-entrantly (§4.3); otherwise control-flow tests fail for the wrong reason.

Everything else in the requirements doc should be **implemented and stressed** in the DV build.

### 8.2 DV build — intentional full surface

Build **one fat profile** (`XELP_ENABLE_SCRIPT` + all language features) for x86-64/ARM host:

| Area | Include in DV | Why now |
|------|---------------|---------|
| Control flow | `_if`, `_next`, `_goto`, `:_end`, labels | Loop + block-skip semantics; `_goto`/`_next` duality is the highest footgun density |
| Procs | C-registered ROM funcs **and** dynamic `_func` at CLI | Root-frame exception, shadowing (DA-07), persistence across frame pop |
| Nesting | `( )`, CONT cleanup on jump (F-19–21) | Fuzz target #1 — orphaned continuations |
| Addressing | `$`, `@1..`, `@#`, `@name` if spec’d (D-03) | Half-shipped `@name` without `@#` is itself a test of D-03 |
| Math | Full MA-* roster incl. bitwise, `_inc/_dec`, symbol aliases | Namespace + type errors; alias vs user `+` collision |
| IO | `_print`, `_lpad` | Composition inside `( )` with live ADC-style procs |
| Float | `XELP_ENABLE_FLOAT`, hex expansion default | EX-08 formatter dispatch; argv round-trip honesty |
| C↔script | `XelpCallProc`, `XelpGetResult`, `XelpSetResult*`, re-entrancy | CC-06 depth + arena headroom |
| Safety | `mpfBreakpoint` with budget + break + trace example | `_goto` loop without budget must fail deterministically |
| Input (exploratory) | `inkey` / `input` as **example C handlers** | Blocking + result stack; not builtins, but story completeness |
| Deferred in spec only | `_run` (eval string) | Still worth a **spike** if time permits — it stress-tests tokenizer + frame isolation |

**Product trimming** (no float on AVR, no dynamic `_func` on BLE, symops off) comes **after** DV passes and produces size tables.

### 8.3 Stink tests — what to run, what smell means “revise spec”

Each row is a deliberate attempt to break the story. Failures should become normative rows or explicit anti-patterns in the spec—not silent “implementation quirks.”

#### Control flow + labels

| Test | Pass criterion | Stink if… |
|------|----------------|-----------|
| `_goto :top` loop + step budget | Clean stop at budget; arena SP/HP sane | Budget only counts statements but `_goto` bypasses hook; or arena leaks across iterations |
| `_next :_end` × N blocks | Each skip lands at nearest forward `:_end` | Scan crosses frame boundary or matches inside quotes |
| **Block disable (multi-line “comment”)** | Lines between `_next :_end` and `:_end` **never dispatch** | Any C handler in skipped region runs; variables mutate; hardware side effects |
| **`#_next :_end` re-enable** | Block runs again; `:_end` alone harmless | `#` span wrong; partial tokenization of block |
| **Nested blocks** | Outer `_next :_end` skips to its `:_end`, not inner | Inner `:_end` steals outer skip target |
| `_goto :_end` | Documented wrong-target behavior | Spec says “script bug” but interpreter corrupts stack |
| `_if $x _then _goto :back` | Backward from `_if` via explicit `_goto` | Bare `:label` sugar accidentally allows backward |
| Duplicate labels + `_goto` | First-from-top match, deterministic | Nondeterministic match or cross-artifact jump |
| `_if` with 64-byte line + long else | Predictable error, no partial dispatch | Silent truncate mid-clause |

#### Jumps × parentheses (F-19–21)

| Test | Stink if… |
|------|-----------|
| `_set x (+ 1 (_goto :L))` | CONT records left on stack after jump |
| `_goto` landing mid-token stream | Unbounded parse or wrong “inside expr” state |
| `_next :label` past closing `)` | Expression abandoned but caller argv half-built |

#### `_func` / frames / shadowing

| Test | Stink if… |
|------|-----------|
| `_func foo "..."` then call `foo` from nested proc | PROC not visible or visible when it shouldn’t be |
| Dynamic `foo` shadows C `foo` | Dispatch order differs between script path and `XelpParseXB` |
| Redefine `_func foo` at CLI | Old body reachable; arena leak; C registration “overwritten” |
| Frame pop after `calibrate` | Parent `$gain` invisible (SC-01) but `@1` from wrong frame |

#### `$` / types / truthiness

| Test | Stink if… |
|------|-----------|
| Full TR-* matrix + fuzz-generated strings | `_if` and `_truthy` disagree |
| `_eq 3 "3"`, `(+ $a $b)` STR+INT | Silent coercion |
| `_set $ptr 42` indirect | Requires special-case beyond SG-01 |
| `$cmd` in command position (PE-07) | Accidental full-line eval or wrong dispatch table |

#### Float + argv boundary

| Test | Stink if… |
|------|-----------|
| `_set x 0.1` → `motor $x` → `XelpArgvFloat` | Bit pattern not lossless under hex mode |
| `_print $x` under hex vs dec profile | Same variable, inconsistent handler view |
| `3.14` without `XELP_ENABLE_FLOAT` | Error vs STR(TI-06) inconsistent with `_set` |

#### Symbol aliases + dispatch

| Test | Stink if… |
|------|-----------|
| User registers `+` and builtin `+` enabled | Nondeterministic or silent wrong handler |
| `_+` token | Tokenizer or docs ambiguity (B-04) |

#### Re-entrancy + C API

| Test | Stink if… |
|------|-----------|
| `XelpCallProc` → C handler → `XelpCallProc` | Frame argv clobber; depth limit unclear |
| `XelpGetResult` after nested calls | Stale arena pointer; use-after-pop |
| `_return` int + read `mR[0]` vs `mR[1]` vs `XelpGetResult` | Three different answers for one return |

#### Multi-instance + policy

| Test | Stink if… |
|------|-----------|
| Instance A defines `_func`; instance B unchanged | Global leakage |
| BLE instance with capability mask | Mask enforced at dispatch, not just registration |

### 8.4 What DV is *not*

- **Not** proof that ATtiny85 runs the full story — profile carving follows measurement.
- **Not** permission to ship contradictory normative text — fix §4.1–4.3 first, then fuzz.
- **Not** an excuse to skip size tables — run `make funcsizes` / cross-build **after** DV stabilizes grammar.

---

## 9. Documentation hygiene before implementation

1. **Single memory model** — edit R-* to reference AR-* stack region.
2. **Single status model** — edit R-10, F-05, CC-04 together.
3. **Rename `_proc` → `_func`** everywhere normative.
4. **Remove or implement** `mpfIn*` / `mpfGetTicks` / `mpfDelay` references.
5. **Reconcile** `XELP_ARGV_MAX`, `XELP_CMDBUFSZ` across spec, `xelpcfg.h`, AGENTS.md.
6. **Resolve Tier 1/2 exploratory section** — either promote to C-* flags or mark non-normative clearly at top.
7. **Cross-link** `XelpFindTok` as the intentional label mechanism (reduces perceived novelty risk).
8. **Version bump policy** — script is a **minor** (0.5.0) or **major** feature; struct growth in `XELP` affects ABI size—call out in release notes for static-allocation integrators.

---

## 10. Bottom line

The requirements document reads like a team that **knows embedded** and **knows how command interpreters fail in the field**. The core bet—command-shaped orchestration with typed nesting, not a VM—is compatible with xelp’s brand and codebase.

**Pre-release, the right sequence is:**

1. **Coherence pass** — one memory model, one return/status story, one proc name, one eval entrypoint for control-flow builtins (§4.1–4.3). Small doc edits; huge test validity.
2. **Design validation build** — implement the **full language story** on host; run stink tests (§8.3) and extend fuzz corpora until invariants break or the spec bends deliberately.
3. **Profile carve-out** — Tier 1 / lean / BLE-safe profiles are **subsets** of a validated whole, with measured `.text`/RAM—not subsets of an untested guess.

Deferral is for **integrators**, not for **you**, right now. The worst outcome is not “too much code in v0.5”—it is shipping a grammar that passes thin tests because `_goto`, dynamic `_func`, float, and jump-inside-`()` were never exercised, then discovering in the field that the arena model or `mR[]` contract cannot support the next enhancement without a breaking change.

That is exactly what this phase is for: **find the stink while the story is still yours to rewrite.**

---

*End of feedback document.*

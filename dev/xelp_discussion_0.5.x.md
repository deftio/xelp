# xelp 0.5.x — Discussion Capture

Captured from design and release review conversations (May 2026). This is a
**discussion record**, not a spec. Normative script requirements live in
[xelp_script_requirements.md](xelp_script_requirements.md). Open engineering
gates before script work: [issues.md](issues.md).

**Status:** 0.4.0 foundation in tree; **0.5.x branch not started** at time of
writing. Script implementation deferred until gates in `issues.md` are closed.

---

## 1. What we were doing

Two parallel tracks:

1. **Ship-quality 0.4.0** — docs, site (`pages/`), tests, fuzz aligned with
   native `(ths, argc, argv)` dispatch.
2. **Think toward 0.5.x** — xelp-script as “C’s missing interpreter” for
   orchestration, factory bring-up, and live hardware inspection — **without**
   starting the branch yet.

This document preserves the *why* behind 0.4.0, the assessment of 0.3.3, and
the architectural direction for 0.5.x so it does not live only in chat history.

---

## 2. Session work completed (0.4.0 hygiene)

### 2.1 GitHub Pages (`pages/`)

All site HTML updated from stale **0.3.3** / `(ths, args, len)` / optional
`XELP_ENABLE_ARGV` content to **0.4.0**:

| Page | Main corrections |
|------|------------------|
| `index.html` | Version badge; native argc/argv feature; quick-start handler |
| `api-reference.html` | Removed `XelpArgs`, `XelpTokN`, `XelpBuf2Argv`, `XELP_PARSE_ARGV`, `XelpArgInt`; added argument helpers, default handlers, `XelpPutc`; `XELP_VERSION` = `0x00000400` |
| `tutorial.html` | Commands-with-args and iteration via dispatch argv |
| `examples.html` | Handler patterns for argc/argv |
| `configuration.html` | No `XELP_ENABLE_ARGV`; argv buffer tied to `XELP_ENABLE_CLI` |
| `porting.html` | Handler signature |
| `releases.html` | v0.4.0 section; historical 0.3.x entries retained |

`docs/*.md` and `AGENTS.md` were already largely on 0.4.0 before this pass.

### 2.2 Validation and fuzz

```
make clean && make validate   → PASS
  45/45 units, 597/597 cases, 100% line coverage (xelp.c)

make fuzz (libFuzzer)         → PASS
  fuzz_parsekey, fuzz_parse, fuzz_buf2argv (60s each)
```

**Fix applied:** `tests/fuzz/fuzz_parse.c` missing `#include <stddef.h>` for
`size_t` (parsekey/buf2argv already had it).

**Note:** First `validate` without `make clean` can fail on stale `.gcda`
artifacts (coverage mismatch). CI clean builds are fine; local dev should
`make clean` after profile-changing edits.

### 2.3 Dispatch safety (argv failure)

Unit tests expect that when `_xelpBuf2Argv` fails (too many tokens, line longer
than `XELP_ARGVBUFSZ`), **handlers are not called** and `mR[0]` reflects error.

`XelpParseXB()` dispatch path checks `_xelpBuf2Argv` return before invoking
matched or default CLI handlers. See [issues.md](issues.md) #1–#2 for remaining
test-hardening (explicit handler call-count assertions).

---

## 3. Assessment: v0.3.3 as a release

**Summary:** 0.3.3 is an excellent release — the fullest expression of the
*classic* xelp line. It is a defensible pin for integrators who want a mature
embedded CLI **without** scripting ambitions. 0.4.0 is not a rejection of
0.3.3; it is the foundation for the interpreter direction.

### 3.1 What made 0.3.3 strong

1. **Evolutionary continuity** — Decades of `fn(args, len)` became
   `fn(XELP *ths, args, len)` in 0.3.0 without breaking the mental model:
   engine dispatches, handler parses what it needs.

2. **Progressive disclosure** — Three argument styles coexisted:
   - `XelpTokN` / pointer spans (smallest, zero-copy)
   - `XelpArgs` iterator (ergonomic sequential access)
   - `XELP_PARSE_ARGV` + `XelpArgvInt` / `XelpArgvStr` (C-native, null-terminated)

   Teams chose code size vs ergonomics. That is rare in embedded APIs.

3. **Additive argv** — `XELP_ENABLE_ARGV` was optional. Existing handlers
   could stay on `(ths, args, len)` indefinitely. Migration was voluntary.

4. **Feature completeness** — History, escape maps, `xelp_ovr.h`, BLE
   dual-instance example, ENTER CR/LF split, fuzz for `XelpBuf2Argv`, large
   test matrix (~49 units / ~678 cases at ship). Serious engineering for a
   ~2.5 KB library class.

### 3.2 Lineage (informal)

```text
ancestors:     fn(args, len)              handler owns all parsing
0.3.0:         fn(ths, args, len)         instance-aware, same contract
0.3.1–0.3.2:   iterators + direct helpers   ergonomic layers
0.3.3:         optional argc/argv           C-native path, still optional  ← peak classic
0.4.0:         native argc/argv             engine owns tokenization         ← script prep
0.5.x (plan):  script builtins + flow      engine owns orchestration too
```

### 3.3 Why 0.4.0 was necessary anyway

0.3.3 optimized **integrator flexibility**. 0.4.0 optimizes **call-boundary
ownership** for xelp-script:

| Concern | 0.3.3 | 0.4.0 |
|---------|-------|-------|
| Who tokenizes? | Often the handler | Always dispatch |
| Public argv APIs | `XelpBuf2Argv`, macro, helpers | Internal `_xelpBuf2Argv`; `XelpArgvInt`/`Str` only |
| Handler signature | `(ths, args, len)` or opt-in argv | `(ths, argc, argv)` only |
| Script prep | Handler might re-parse | Engine can inject/expand argv once |
| Failure on overflow | Macro/handler responsibility | Dispatch can fail closed |

0.4.0 is a **philosophical** break, not a patch release — but it preserves
xelp’s identity: no heap, no globals, ROM-safe scripts, C functions as vocabulary.

---

## 4. v0.4.0 — what shipped (reference)

From [CHANGELOG.md](../CHANGELOG.md) and review:

**Breaking**

- CLI: `XELPRESULT fn(XELP *ths, int argc, const char **argv)`
- `mpfDefCLI` same shape
- `XelpBuf2Argv` → internal `_xelpBuf2Argv`
- `XELP_ENABLE_ARGV` removed; argv always on when `XELP_ENABLE_CLI`
- `mArgvBuf` under CLI guard (RAM unchanged vs default 0.3.3 with argv on)

**Removed**

- `XelpArgs` iterator (`XelpArgsInit`, `XelpNextTok`, …)
- `XelpArgInt`, `XelpArgStr` (positional on raw span)
- `XelpTokN`, `XelpNumToks`
- `XELP_PARSE_ARGV` macro

**Kept**

- `XelpArgvInt`, `XelpArgvStr`
- `XelpTokLineXB` (PSM line/token split for dispatch)
- KEY / THR modes unchanged

**Migration (one-liner checklist)**

1. Change every CLI handler to `(ths, argc, argv)`.
2. Replace iterator / `XelpTokN` / `XelpNumToks` with `argv[n]` / `argc`.
3. Delete `XELP_PARSE_ARGV` — argv are parameters.
4. Remove `XELP_ENABLE_ARGV` from overrides.

---

## 5. Toward 0.5.x — product intent

### 5.1 Positioning

**Xelp Script** = orchestration and bring-up over registered C verbs — not a
Lua/MicroPython/Tcl replacement.

Target users: makers, factory techs, firmware authors debugging live hardware
(I2C/SPI bring-up, `_peek`/`_poke`, register dumps), internal engine tuning
(e.g. MIDI/synth parameter ladders).

```text
C/C++     = fast path (drivers, timing, protocol, math)
Xelp CLI  = interactive command surface (today)
Xelp Script = ROM-able command programs (tomorrow)
```

Core identity unchanged: **C functions are the native vocabulary**, not foreign
bindings.

### 5.2 Possible tiers (exploratory — not committed)

Discussed staging; see end of [xelp_script_requirements.md](xelp_script_requirements.md).

```text
Xelp CLI  ⊆  Tier 1  ⊆  Tier 2
```

| Tier | Sketch | Parentheses | `$` vars | Flow | Builtin table |
|------|--------|-------------|----------|------|----------------|
| CLI (today) | User commands only | no | no | no | no |
| Tier 1 | Inspector / bring-up | no | no | minimal | optional `_*` before user CLI table (`_peek`, `_poke`, `_echo`, int math) |
| Tier 2 | Full script | yes `( )` | yes `$` / `@` | `_if`, `_goto`, labels | full builtin + proc story |

**Lean order of work discussed:** Tier 1 as a **command suite** on existing
argc/argv dispatch before DAG `_if` without loops or full Tier 2.

### 5.3 Language shape (recap)

- Surface: `command arg arg …` — Tcl-like words, not Tcl substitution soup.
- No infix algebra in core; prefixes as commands: `(+ 1 2 3)`, `_if (> $x 5) high`.
- Two invocation shapes for authors: statement vs `(nested value)`.
- Truthiness is explicit and testable (`TR-*` matrix); see
  [xelp_truthiness_trap_catalog.md](xelp_truthiness_trap_catalog.md).
- Interrupt: long scripts need ESC ESC / CTRL-C style **live** abort between
  statements (engine stays responsive).

### 5.4 Memory and ABI direction

**Discussed constraints (not all implemented):**

| Topic | Direction |
|-------|-----------|
| Heap | Still no malloc; bump pool per instance (~2–4 KB default on 32-bit; ~8 KB script-heavy profile) |
| argv storage | Pointers in instance pool, not handler stack long-term |
| Token cap | Byte budget dominates over raising `XELP_ARGV_MAX` alone |
| Call depth | ~128–200 B/frame target for orchestration-depth scripts |
| Tier 2 calls | Likely `XelpCall`-style fat context (raw span + typed return), not only argc/argv |
| `@` expansion | Pure replacement (C-injected params) exposes expand-buffer / ROM-copy / pool sizing early |

**Size envelope (Thumb -Os, rough):**

- Lean Tier 2 + CLI: ~5–6 KB plausible
- Rich profile ceiling: ~8–10 KB

### 5.5 Naming conventions (discussion)

- Canonical builtins: `_add`, `_band`, `_bor`, `_bxor`, `_truthy`
- Optional aliases: `+`, `&`, `|`, `^` — avoid `_+` as a builtin name
- Truthiness: `_truthy` canonical; document traps from other languages

### 5.6 What 0.4.0 gives script for free

- Single tokenization path into null-terminated argv
- `XelpArgvInt` / `XelpArgvStr` for bounds-checked access
- Registers `mR[0..3]` as return mailbox (callee-clobbers-all)
- PSM tokenizer unchanged for `#`, `;`, quotes, CLI/script escapes

What still needs design before coding: expansion, variables, `( )` evaluation,
builtin vs user dispatch order, ERR propagation, pool layout — see proposals and
[issues.md](issues.md).

---

## 6. Open gates before 0.5.x coding

From [issues.md](issues.md) (paraphrased; check file for current status):

| # | Topic |
|---|--------|
| 1 | ~~Dispatch after argv failure~~ — addressed in tree; verify tests |
| 2 | Handler call-count tests on argv overflow |
| 3 | ~~Fuzz stale~~ — harnesses updated; keep aligned |
| 4 | Document argv pointer lifetime (transient until handler returns / next parse) |
| 5 | `XELP_ARGV_MAX` default: header=8 vs AGENTS.md=16 — pick one |
| 6 | Remove dead `XELP_ENABLE_ARGV` from size tooling |
| 7 | One uniform argument/value model in script proposal |
| 8 | Call stack + variable stack spec (pool-backed) |

Do **not** treat script implementation as unblocked until these are consciously
closed or accepted as deferred with written risk.

---

## 7. Related documents

| Document | Role |
|----------|------|
| [xelp_script_requirements.md](xelp_script_requirements.md) | Normative script requirements (`A-01`, `C-01`, …) |
| [xelp_script_proposal3.md](xelp_script_proposal3.md) | Unified target spec (draft) |
| [xelp_script_impl_v1.md](xelp_script_impl_v1.md) | Pseudocode walkthrough (design fiction) |
| [xelp_truthiness_trap_catalog.md](xelp_truthiness_trap_catalog.md) | Cross-language truthiness → `TR-*` tests |
| [issues.md](issues.md) | Pre-script engineering gates |
| [manu_xelp_notes_legacy.md](manu_xelp_notes_legacy.md) | Historical design notes |
| [arg_parse_updates.md](arg_parse_updates.md) | argv migration notes |
| [CHANGELOG.md](../CHANGELOG.md) | Released version facts |

---

## 8. Historical decisions (archived plans removed)

- **Unified `XelpCmd` table** (early “xelp 2.0” plan) — considered and
  **not adopted**. Kept separate KEY/CLI tables and separate dispatch;
  0.4.0 instead standardized on `(ths, argc, argv)` for CLI handlers.

## 9. Decisions explicitly deferred

- **0.5.0 branch** — not created at time of capture
- **Tier 1 vs Tier 2 first ship** — leaning Tier 1 command suite; not committed
- **`XELP_ARGV_MAX` default bump to 16** — discussed, not decided
- **Full `XelpCall` ABI** — Tier 2; not specified in 0.4.0

---

## 10. One-paragraph executive summary

**0.3.3** perfected the optional-layer classic xelp: handlers could stay on raw
spans forever or adopt argc/argv when convenient. **0.4.0** moved tokenization
into dispatch so the engine can own the call boundary for **xelp-script**.
**0.5.x** will add a no-malloc, ROM-safe orchestration layer on top of C
commands — inspector-tier `_*` builtins first, full `$` / `( )` / flow later —
without becoming a general-purpose VM. Pages, tests, and fuzz now match 0.4.0;
script work waits on documented argv safety, consistent limits, and a single
argument model in the requirements/proposal stack.

---

*Captured: 2026-05-18. Update this file when tier boundaries or gate status
change materially.*

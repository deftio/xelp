# XELP Script — Proposal 3 (unified CLI + script ABI)

**Status:** DRAFT — design target for post-0.3.3; not implemented.  
**Replaces/supersedes:** informal choice between `xelp_script_proposal1.md` and `xelp_script_proposal2.md` for *API shape*; language examples may still borrow from either.

This document resolves the main tension between earlier proposals:

| Topic | Proposal 1 (skew) | Proposal 2 (skew) | Proposal 3 (resolution) |
|-------|-------------------|-------------------|-------------------------|
| Positional args | `$1`, `$2`, `$#` | `@1`, `@2`, `@#` | **`@1`, `@2`, `@#` for call parameters; `$` reserved for named variables only** (no `$1` — avoids shell ambiguity and matches “arg vs var” mental model). |
| Variables | `$name` (often int-only expansion) | `$name` as typed cell | **`$name` — typed lookup** (int/str at minimum); expansion is **not** primary string substitution into raw text. |
| User C ABI | `(args, len)` + optional argv | Sketched `XelpArgIter` + different signature | **Single unified `XelpCall` (see below)** — one ABI for CLI, script statement, and value position. |
| Return value | Registers / `$?` | Typed return + `mR[]` overload | **`XELPRESULT` status + `XelpValue` (or compact tagged union) for script-visible value**; **`mR[1..3]` remain optional fast int slots** for C ergonomics. |
| Operators | `_add`-style | `+` / `_add` aliases | **Profile A:** `_add`, `_band`, `_lt`, … only. **Profile B (+code):** thin aliases `+`, `<` mapping to same builtins — never infix parsing. |

---

## 1. Hard requirements (non-negotiable)

1. **No dynamic allocation** — no `malloc`; all runtime state in fixed buffers owned by the instance or supplied at init.
2. **Multi-instance** — no global interpreter state; each `XELP` (or paired `XELP` + script context) is independent.
3. **Command form** — `command arg arg ...` at every level; **nested value position** uses balanced parentheses: `(command arg arg)`.
4. **ROMmable scripts** — source text may live in flash; core must not write into source bytes for execution.
5. **Natural C ↔ xelp** — registering a C function and calling it from script looks like calling any other command; `XelpParse` / interactive CLI can run script fragments; C can invoke script entrypoints with a stable API.
6. **Size mindset** — prefer **&lt; 8 KB** `.text` on typical 32-bit Thumb `-Os` for core + minimal script profile; **hard ceiling ~10 KB** for a rich profile; RAM **tiered** (see §7).

---

## 2. Design center

- **C/C++ is the fast path** — drivers, bit-twiddling, DSP, crypto, heavy parsing stay in C.
- **Xelp Script is the glue** — bring-up sequences, calibration, field macros, branching, **calling C** with structured arguments and return values.
- **Same tokenizer philosophy as xelp** — small table-driven state machine for **line/stream** tokenization; **separate bounded scanner** for **script words** (literals, `$x`, `@1`, quoted strings, `( ... )` nesting). No full AST, no infix grammar.
- **Typed values, not Tcl-style string soup** — words evaluate to small tagged values; **printing** is explicit (`print`, `echo`, or C `XelpOut`), not “value coerced to string everywhere.”

---

## 3. Unified application binary API (`XelpCall`)

To get **one** consistent story for:

- interactive CLI dispatch  
- script statements `foo 1 2`  
- nested value calls `(foo 1 2)`  
- optional raw-line access for logging or custom parsers  

introduce a **single** C command signature and a **fat pointer** to call context:

```c
typedef struct XelpCall XelpCall;

XELPRESULT my_cmd(XELP *ths, XelpCall *call);
```

### 3.1 What `XelpCall` exposes (conceptual)

Implementations may use a struct + accessors/macros; fields are illustrative:

| Facet | Purpose |
|-------|---------|
| **`argc` / `argv[]`** | **Null-terminated tokens** for the current invocation after script preprocessing for **this** command (always safe to use for typical handlers). |
| **Raw span** | **`const char *line`, `int len`** — optional slice of the **current statement source** (ROM or RAM), for handlers that need original text or custom tokenization. |
| **Call kind** | **CLI line**, **script line**, **nested value call** — bit flags so handlers can reject nesting or change behavior. |
| **Value result** | **`XelpReturnInt` / `XelpReturnStr` / `XelpReturnErr`** (names TBD) — how a C command **returns a value** to script when invoked in **`(...)`** position. |
| **Status** | **`XELPRESULT`** remains the command execution status (`XelpParse`/`XelpParseKey` still write `mR[0]` conventionally). |

### 3.2 Why not keep only `(args,len)` or only `argc,argv`

- **Raw `(args,len)`** (0.3.3 style): excellent for **non-script** minimal helpers and custom parsers; painful for **every** command hand-rolling argv.
- **Plain `argc,argv` only**: excellent for ergonomics; **insufficient** for “give me the untouched line” and for **typed return** without inventing parallel APIs.

**`XelpCall` unifies** ergonomics (argv), transparency (raw span), and script composition (typed return) **without** a second handler function type.

### 3.3 `mpfDefCLI` and command tables

`XELPCLIFuncMapEntry` should use the **same** function pointer type as user commands. KEY mode stays **`(ths, keycode)`** — unchanged.

### 3.4 Backward compatibility

This is a **breaking** change vs 0.3.3 `(ths, args, len)` and vs experimental native `argc,argv`-only drafts. Migration: **thin wrappers** or one mechanical pass to read `XelpCallArgc(call)` / `XelpCallArgv(call)` instead of `args,len`.

---

## 4. Language surface (v1 target)

### 4.1 Statements and nesting

```text
command arg arg
( child arg )           # value position only — evaluates to one value
```

- **Left-to-right** evaluation of arguments; **`(...)`** evaluates **before** passing as argument to parent.
- **No infix** — `+ 1 2` or `_add 1 2`, never `1+2`.

### 4.2 Namespaces

| Token | Meaning |
|-------|---------|
| **`$name`** | Lookup **variable** `name` in current frame/script store. |
| **`@n`** | Positional argument **`n`** of the **innermost script-invoked** callable (`@1` = first arg after name). `@#` = count. |
| **Bare word** | In **command position**: resolve as built-in / script proc / C command. In **argument position**: **literal** (SYM/STR — implementation defines coercions to INT when passed to numeric ops). |
| **`"..."`** | String literal with `\` escapes (reuse `XELP_QUO_ESC` / `XELP_ESC_MAP` philosophy). |

### 4.3 Control (minimal first ship)

- **`_set name value`**
- **`_if value label`** — if truthy, jump to `label`
- **`_goto label`**
- **`:` `label`** — line-start label (parser recognizes dedicated label token; exact syntax TBD)

**Loops:** `_while` optional later; **`_goto` + `_if`** is enough for v1 and keeps code small.

### 4.4 Built-in naming

- **Language/control/math** builtins use **`_` prefix** — reserved and matched before user tables: `_set`, `_if`, `_goto`, `_return`, `_add`, `_band`, …  
- **Logical** vs **bitwise**: **`_land` / `_lor` / `_lnot`** vs **`_band` / `_bor` / `_bxor` / `_bnot` / `_shl` / `_shr`** — never overload one symbol for both.

### 4.5 Optional symbolic aliases

Compile-time profile:

- **`XELP_SCRIPT_SYMOPS=0`** — only `_add`, `_lt`, …  
- **`XELP_SCRIPT_SYMOPS=1`** — also register `+`, `<` as **command names** aliasing the same implementations (still prefix/command form, **not** infix).

---

## 5. Return model (script ↔ C)

- **`XELPRESULT`**: transport / engine status (success, warning, errors). Still maps to **`mR[0]`** for “last command status” reads.
- **`XelpValue`**: **one** script-visible return per command for use inside **`(...)`** — `INT`, `STR` (bounded), `BOOL`, `NIL`, `ERR`.
- **C helper API** (called inside command handler): e.g. `XelpReturnInt(ths, call, 42);`  
- **Register shortcut**: writing **`mR[1]`** may mirror **INT returns** for C authors who dislike helpers — document one blessed convention.

**Rule:** **`XelpOut` is not a return value** — side effect only.

---

## 6. Parsing architecture

### 6.1 Existing core

Keep **ROM-safe line PSM** (`XelpTokLineXB`) for:

- statement boundaries (`;`, `\n`)  
- comments `#`  
- first-token command name detection for **plain CLI without script**

### 6.2 Script layer (new)

When **`XELP_ENABLE_SCRIPT`** (name TBD):

1. **Word scanner** (bounded) produces a sequence of **words** from a statement string **without mutating source**. Words may be: literal, `"quoted"`, `$ident`, `@digits`, `@ident`, or **`(` … `)` balanced subexpression**.
2. **Nested evaluation** uses an **explicit frame stack** (see §7) — not the CLI `mArgvBuf` alone.
3. **Parentheses** are **not** “new core PSM states” on day one — they are **script scanner** logic sitting **on top** of the same character classes and escape rules.

---

## 7. Memory model (no malloc)

Per **script-enabled** instance, allocate (size via `xelpcfg.h` / override):

| Region | Role |
|--------|------|
| **`mArgvBuf` / CLI line buf** | Unchanged role: **scratch for current CLI dispatch** after tokenization of **one** line segment. |
| **`XelpScriptHeap[]` (new)** | Arena for: variable table, script proc bodies **copied** when defined interactively, temp strings, **call stack** records, **value stack** slots. |
| **Separate nested `argv` scratch (optional)** | One or more fixed buffers for **nested** `( ... )` evaluation so parent `argv` is not clobbered by child dispatch — **or** allocate child `argv` slots inside the script arena with frame pop. |

**Overflow:** every push returns **`XELP_E_ERR`** (or script-specific **halt**) — **no silent truncation**.

**Relocatability:** if the project still wants `memcpy` relocatable instances, heap must be **offset-based** or script layer opts out of relocation — **decide explicitly** before shipping.

---

## 8. Multi-instance and permissions

- Each instance has its own script heap and stacks.
- Optional **capability mask** (future): disable `_poke`, raw I2C from BLE instance, etc.

---

## 9. Execution limits

- **Step/statement budget** default **on** (compile-time default).  
- **`#define XELP_SCRIPT_UNBOUNDED` or runtime flag** for intentional **forever** loops (`while(1)` manufacturing test).  
- Halting leaves instance in **`XELP_S_OK`-ish idle** and **next command still works**.

---

## 10. C calling script; script calling C

**C → script**

```c
XelpScriptRun(ths, "blink 3 100");   /* name TBD */
/* or */
XelpScriptInvoke(ths, "factory_cal", argc, argv_like);  /* optional entry with args */
```

**Script → C**

Same as any command: **`sensor_read 2`** or **`(sensor_read 2)`** when a value is needed.

---

## 11. Tradeoffs (explicit)

| Choice | Gain | Cost |
|--------|------|------|
| **`XelpCall` fat API** | One mental model; raw + argv + returns | Struct size, indirection; **breaking** ABI change |
| **`@` not `$` for positional** | Clear separation from `$var` | Divergence from bash — **document hard** |
| **Typed values vs text expansion** | Predictable composition, fewer hidden coercions | More code than pure `$` string expand |
| **No infix** | Tiny parser | Ugly math for complex expressions → **do in C** |
| **Goto-if minimal control** | Small ROM footprint | “Ugly” — acceptable for embedded glue |
| **Separate script scanner + frames** | Robust nesting | RAM for stacks — **tunable by `#define`** |

---

## 12. Relationship to legacy notes

`dev/manu_xelp_notes_legacy.md` sketched `_if`, `_go`, variable stacks, and **no malloc** — consistent with this proposal. Old ideas like **`$1` positional** are **superseded** here by **`@1`** to match proposal 2’s clarity and avoid `$` overloading.

---

## 13. Open decisions (before implementation)

1. Exact **`XelpCall`** struct layout and **C89** compatibility for optional features.  
2. **Default `XELP_ARGV_MAX`** and **script max nesting depth** — balance stack/RAM vs ergonomics.  
3. **String length limits** and whether **STR** in `XelpValue` is **always copied** into script arena.  
4. Whether **script procs** can **shadow** C commands — default **no shadowing** recommended for safety.

---

## 14. Summary

**Xelp Script (proposal 3)** keeps xelp’s **command-shaped** spirit, adds **typed composition** via **`(...)`**, fixes the **`$` vs positional** ambiguity with **`@`**, and replaces dual ABIs with a **single `XelpCall`-based C command signature** so CLI, script, and nested evaluation stay **one coherent system** — **no malloc**, **multi-instance**, **ROM scripts**, **natural C ↔ script** — at the cost of a **deliberate breaking change** to the public command handler type when this layer ships.

# Truthiness trap catalog — other languages → Xelp test ideas

Companion to **`dev/xelp_script_requirements.md`** (§ Truthiness **`TH-*`** / **`TR-*`**). Goal: steal **pain points**, not semantics—then pin **one** **`_truthy`** table for Xelp and regress it hard.

References (stable entry points):
- MDN glossary *Truthy*: [developer.mozilla.org/docs/Glossary/Truthy](https://developer.mozilla.org/en-US/docs/Glossary/Truthy)
- Python **`bool`** on built-in objects: docs under *Built-in Types* (`bool()` / empty vs non-empty)
- PHP casts to **`(bool)`** / boolean context: [php.net/language.types.boolean](https://www.php.net/manual/en/language.types.boolean.php)
- C conditional “compare to zero” model: eg. cppreference **`if`** (C scalar → zero vs nonzero)
- SQL three-valued logic: eg. Postgres *Logical Operators* docs; **`NULL`** is **UNKNOWN**, not TRUE/FALSE.

---

## 1. Snapshot: “what counts as false?” (coarse comparison)

Rough guide only—the real lesson is **`"0"`** and **`0`** diverge wildly.

| Language / system | Typical falsy-ish values (high level, context-dependent) | Notes relevant to scripting foot-guns |
|-------------------|------------------------------------------------------------|----------------------------------------|
| **C** (`if(expr)`) | Expression **scalar** compared to **0**; **`NULL`** pointer compares like 0 | **No string** truthiness—you never `if(char*)` *meaningfully* without deciding size |
| **Python** (`if x:`) | **`None`**, **numeric zero**, **empty sequences/containers**, **`False`** | **`bool("0")` is True** — only **empty string** falsy among normal strings ([SO / docs pattern](https://stackoverflow.com/questions/57236475/why-0-and-0-are-evaluated-to-false)) |
| **JavaScript** (`if(x)`) | Fixed small **falsy set**: **`undefined`, `null`, `false`, `0/-0`, `NaN`, `""`, `0n`** … (+ legacy **`document.all`**) MDN *[Truthy]* | **`"0"` is truthy**; **`[]` / `{}`** are **truthy** while **`""`** falsy → big class of JS/Python divergences |
| **Lua** | Only **`false`** and **`nil`** are falsy | **`0`** and **`""`** are **truthy** ([Lua manual](https://www.lua.org/manual/5.4/manual.html)); opposite intuition from PHP/Perl |
| **Ruby** | Only **`false`** and **`nil`** are falsy | **`0`, `""`, `[]`** are **truthy** (Ruby FAQ / guides) |
| **PHP** / **Perl** | Typical cast-to-boolean: **`false`, `0`, `0.0`, `""`, `"0"`**; arrays / containers per language; **`NULL`** / **`undef`** | Famous: **`"0"` falsy** in PHP (and Perl style rules)—violates naive “non-empty ⇒ true” |
| **SQL (3VL)** | **`UNKNOWN`** from **`NULL`** poisons predicates unless short-circuit | **`NOT NULL`** is **`NULL`**; **`AND`/`OR`** tables unlike boolean algebra alone |

Lesson: **`"0"`** splits **three camps**: **truthy** (JS, Python), **falsy** (PHP, Perl), **does not apply until you coerce** (C strings—don’t pretend they’re booleans).

---

## 2. Trap families (borrow the failure mode, decide Xelp separately)

Each row suggests **hypothesis-facing tests** **`TR-…`** (extend the matrix in **`xelp_script_requirements.md`**).

### A. **`0` (INT) vs `"0"` (STR)**

| Trap | Seen in | Xelp implication |
|------|---------|------------------|
| **Numeric zero falsy**, **string `"0"` unrelated** unless explicit parse | Python, Ruby, Lua-ish split | Decide: **narrowing** (**`TH-05`**): full-string parses as **`XelpParseNum`** → INT truth table; **else STR** (**empty vs non-empty**). |
| **String `"0"` falsy alongside `""`** | PHP, Perl | If you imitate this globally, **`_truthy`** must special-case **`"0"`** and **`"false"`/`"no"`/`"off"` stay truthy unless you widen** ugly—usually **explicit helpers** beats magic. |

**Suggested extra cases**

- **`TR-NUMSTR-001`**: `"00"`, `"0x00"`, `"+0"`, whitespace-padded **`" 0"`** (`XelpParseNum` behavior).
- **`TR-NUMSTR-002`**: `"0"` with **narrowing OFF** → expect **truthy nonempty string** vs **narrowing ON** → **falsy** (document one).

### B. **Whitespace and “almost empty” strings**

| Trap | Example | Languages |
|------|---------|-----------|
| **`" "`**, tabs, NBSP semantics | **`" "`**: Perl historically **truthy** (≠ `""` / `"0"`); JS **`" "` truthy** | Pin **`TH‑04`** (trim or no trim). NBSP (**`\u00A0`**) if UTF-8 path exists. |

Suggested: **`TR-STRWS-001`…** space-only, tab-only, mixed `\r\n`.

### C. **`||` defaulting loses valid `0` or `""`**

| Trap | JS **`a || b`** / Python **`or`** — “first truthy” hides valid **`0`** (JS **`||`**) etc. | Xelp: no implicit **`||` truthiness`; if **`_orelse`**-style builtins appear later, give them explicit coercion rules separate from **`_truthy`**. |

### D. **`indexOf` −1 truthy**

| Trap | JS **`-1` truthy** (e.g. `indexOf` miss) | If a builtin returns sentinel **−1**, document “do not feed raw into **`_if`**”—compare explicitly. (**API posture**, mostly outside **`_truthy`**) |

### E. **`NaN`** (float path)

| Trap | **`NaN` falsy** in JS boolean context | If Xelp grows **FLT**: define **`nan`/`inf`** explicitly or forbid in **`_truthy`**. |

### F. **`[]` / `{}` containers**

| Trap | **`[]`** truthy in JS/Python/ruby sense varies | With **typed cells**: introduce **LIST**/`MAP` deliberately or **reject** unless empty-container rule specified. MVP = **none** ⇒ no trap surface. |

### G. **`NULL` / uninitialized / MISSING (3VL)**

| Trap | **`NULL`** in SQL—not false | Map to **`NIL`/`ERR`/`unset` policies** (**`TH-06`**): **prefer error** vs **silent falsy**, never **silent truthy**.

### H. **`==` vs truthiness diverge**

| Trap | **`'0' == True`** false in Python while **`bool('0')` true | Disambiguate **`_truthy`** vs **`_eq`** (**`TR-CMP-001`** line already hints). **`_eq` never double-duties coercion for `_if`** without documenting. |

### I. **`document.all` / host objects**

Trap: legacy **“falsy object”**. **Irrelevant** unless Xelp grows host objects-as-values—don't.

### J. **Word strings: `"false"`, `"true"`, `"null"`**

Trap: **All truthy strings** except special cases above in **Perl/Python** norms | If users want **`"false"`** falsy ⇒ **`_strcasecmp`** / **`_streq`** explicit—**don't smuggle keyword parsing into **`_truthy`** without a numbered row.

---

## 3. Suggested **`TR-*`** basket (beyond current requirements table)

Grow tests as **`TR-CAT-SEQ`** numbering:

**INT lane:** `−0` if distinguishable (`int` ambiguous), **`INT_MIN`**, **`INT_MAX`**, overflow parse rejection.

**STR lane:** `""`, `"0"`, `"00"`, `"0x0"`, `" \t\r\n"`, `"false"`, `"True"`, **UTF‑8 BOM-prefixed** string if handled.

**Cross:** result of **`_truthy`** == result of **`_if (_truthy x) label`** tautology.

**ERR lane:** **`(cmd_that_errors)` inside `_if` predicate slot** per **`TH-07`/EH.**

**NIL/unset lane:** **`$unset_var`** whichever semantics **`TH-06`** freezes.

---

## 4. Xelp design takeaway (recommended default posture)

**Narrow coercion surface**:

1. **`_truthy`** is **the sole** coercion entry for **`_if`** branching.

2. **INT / STR / NIL / BOOL / ERR**: orthogonal branches spelled out as a truth table + tests (not folklore in comments).

3. **Optional numeric narrowing** (**`TH-05`**): **global**, **tests named per profile**, not “sometimes `atoi` vibes”.

4. English words (`"false"`, `"yes"`) → **`_streq`** / **`_casecmp`** (or explicit predicates)—**not** silently folded into **`_truthy`**.

Ship this file beside **`xelp_unit_tests`** when script lands (**JumpBug**) so reviews grep **`TR-*` IDs** unchanged.

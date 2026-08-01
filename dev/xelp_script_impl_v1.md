# XELP Script — illustrative implementation sketch (v1)

**Status:** design fiction for clarity — **not** committed API or file layout.

This document walks through **one plausible** way Xelp Script **could** hang off today’s **`XelpParse`** / **`argc`,`argv`** world: how a statement turns into evaluations, how **C** invokes script, how the **interactive CLI** runs the same path, and how **values** bounce between **nested `( … )`**, **`_set`**, and **C callers**.

Code is **pseudocode** only (readable types, abbreviated error handling).

---

## 1. Possible implementation — big picture

**Idea:** keep **one** outward command surface, and add a thin **script engine** on each `Xelp` instance.

* **Reuse** — `XelpParse` / `XelpTokLineXB` continues to split **`#`**, **`;`/newline**, and deliver logical lines; argv tokenization stays ROM-safe (`_xelpBuf2Argv`-class behavior).

* **pure CLI path** — one command, no nested parentheses (e.g. `led 1`); existing tokenizer + `_xelpBuf2Argv`-style argv is enough.

* **script path** — a **scanner** emits **words** (literals, `$x`, `@1`, balanced `( … )` spans), then an **evaluator** walks arguments left-to-right, recursing into `( … )` for **typed value** slots.

State lives in fixed buffers inside the instance (no malloc):

```
struct ScriptCtx {
    XelpValue   stack[ SCRIPT_VALUE_STACK_MAX ];   /* temporary nested results */
    int         stack_depth;
    Frame       frames[ SCRIPT_FRAME_MAX ];        /* scopes / @ bindings / call depth */
    int         frame_idx;
    uchar       arena[ SCRIPT_ARENA_SZ ];         /* vars, copied proc bodies, strings */
    int         arena_used;
    ushort      pc;                               /* optional: label / line ptr */
};
```

Registers `mR[0]` conventionally mirror execution status (`XELPRESULT`); the **typed result** of a `( … )` call fits better in **`ScriptCtx.last_value`** or **`XelpCall.return_value`** (see proposals). Pick one coherent ABI rather than overloading **`mR[1]`** alone.

---

## 2. How simple statements work (no nesting)

Author types or ships ROM string:

```
read_adc 0
```

### 2.1 Interpreter steps (pseudocode)

```
function DispatchStatement(ths, line):
    argc, argv = TokenizeArgv(ths, line)       /* reuse fixed scratch; same tokens as CLI */
    if argc == 0: return OK

    /* First word = verb */
    verb = argv[0]

    if IsLanguageBuiltin(verb):                /* "_set", "_if", "_goto", ... */
        return RunBuiltin(ths, argc, argv)
    elif LookupUserScriptProc(ths, verb):
        return RunScriptProc(ths, verb, argv[1..argc-1])
    else:
        return RunCCommand(ths, verb, argc, argv)
```

`TokenizeArgv` behaves like today's `XelpParseXB` path: quotes + escapes produce null-terminated tokens in argv.

`RunCCommand` aligns with **`XelpCall`** in proposals: `my_cmd(Xelp *ths, XelpCall *call)` with argv, optional raw `(line,len)`, flags (CLI vs script), plus helpers **`XelpReturnInt(ths, call, …)`**.

Example handler (pseudo-C):

```c
int cmd_led(Xelp *ths, XelpCall *call) {
    int led_id, pct;
    XelpArgvInt(XelpCallArgv(call), XelpCallArgc(call), 1, &led_id);  /* illustrative */
    XelpReturnInt(ths, call, 42);   /* fills script-visible value for (...) */
    return XELP_S_OK;
}
```

For a plain **`led 1`** statement (not inside `( … )`), **`XelpReturn*`** may be ignored by outer script glue unless you define implicit “expression statement” semantics.

---

## 3. Writing Xelp Script — **`_set`**, nested **`( … )`**, and what the interpreter does

### 3.1 Example script source (ROM literal)

```
_set x (+ 2 3)
```

### 3.2 Phase A — tokenizer / scanner (conceptual)

Not the full **`XelpTokLineXB`** PSM pasted here — think **second scanner** atop the **logical line**:

```
words = ScanStatementWords(line):
    emits: "_set", "x", "(", "+", "2", "3", ")"   /* flattened for illustration */
real impl: '(' ... ')' is ONE nested span or recursive structure
```

For clarity, pretend **`ParseParenExpr`** parses balanced parens:

```
ParseParenExpr(chars) → (inner_cmd_words[])
```

### 3.3 Phase B — evaluator for `_set`

Pseudocode for **`_set name valueExpr...`** (**value** maybe multiple words composing one value — simplistic v1 **`_set x (+ 2 3)`**):

```
function Builtin_Set(ths, argc, argv):
    if argc < 3: return ERR
    name = argv[1]                         /* literal token "x" after scan rules */
    value = EvaluateValueTail(ths, argv, 2, argc)
    EnvPut(ths, name, value)               /* typed cell in arena */
    return OK
```

**`EvaluateValueTail`** walks from index **`2`** to **`argc‑1`** **— for this example ONE composite value**:

```
function EvaluateValueTail(ths, argv, lo, argc):
    if lo >= argc: return NIL
    if argv[lo] == "(" token group:
         return EvaluateParenCall(ths, argv, lo)
    elseif IsLiteral(argv[lo]): ...
```

### 3.4 Recursive nested call **`(+ 2 3)`**

```
function EvaluateParenCall(ths, argv, idx):
    inner = ExtractBalancedArgvSlice(argv, idx)     /* hypothetical slice */
    /* inner ~ [ '+', '2', '3' ] */
    return DispatchValueCall(ths, inner.argc, inner.argv)

function DispatchValueCall(ths, argc, argv):
    verb = argv[0]                                    /* "+" or "_add" */
    FoldArgsLeftToRight:
        vals[i] = CoerceLeafOrRecurse(...)
    handler = LookupBuiltinOrC(verb)
    result_cell = InvokeForValue(handler, ...)        /* fills XelpValue */
    StoreScriptReturn(ths, result_cell);             /* where ( ) semantics read it */
    return result_cell.type == ERR ? propagate : OK
```

**`(+ 2 3)`**:

1. Coerce **`"2"`** → **INT** `2`; **`"3"`** → **INT** `3`.
2. **`_add` implementation** computes **5**.
3. **`StoreScriptReturn`**: **`XelpValue{INT, 5}`** visible to **`_set`**.

 **`_set`** stores **`INT 5`** in **`$x`**.

---

## 4. Calling Xelp Script from **C**

### 4.1 Run a ROM buffer (batch)

```
const char FACTORY[] =
    "_set taps 12\n"
    "calibrate $taps\n";          /* hypothetical user C verb */

function OnFactoryButtonGPIO():
    XelpScriptRun(&cli, FACTORY, sizeof(FACTORY)-1);   /* name TBD */
```

**`XelpScriptRun`** (pseudo):

```
function XelpScriptRun(ths, buf, len):
    XB = InitXelpBuf(buf, len)
    while NextLogicalLine(ths, &XB, &line):
        DispatchStatement(ths, line)               /* §2 pipeline */
        if EngineHalted(ths): break
```

### 4.2 C invokes one expression-as-value (**optional ergonomic**)

Sometimes **C wants the script return**:

```
int n;
XelpScriptEvalExpr(&cli, "(read_sensor 3)", len, &n);   /* INT into C int */
```

Internal sketch:

```
function XelpScriptEvalExpr(ths, source, len, out_int):
    IsolateOneStatement(source, len)
    cell = EvaluateParenWrappedOrWhole(ths, source)
    if cell.type != INT: return WRONGTYPE
    *out_int = cell.int_val
    return OK
```

(Real ABI might use **`call->regs`** / **`mR[1]`** instead — requirements **allow** (**`R‑04`**)).

---

## 5. Interactive CLI (**same verbs**)

User at serial:

```
xelp> _set gain 70
xelp> read_adc $gain
```

**Path:**

```
XelpParseKey(ths, ch) collects line until ENTER
OnEnter:
    line_buf = CmdBufferSnapshot(ths)
    DispatchStatement(ths, line_buf)      /* SAME as XelpScriptRun inner loop */
```

**No silent second interpreter** unless you choose that product split (**requirements discourage it** (**`N‑02`**, **`C‑04`**).

---

## 6. Returning values — **four audiences**

### Table (who reads what)

| Path | Carrier | Typical consumer |
|------|---------|------------------|
| C execution status | `XELPRESULT` + **`mR[0]`** | C polls last command outcome |
| `( … )` typed value | `XelpValue` on `call` **or** `ScriptCtx.last_value` | enclosing `_set`, nested `(+ …)`, `_truthy`, etc. |
| Human-visible text | `XelpOut(ths, "…")` | UART/BLE (**not** synonymous with script return values) |
| Optional int slots | **`mR[1..3]`** | ergonomic C shorthand after verbs |

### 6.1 **`(nested)` error propagation (sketch)**

```
function DispatchValueCall(...):
    st = InvokeForValue(...)
    if st < 0 AND policy == HARD_STOP:
         ScriptAbort(ths)
         return st
    if st < 0 AND policy == SOFT_FALSE:
         return XelpValue{BOOL, false, warn_flag}
```

Policy belongs to **`TH‑07`** / **`EH‑03`** tiers.

---

## 7. End-to-end nano trace (pseudo timeline)

Script:

```
_set mask (_band (_read_reg 10) 0xff)
_apply $mask
```

| Step | Engine state (abstract) |
|------|-------------------------|
| 1 | Tokenize `_set mask (_band …)` into argv; parentheses become **nested slices** or an explicit sub-AST (flattened depiction here only). |
| 2 | `EvaluateValueTail` sees grouped `(_band …)`; recurse. |
| 3 | Inner `(_read_reg 10)` runs as value call → hypothetical INT **`0xAB12`**. |
| 4 | `_band 0xAB12 0xff` yields INT **`0x12`**; propagate up to enclosing `_set`. |
| 5 | `_set` binds variable **`mask` → typed INT 0x12** in arena. |
| 6 | Line `_apply $mask`: `$mask` resolves to typed cell; **`apply`'s argv[1]** may be canonical string `"18"` vs raw `0x12` encoding — implementation must freeze stringification rules (`XelpArgvInt`, etc.). |

The last bullet is deliberate tension: **`$`** expansion for C argv should match **`XelpParseNum`** parity with CLI—document once, test with **`TR-*`**.

## 8. Closing reminder

This file is **behavioral scaffolding**. Final names (**`XelpScriptRun`** vs extending **`XelpParse`**), **`XelpCall`** fields, **`_truthy`**, **`_goto`**, arena layout — lock against **`dev/xelp_script_requirements.md`** plus the grammar annex. When implementing, promote fragile examples into **`TR-*`** fixtures.

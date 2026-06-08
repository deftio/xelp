# Xelp Script Redesign Discussion - 2026-05-30

Status: **Brainstorming / Nothing decided**

- **Current version**: 0.4.1 (`0x00000401` in `src/xelp.h`)
- **Branch**: `exp_script_revisions`

This captures an ongoing design discussion about what a cleaner xelp script engine could look like, starting from the current experimental `exp_argv_core` branch. The current implementation works but the question is: could we do better with a fresh start, keeping the same constraints?

## Constraints (non-negotiable)

- < 10KB compiled on 32-bit (preferably < 10KB on 16/8-bit)
- Instance-based, no globals
- No malloc, no stdlib
- C89/C90 in src/
- Builds on top of the existing CLI (tokenizer, command dispatch, line editing)
- Must work from ATtiny85 (512B RAM) through ARM64
- "Drop it in and leave it" -- ROM-resident, useful for bringup, debugging, teaching

## What the current implementation gets right

- Instance-based, no globals
- Builds on the CLI tokenizer/dispatcher -- script is an extension of CLI, not separate
- ROM-resident script functions (const string bodies in flash, don't consume arena RAM)
- Breakpoint callback for safety (instruction budget, watchdog, yield)
- `#` comments handled at the PSM/tokenizer level (first-class, not bolted on)
- Iterative eval loop (no C recursion for function calls) -- good for stack-constrained targets
- `_if ... _then ... _next` pattern gives block-conditional semantics without block syntax
- `_` prefix for builtins avoids namespace collision with user commands
- `$var` and `@n` expansion before dispatch -- builtins see resolved strings

## Core design issues identified

### 1. Text-substitution expression evaluation

The paren expression model `(_add $a (_mul $b $c))` works but the implementation evaluates inner expressions, converts results back to text, splices text into the command buffer, and re-tokenizes. This creates ~1.3KB of C stack locals in `_xelpEvalStatement` (5 buffers of XELP_ARGVBUFSZ). Contradicts the 8-bit target goal.

**The concept is right** (parens are readable and should stay), **the mechanism is wrong** (text substitution is the expensive part, not the function call).

### 2. Result stack interleaved with call frames

Results and call frames share one upward-growing stack in the arena. `_xelpResultPop` must walk the entire stack from base to find the last non-frame entry -- O(n). The interleaving creates complexity in frame push/pop and result cleanup.

### 3. Arena fragmentation on variable type/size changes

Variable heap grows downward. Overwriting a variable with a different size creates dead space that is never reclaimed. INT-to-INT same-size overwrite has a fast path, but any size change wastes space permanently. Long-running scripts on non-reflashable devices will eventually hit ARENA_FULL with few live variables.

### 4. Monolithic builtin set

`XELP_ENABLE_SCRIPT` gates all 33 builtins. No way to include just the core set (_set, _if, _goto, _print) without the bitwise/comparison/etc. operators.

## Key design tensions under discussion

### Parens: keep or drop?

- **Keep**: `_set area (_mul (_add $w 1) $h)` is readable and shows intent in one place. The target audience (embedded engineers) finds this natural.
- **Drop**: Eliminates result stack, paren machinery, splice buffers. Imperative style with explicit temporaries is also natural to C programmers.
- **Current leaning**: Keep parens, but fix the evaluation mechanism.

### Paren evaluation mechanism

The paren prepass that converts `(foo` to `( foo` is fine -- it's just a string operation that lets the existing tokenizer work. An interesting reframing: treat `(` as a builtin function call `_(`. Now it looks like any other builtin (starts with `_`), and it can do whatever it wants with its arguments, including recursive evaluation. The key insight: **`(` is really just a function call**.

### Return values: what can functions return?

This is the central unsolved problem. Current options discussed:

**Option A: Functions only return XELPRESULT (int status code)**
- Very C-like. Data passes through side channels (variables, registers).
- Pro: Simple, eliminates result stack.
- Con: Can't compose expressions. `_set x (myFunc 3)` doesn't work if myFunc only returns OK/ERR.

**Option B: Single result register (mR[0])**
- Builtins and script functions write result to mR[0].
- Pro: Simple, fast, no stack needed for linear call chains.
- Con: Nested calls clobber R0. Need to save/restore manually via variables. mR[] is int-only -- returning strings requires a different mechanism. Making mR[] hold both ints and pointers is a bad idea (how does xelp know which is which?).

**Option C: Typed result stack**
- Functions push typed results (INT or STR). Callers pop.
- Pro: Supports composition, handles both types.
- Con: Who cleans up unused results? Need clear rules for when results are consumed vs discarded. Need to distinguish "function return value" from "variable passed back". Adds complexity.

**Option D: Mailbox / named result slots**
- `setMailbox(name, type, value)` / `getMailbox(name)` -- explicit named channels.
- Pro: Clear ownership, named for readability.
- Con: Might just be reinventing variables. Is this different from `_set`/`$var`?

**Option E: Hybrid -- mR[0] for ints, convention for strings**
- Return int values through R0 (covers 90% of embedded use cases).
- String returns go through a designated variable or output buffer.
- Pro: Simple for the common case.
- Con: Two mechanisms, asymmetric.

### The "unused result" problem

If a function pushes a result and the caller doesn't consume it, who cleans up? Current implementation does `mSP = mArena` after each top-level CLI statement (brute force reset). Need a grammar-level convention that makes it obvious when results are consumed vs discarded.

### The CLI nature of the grammar

The fundamental grammar is `command arg arg ...` where everything is strings. This is a CLI mechanism -- both C functions and xelp functions receive `(ths, argc, argv)`. Things that aren't string convention need sigils:
- `$var` -- variable reference (expand before dispatch)
- `@n` -- positional parameter (expand before dispatch)
- `(expr)` -- sub-expression (evaluate and substitute)

Since C functions return an int (XELPRESULT), that's the "native" return type of this grammar. Returning non-int values requires something beyond the C convention. This is where the design gets hard -- xelp needs a mechanism that C doesn't naturally have (returning typed values), but it must stay simple and small.

### Variable storage: fixed slots vs arena

- **Fixed slots**: Predictable, no fragmentation, but wastes RAM on small targets if most slots unused. Bad for ATtiny85.
- **Arena heap (current)**: Memory-efficient packing, but fragments on type/size changes. Good for small targets when variables are stable, bad for long-running scripts that reassign.
- **Arena + compaction**: Keep dense packing, add compact-on-pressure. Costs code size but self-heals. Compaction rarely executes (only on ARENA_FULL).

### Result type: `[type, size, bytes]` (TLV)

The result construct should be `[type, size, bytes]` -- a TLV encoding. This is extensible (new types don't change the structure), self-describing, and works at any depth. This is what the current result stack entries already use (`[kind:1][payload...]`). The question isn't the encoding -- it's where results live and how they flow between producer and consumer.

### Key insight: only one result is ever pending

Sequential evaluation guarantees that at most one result is unconsumed at any point. Even with deeply nested parens like `(_add (foo 10) (bar 5))`, the evaluation order is:
1. Evaluate `(foo 10)` → result consumed immediately (written to argv slot)
2. Evaluate `(bar 5)` → result consumed immediately
3. Evaluate `_add` with both args already in argv

This holds at any depth. A "result stack" would only ever reach depth 1. This suggests a single result slot might be sufficient -- not as an arbitrary limit, but as a consequence of the evaluation semantics. However, the concern is forward-thinking: what if future features require multiple pending results? TBD.

### `_func` / `_funcend` as semantic boundary

**Key realization**: `_funcend` isn't syntactic sugar -- it's a critical semantic marker that solves multiple problems:

1. **Frame boundary**: Explicitly marks where local variables and stack frames are cleaned up. Without it, the system guesses or the programmer manually cleans up.

2. **CLI authoring mode**: `_func name` (without inline body) puts the CLI into **accumulation mode** -- lines are buffered, not executed or expanded. `$var` and `@n` stay as literal text (definition time vs call time distinction). `_funcend` exits the mode, wraps the buffer into a PROC entry, stores it in the arena.

3. **Parser clarity**: Both for the tokenizer and for human readers, `_funcend` removes ambiguity about where the function body ends. The current inline-string form `_func name "body"` works for one-liners and ROM strings, but multi-line functions in quoted strings are fragile and unreadable.

**Both forms can coexist**: `_func name "body"` for one-liners / C ROM strings. `_func name` + lines + `_funcend` for multi-line definitions at the CLI or in scripts. The parser checks: does the `_func` line have a body argument? If yes, inline form. If no, accumulation mode.

**Nesting**: For v1, nested function definitions (defining a function inside a function body) should error during accumulation. Could track `_func`/`_funcend` depth in the future if needed.

**Connection to returns**: With explicit `_funcend`, `_return` has a clear meaning: write the result value (TLV), then exit to the `_funcend` boundary. No `_return` → function returns `XELP_S_OK` with no value result.

## Current consensus (tentative)

**Seems decided:**
- Parens stay as syntax; evaluation mechanism needs to be cheaper (avoid text substitution / excessive C stack buffers)
- `_func`/`_funcend` is the right approach for function definition (semantic boundary, CLI accumulation mode, scope clarity)
- Functions return XELPRESULT (int status code); data flows through other channels
- The CLI grammar (`command arg arg ...`) is the foundation; script extends it, not replaces it
- `#` comments, `$` expansion, `_` prefix builtins — all correct and staying
- The paren prepass (`(foo` → `( foo`) is fine — it's just a string op that lets the tokenizer work

**Still open:**
- Types: explicit (BASIC-style), inferred (current), or untyped (Tcl/shell "everything is a string")?
- How non-int values pass between functions (result stack, register, single slot, mailbox, variables?)
- Control flow grammar (`_if ... _then cmd` vs block-structured `_if`/`_endif` vs other)
- Result mechanism and the "unused result" cleanup problem
- Arena fragmentation strategy (compaction? fixed slots? accept it?)
- Whether `$n` should unify positional params and variable expansion (replacing `@n`)
- Builtin layering (sub-feature gates for math, bitwise, comparison)
- Computational model: what tradition does xelp script commit to? (shell, Tcl, BASIC, hybrid?)

**Design tension acknowledged:**
- The CLI is stateless between lines; scripts need structure (control flow, blocks, scope). These are fundamentally different and can't be fully unified. The script layer must add concepts the CLI doesn't have, but should do so in a way that feels natural alongside the CLI grammar.
- BASIC is a useful analogy for what xelp could be: ROM-resident, interactive + scriptable, simple, explicitly typed, powerful enough for real work, small enough for 8-bit.
- "Everything is a string" eliminates type complexity but introduces comparison ambiguity and loses the ability to be a "mini general purpose language."

## Call / Variable Interaction Matrix

This matrix examines how functions call each other and how typed values flow across all four combinations. Goal: find asymmetries, missing mechanisms, and design the uniform rules.

**Notation**: "C func" = a C function registered with xelp. "Xelp func" = a script function defined with `_func`/`_funcend`.

### How calls are made

| Caller → Callee | Current mechanism | Callee sees |
|---|---|---|
| **C → C** | Normal C call (not xelp's concern) | C args |
| **C → Xelp func** | `XelpCallProc(ths, "funcname arg1 arg2")` | `@1`=`"arg1"`, `@2`=`"arg2"` (strings) |
| **Xelp → C func** | `mycfunc arg1 arg2` in script | `argc`, `argv[]` (strings) |
| **Xelp → Xelp func** | `myfunc arg1 arg2` in script | `@1`=`"arg1"`, `@2`=`"arg2"` (strings) |

**Observations:**
- Xelp→C and Xelp→Xelp look identical at the call site. Good — caller doesn't know or care what the callee is. Dispatch figures it out.
- C→Xelp requires packing args into a single string for re-tokenization. Slightly awkward but workable. Alternative: `XelpCallProcArgv(ths, "funcname", argc, argv)` to avoid re-tokenization?
- All callees receive string arguments. This is the CLI model. Consistent.
- **Gap**: C→Xelp can only pass strings. No way to pass a typed int without converting to string first. Is this a problem? Probably not — the string gets parsed back to int if needed. Round-trip cost is negligible for the use case.

### How arguments are received

| Callee type | Access mechanism | Types available |
|---|---|---|
| **C func** | `argc`, `argv[]` (standard C) | Strings only. Parse with `XelpParseNum()` etc. |
| **Xelp func** | `@1`, `@2`, ... (or `$1`, `$2` if unified) | Strings only. Parse happens at point of use (e.g., `_add @1 5`). |

**Observations:**
- Symmetric. Both receive strings. Both parse as needed. Good.
- The `@n` vs `$n` question: if positional params used `$1`, `$2`, then variable and param access are unified under `$`. A var named `1` would conflict, but numeric-only names could be reserved for params.

### How local variables are created

| Context | Create int var | Create string var | Scope |
|---|---|---|---|
| **C func** | Normal C local `int x = 42;` | Normal C local `char s[]` | C stack (automatic) |
| **C func via xelp** | `_xelpVarSet(ths, ...)` or `XelpSetVar(ths,...)` | Same API | Arena heap — **global** (persists after return) |
| **Xelp func** | `_set x 42` | `_set x "hello"` | Arena heap — **global** (persists after return) |
| **Xelp root (CLI)** | `_set x 42` | `_set x "hello"` | Arena heap — **global** |

**Observations:**
- **Major gap**: There are no local variables in xelp. Every `_set` in a function pollutes the global namespace. A function that uses `_set i 0` for a loop counter clobbers any `$i` the caller was using.
- C functions have true locals (C stack) AND can create xelp globals. Xelp functions can ONLY create globals.
- `_func`/`_funcend` gives us a scope boundary. Could add `_local x 42` that allocates in the frame (cleaned up on `_funcend`/`_return`). Or: all `_set` inside a `_func`/`_funcend` block is local by default, with `_global x 42` to opt out.
- **Question**: Should "global vars are just local vars at root context" mean that root-level `_set` and function-level `_set` behave identically, but function-level vars are cleaned up at `_funcend`?

### How values are returned

| Callee type | Return status (int) | Return typed value (int) | Return typed value (string) | Caller retrieval |
|---|---|---|---|---|
| **C func** | `return XELP_S_OK;` | `XelpSetResultInt(ths, val)` | `XelpSetResultStr(ths, s, len)` | Script: via `()` paren expansion. C: `XelpGetResult(ths, &r)` |
| **Xelp func** | Implicit (`_funcend` reached) | `_return 42` | `_return "hello"` | Script: via `()` paren expansion. C: `XelpGetResult(ths, &r)` |

**Observations:**
- Status return is clean in both cases. XELPRESULT is always an int. Consistent with C.
- **Typed value return is the messy part.** C funcs use `XelpSetResultInt/Str` (push to result stack). Xelp funcs use `_return val`. Both go through the result stack. The result stack is where the warts live.
- **The two-channel problem**: A function returns BOTH a status (XELPRESULT) AND optionally a value (via result stack). These are independent channels. `_return 42` sets the value but the status is always XELP_S_OK. What if a function needs to return an error AND a value (e.g., partial result)? Currently can't.
- **Paren expansion** is the primary way script code consumes return values: `_set x (myfunc 3)`. This works for both C and Xelp callees — the caller doesn't know which type it called.
- **Gap**: If a C func or Xelp func returns a value but the caller doesn't use `()`, the value sits on the result stack. Current cleanup: `mSP = mArena` after top-level statement. This is the "unused result" problem.

### How values are passed between functions (non-argument, non-return)

| Mechanism | Direction | Types | Scope | Available to |
|---|---|---|---|---|
| **xelp variables** (`_set`/`$var`) | Bidirectional | INT, STR (current) | Global | Both C and Xelp |
| **mR[] registers** | Bidirectional | INT only | Global | Both C and Xelp |
| **Output** (`XelpOut`/`_print`) | One-way (out) | String | N/A | Both (side effect) |
| **Result stack** | Callee→Caller | INT, STR | Per-statement | Both C and Xelp |

**Observations:**
- Variables are the most flexible channel but lack scoping and have fragmentation issues.
- mR[] is fast but int-only and small (XELP_REGS_SZ entries).
- The result stack is the ONLY mechanism for returning typed values from `()` expressions. Everything else requires the caller to know a variable name or register index by convention.
- **Missing**: No way to pass a struct/buffer/byte-array. For embedded use (reading sensor data, register blocks), this matters. Could a variable hold raw bytes? The TLV encoding (`[type, size, bytes]`) supports it in principle.

### Summary of asymmetries and gaps

| Issue | Description |
|---|---|
| **No local vars** | All xelp variables are global. Functions clobber each other's state. |
| **Two return channels** | Status (XELPRESULT) and value (result stack) are independent. Confusing semantics — what does "function succeeded but returned no value" look like vs "function succeeded and returned 0"? |
| **Unused result cleanup** | Brute-force `mSP = mArena` after each top-level statement. No per-function cleanup. |
| **C→Xelp arg passing** | Must pack into a string. Minor friction but acceptable. |
| **No typed value passing except result stack** | Variables are the only alternative, but they're global and named-by-convention. No formal "out parameter" mechanism. |
| **Return type ambiguity** | `_return 42` — is that int 42 or string "42"? Type inference decides. Caller may not know. |
| **Paren-only value consumption** | `_set x (func)` works. `func; _set x $???` doesn't — there's no way to get the last return value without parens (except mR[0] for C funcs). |

### Design questions exposed by this matrix

1. **Local variables**: Should `_set` inside `_func`/`_funcend` be local by default? If so, how are locals stored — in the frame? In the arena with a scope marker?
2. **Unified return mechanism**: Should there be ONE way to return a value, not two (status + result stack)? Could XELPRESULT itself be typed — a tagged union of status/int/string?
3. **Last-result access**: Should there be a way to access the last return value without parens? Like `$_` or `$R` — a pseudo-variable that holds the last returned value?
4. **Out parameters**: Should functions be able to write to a caller-named variable? Like `myfunc result_var 42` where the function does `_set @1 computed_value`? (This already works since vars are global, but it's by accident, not by design.)
5. **Value identity**: When `_return 42` is called, should the system store int 42 or string "42"? If typed, how does the caller know what type to expect?

## Lazy Argv Accessor Model

Instead of upfront parsing into argc/argv arrays, use accessor functions:

- `XelpArgc(ths)` — returns argument count (triggers tokenization if needed)
- `XelpArgv(ths, n)` — returns pointer to arg n (tokenizes on demand up to n, caches)
- `XelpArgvInt(ths, n, &val)` — parse arg n as int (bounds-checked)
- `XelpArgvStr(ths, n, &s, &len)` — get arg n as pointer+length

**Key properties:**
- Handler signature simplifies to `XELPRESULT fn(XELP *ths)` — no argc/argv params
- Lazy: no parsing until someone asks. No-arg commands pay zero tokenization cost.
- Arena-backed: no hard limit on argc (limited by arena size, not XELP_ARGV_MAX)
- Bounds-checked: accessor validates n < argc automatically
- Frame-aware: each call frame has its own line context. Frame push saves/resets, frame pop restores.
- Comparison to prior approaches:
  - 0.3.3 getTokN: re-scanned from start each call (O(n) per access). No copy, no limit, but redundant work.
  - 0.4.0 upfront argv: full parse+copy before dispatch. O(1) access but fixed XELP_ARGV_MAX=8 and always pays full cost.
  - Lazy accessor: parse-on-demand, cache in arena. O(1) amortized, no fixed limit, zero cost if unused.

## Frame-Based Scope and Return Values (Key Insight)

**Core principle: every frame owns everything inside it. When the frame exits, it's all gone. `_return` is the only way to promote a value to the parent frame.**

### Frame contents (all in arena, growable):
1. Saved caller context (script pointers, parent frame boundary)
2. Argv data (arguments this function was called with)
3. Local variables (created with `_set` during execution)
4. Values received from callee returns (consumed immediately or captured with `_set`)

### Lifecycle:
- `_set x 42` inside a function → `$x` is local to that frame
- `_return $x` → value of `$x` is promoted to the parent's frame; current frame is then destroyed
- If the parent doesn't capture or re-return the value, it dies when the parent's frame exits
- Root frame (CLI) never exits → its locals persist (appear "global")

### Example:
```
_func grandparent
  _set x (parent 10)     # parent returns 42, $x is local here
  _print $x              # works — $x is in this frame
_funcend                  # $x cleaned up with frame

_func parent
  _set a (child @1)      # child returns 20, $a is local here
  _set b (_mul $a 2)     # $b is local here
  _return $b             # 40 promoted to grandparent's frame
_funcend                  # $a, $b cleaned up with frame

_func child
  _return (_mul @1 2)    # promoted to parent's frame
_funcend
```

### Why this works:
- **No garbage collection**: frame exit = rewind arena SP to frame boundary
- **No "unused result" problem**: uncaptured returns die with the frame, naturally
- **No separate result stack**: returns land in the parent's frame region, same as locals
- **Uniform rule**: args, locals, and received returns all have frame lifetime
- **C-like**: mirrors how the C stack works (locals die on return, caller must capture return value)

### Resolved sub-questions:
- **Variable lookup scope**: Current frame only. No inherited environment. If a child needs a value, pass it as an argument. Clean isolation.
- **Multiple returns**: Yes. `_return $a $b 39` returns multiple typed values. See "Type System and Arrays" below.

### Remaining sub-questions:
- **Return value landing zone**: When `(func args)` evaluates, the return value must briefly exist to be written into the argv slot. Where exactly? Directly into argv expansion? Temporary in parent frame?

## Type System and Arrays (Decided: IN)

**Types: INT, STR, ARRAY.** Three types. No feature gate — arrays are core to the type system for consistency.

BASIC had arrays on 8-bit MCUs with 2KB RAM in the 1980s. If that worked, this works.

### Why arrays

Arrays solve the multiple-return-value problem cleanly:
- Every function return is implicitly an array (single return = length 1, no return = length 0)
- The caller decides how to consume: destructure into named vars, capture whole array, or take first element
- No name collisions — caller controls names
- No "unused result" problem — uncaptured elements are discarded
- Also useful as a general-purpose data structure: sensor readings, register blocks, calibration tables

### TLV encoding

Each value (in variables, in arrays, in returns) uses TLV: `[type:1][size:2][bytes:size]`
- INT: `[0x01][0x04 0x00][4 bytes]`
- STR: `[0x02][len_lo len_hi][string bytes]`
- ARRAY: `[0x03][total_size_lo total_size_hi][count:2][elem0:TLV][elem1:TLV]...`

Arrays are mixed-type — each element is independently INT or STR (or nested ARRAY if needed later).

### Syntax

Creation / capture:
```
_return $a $b 39                  # function returns 3 values (mixed types ok)
_set x (func)                     # capture first return value into $x
_set x y z (func)                 # destructure first 3 return values into $x $y $z
_setarr results (func)            # capture all return values as array $results
_array myarr 1 2 "three" 4       # explicit array construction
```

Access:
```
$results.0                        # first element
$results.1                        # second element
$results._len                     # element count
```

### Consumption rules (Lua model):
- More names than return values → excess names get NIL/error
- Fewer names than return values → excess values discarded
- `_setarr` captures everything regardless of count

### Arena representation:
Array variable's value region: `[count:2][TLV][TLV]...`
- Same TLV encoding used for all typed values
- Indexed access walks forward through TLV sequence to element N — O(N) but arrays are small
- Array lives in the variable's arena slot like any other value

### Cost:
- One additional type tag in the system
- Indexed access syntax (`.N`) in the expansion logic
- `_setarr` builtin and `_array` constructor
- BASIC managed arrays on 6502 (1MHz, 2KB RAM). This is tractable.

## Symmetric C / Script Interface (Proposal)

### Core principle: C and script functions are interchangeable

The caller should not know or care whether the callee is C or script. The callee should not know or care whether the caller is C or script. Same argument access, same return mechanism, same types flowing end-to-end.

### Typed arguments — no string round-trip

When the expansion layer resolves `$x` and `$x` is INT 42, it passes the typed value through, not the string "42". The accessor layer knows the type. Conversion to string or int happens on demand, at the point of use. Once a value enters the type system, it stays typed until someone explicitly needs a different representation.

### Handler signature

All handlers (C and script) use the same signature:
```c
XELPRESULT fn(XELP *ths);
```
No argc/argv parameters. Everything through accessors.

### C-side argument access — query-first pattern

The developer should never have to assume a type. Query first, decode second. Graceful failure if types don't match.

```c
XELPRESULT cmd_example(XELP *ths) {
    int argc = XelpArgc(ths);
    int i;

    for (i = 0; i < argc; i++) {
        int argtype = XelpArgType(ths, i);  /* XELP_T_INT, XELP_T_STR, XELP_T_ARRAY, XELP_T_NIL */

        switch (argtype) {
        case XELP_T_INT: {
            int val;
            XelpArgInt(ths, i, &val);       /* guaranteed to succeed — we checked type */
            break;
        }
        case XELP_T_STR: {
            const char *s; int slen;
            XelpArgStr(ths, i, &s, &slen);  /* guaranteed to succeed */
            break;
        }
        case XELP_T_ARRAY: {
            int arrlen = XelpArgArrayLen(ths, i);
            int elemtype = XelpArgArrayElemType(ths, i, 0); /* type of first element */
            break;
        }
        }
    }
    return XELP_S_OK;
}
```

### Convenience accessors with graceful fallback

For the common case where you just want an int or string and don't want to switch on types:

```c
int val;
XELPRESULT r;

r = XelpArgInt(ths, 0, &val);
/* r == XELP_S_OK: arg 0 was INT (or STR that parses as int) — val is set */
/* r == XELP_E_TYPE_ERR: arg 0 is STR that doesn't parse as int, or ARRAY */
/* r == XELP_E_ERR: arg 0 doesn't exist (out of bounds) */
```

Key: `XelpArgInt` should try to be helpful — if the arg is a STR "42", parse it and succeed. Only fail if the string genuinely isn't numeric. This means CLI-typed input (always strings) works seamlessly with typed values from script. The developer doesn't have to care WHERE the argument came from.

Similarly:
```c
const char *s; int slen;
r = XelpArgStr(ths, 0, &s, &slen);
/* Works for STR args directly */
/* For INT args: returns string representation (lazy conversion) */
/* For ARRAY args: returns XELP_E_TYPE_ERR (can't meaningfully stringify an array) */
```

### C-side return values — query first, same pattern

```c
XelpCallProc(ths, "some_func 10 20");

int n = XelpReturnCount(ths);           /* how many values returned? */
int rtype = XelpReturnType(ths, 0);     /* type of first return value */

if (rtype == XELP_T_INT) {
    int val;
    XelpReturnInt(ths, 0, &val);        /* get first return as int */
}
```

Same graceful pattern: query count, query type, decode. Never assume.

### C-side producing return values

```c
XELPRESULT cmd_read_sensors(XELP *ths) {
    XelpReturn(ths, XELP_T_INT, read_temp());
    XelpReturn(ths, XELP_T_STR, "celsius", 7);
    XelpReturn(ths, XELP_T_INT, read_pressure());
    return XELP_S_OK;  /* status is separate from return values */
}
```

### Script-side — same semantics, different syntax

```
# Receiving args (in function body):
_if (_type $0) _then ...         # query type of first arg
_set val $0                      # use arg 0

# Returning values:
_return 42 "celsius" 255         # multi-typed return

# Capturing returns:
_set temp unit pres (read_sensors)   # destructure
_setarr readings (read_sensors)      # capture all
_type $readings.0                    # query type of first element
```

### End-to-end type flow examples

**CLI user types command (strings in):**
```
read_sensors 5          # "5" arrives as STR, XelpArgInt parses it to INT 5
```

**Script calls with typed values (types preserved):**
```
_set n 5
read_sensors $n         # INT 5 arrives as INT 5, no parse needed
```

**C calls script function (types preserved):**
```c
XelpSetArgInt(ths, 5);              /* or pack into call string */
XelpCallProc(ths, "myscriptfn 5"); /* string form also works */
```

**Mixed chain (transparent):**
```
_setarr data (c_func (script_func $x) "hello")
# $x: typed INT → passed typed to script_func
# script_func returns typed → passed typed to c_func argument
# c_func returns typed array → captured as typed array in $data
# At no point did any value round-trip through string unless it started as one
```

### Key properties:
- **Query, don't assume**: Always check type before decoding. Accessor returns error on mismatch.
- **Helpful coercion**: XelpArgInt tries to parse STR → INT. XelpArgStr converts INT → string repr. Best-effort, not silent corruption.
- **Graceful failure**: Wrong type → XELP_E_TYPE_ERR, not crash. Out of bounds → XELP_E_ERR, not crash.
- **CLI compatibility**: Strings from interactive CLI work fine — they're STR values that accessors coerce as needed.
- **No mental tax**: Developer doesn't need to remember whether callee is C or script, or whether caller was CLI or script.

## Full Round-Trip Example: Typed Values Through C and Script

This traces a string, an int, and a mixed-type array through every combination of C↔script calling.

### Setup: C-side declarations

```c
/* A C function that creates typed values and calls a xelp script function */
XELPRESULT cmd_create_data(XELP *ths)
{
    XELPRESULT r;
    int rcount, rtype, ival;
    const char *sval;
    int slen;

    /*-- Step 1: Create typed values in C --*/
    /* These go into the xelp frame as typed args for the call */
    XelpCallBegin(ths, "process_data");  /* target = xelp script func */
    XelpCallArgStr(ths, "hello", 5);     /* arg 0: STR "hello" */
    XelpCallArgInt(ths, 42);             /* arg 1: INT 42 */
    XelpCallArgArrayBegin(ths);          /* arg 2: ARRAY begin */
      XelpCallArgStr(ths, "sensor1", 7); /*   elem 0: STR */
      XelpCallArgInt(ths, 99);           /*   elem 1: INT */
    XelpCallArgArrayEnd(ths);            /* arg 2: ARRAY end */
    r = XelpCallExec(ths);               /* dispatch */

    if (r != XELP_S_OK) return r;

    /*-- Step 4: C receives typed returns from xelp script func --*/
    rcount = XelpReturnCount(ths);       /* how many values came back? */

    /* Query each return — never assume types */
    rtype = XelpReturnType(ths, 0);      /* what type is return 0? */
    if (rtype == XELP_T_STR) {
        XelpReturnStr(ths, 0, &sval, &slen);  /* "HELLO" */
    }

    rtype = XelpReturnType(ths, 1);
    if (rtype == XELP_T_INT) {
        XelpReturnInt(ths, 1, &ival);    /* 84 */
    }

    rtype = XelpReturnType(ths, 2);
    if (rtype == XELP_T_ARRAY) {
        int arrlen = XelpReturnArrayLen(ths, 2);
        int elem0type = XelpReturnArrayElemType(ths, 2, 0);
        /* walk elements as needed */
    }

    return XELP_S_OK;
}
```

### Setup: Xelp script function (called from C above, calls C below)

```
# Step 2: Xelp script function receives typed args from C
_func process_data
  # Query types — $0 is STR "hello", $1 is INT 42, $2 is ARRAY ["sensor1", 99]
  _set label $0                  # STR "hello" → local STR var
  _set count $1                  # INT 42 → local INT var
  _setarr sensors $2             # ARRAY → local ARRAY var

  # Step 3: Pass typed values to a C function
  # c_transform is registered C func — receives typed args, returns typed values
  _set result_str result_int result_arr (c_transform $label $count $sensors)

  # Step 3b: return typed values back to C caller
  _return $result_str $result_int $result_arr
_funcend
```

### Setup: C function called FROM xelp (Step 3)

```c
/* C function called by xelp script — receives and returns typed values */
XELPRESULT cmd_c_transform(XELP *ths)
{
    int argc = XelpArgc(ths);
    const char *s; int slen, ival;
    XELPRESULT r;

    /* Arg 0: query and get string */
    r = XelpArgStr(ths, 0, &s, &slen);
    if (r != XELP_S_OK) return r;
    /* s = "hello", slen = 5 — arrived as typed STR, no parse needed */

    /* Arg 1: query and get int */
    r = XelpArgInt(ths, 1, &ival);
    if (r != XELP_S_OK) return r;
    /* ival = 42 — arrived as typed INT, no parse needed */

    /* Arg 2: query — it's an array */
    if (XelpArgType(ths, 2) == XELP_T_ARRAY) {
        int arrlen = XelpArgArrayLen(ths, 2);
        int elem1;
        XelpArgArrayElemInt(ths, 2, 1, &elem1);  /* elem 1 = 99 */
    }

    /* Return transformed values — typed */
    XelpReturnStr(ths, "HELLO", 5);       /* return 0: transformed string */
    XelpReturnInt(ths, ival * 2);         /* return 1: 84 */
    XelpReturnArrayBegin(ths);            /* return 2: array */
      XelpReturnStr(ths, "SENSOR1", 7);
      XelpReturnInt(ths, 198);
    XelpReturnArrayEnd(ths);

    return XELP_S_OK;
}
```

### Now the same thing from CLI (strings in):

```
> process_data hello 42 ???
```

**Problem exposed**: How does a CLI user pass an array as a typed argument? Options:
- Literal syntax: `process_data hello 42 [sensor1,99]` — needs array literal parsing
- Can't — arrays can only be created programmatically (`_array`, `_setarr`, or from C)
- CLI args are always STR. The function uses `XelpArgInt` with coercion for numbers.

This is acceptable: CLI is the string-native entry point. Types are inferred/coerced at the boundary. Once inside the type system (via `_set`, `_array`, function returns), types are preserved. The CLI is where strings enter; the script engine is where they become typed.

### Type flow summary:

```
C creates typed ──→ Xelp receives typed ──→ C receives typed ──→ Xelp receives typed ──→ C receives typed
   STR "hello"        $0 = STR "hello"       s = "hello"          $result_str = STR       sval = "HELLO"
   INT 42             $1 = INT 42            ival = 42             $result_int = INT       ival = 84
   ARRAY [STR,INT]    $2 = ARRAY             array access          $result_arr = ARRAY     array access

No string round-trips anywhere in the chain. Types preserved end-to-end.
CLI entry point is the exception: strings coerced to typed on first use.
```

## Design Exploration: Mailbox Model (explored and set aside)

We explored separating the call interface (always strings) from a data interface (typed mailbox). The idea: `command arg arg` stays pure CLI strings, typed values cross the C↔Xelp boundary through a separate named mailbox mechanism.

**Why explored**: Keeps CLI grammar truly unchanged when script is enabled. No typed argument passing complexity.

**Why set aside**: Every attempt to define the mailbox converged back to "named typed values in a scoped container" — which is exactly what frame-scoped variables already are. The mailbox was reinventing the variable system. The real use cases at the C↔Xelp boundary aren't concrete enough yet to justify a separate mechanism.

**Conclusion**: Frame-scoped typed variables with typed C accessors (the earlier proposal) solve the boundary-crossing problem without a separate mechanism. The arena frame IS the shared storage. C functions write to it via `XelpReturnInt/Str/Array`, read from it via `XelpArgInt/Str/Array`. Xelp functions use `_set`, `$var`, `_return`. Same storage, two interfaces.

## Current Design Direction (converged)

After exploring multiple approaches (result stack, registers, mailboxes, pure-string CLI), the design converges on:

### Core model: frame-scoped typed values

- **Types**: INT, STR, ARRAY — all TLV encoded in the arena
- **Storage**: All values live in frames in the arena
- **Lifetime**: Frame exit cleans everything up. No GC, no manual cleanup.
- **Promotion**: `_return` promotes values to parent frame. Only way data survives a frame exit.
- **Root frame**: Never exits. Root-level `_set` creates "global" values. CLI variables live here.

### Call interface

- **Grammar**: `command arg arg ...` — CLI grammar is the foundation
- **Parens**: `()` evaluates sub-expression, returns any type (INT, STR, or ARRAY)
- **Brackets**: `[]` constructs array literals
- **Positional params**: `@n` (kept separate from `$` to avoid ambiguity)
- **Variables**: `$name` expansion
- **Comments**: `#` to end of line
- **Labels**: `:name` for goto targets
- **Builtins**: `_` prefix

### C↔Xelp boundary

- **C accessors (args in)**: `XelpArgc(ths)`, `XelpArgType(ths, n)`, `XelpArgInt(ths, n, &val)`, `XelpArgStr(ths, n, &s, &len)` — query first, decode second, graceful errors
- **C returns (values out)**: `XelpReturnInt(ths, val)`, `XelpReturnStr(ths, s, len)`, `XelpReturnArrayBegin/End(ths)`
- **C calling xelp**: `XelpCallProc(ths, "func args")` + `XelpReturnCount/Type/Int/Str` to read results
- **Coercion**: `XelpArgInt` on a STR tries to parse it. `XelpArgStr` on an INT converts to string. CLI input (always strings) works seamlessly.

### Handler signature

```c
XELPRESULT fn(XELP *ths);   /* no argc/argv params — use accessors */
```

### Arrays (core type, not feature-gated)

- Construction: `[1 "two" 3]` or from function returns
- Access: `$arr.N` for static index, `(_get $arr $i)` for dynamic
- Length: `$arr._len` or `(_len $arr)`
- Immutable for v1 (no in-place modification or resize)
- Passed as typed arguments, returned as typed values

### Functions

- `_func name` / `_funcend` for multi-line definitions (CLI accumulation mode)
- `_func name "body"` for one-liners / ROM strings
- Frame-local variables (`_set` inside function = local to that frame)
- `_return val1 val2 ...` promotes typed values to parent frame
- Caller captures with `_set x y z (func)` or `_setarr arr (func)`

## Open questions

1. Should builtins be layered into sub-feature gates (core, math, bitwise, comparison)?
2. Is arena compaction worth the code size for the fragmentation problem?
3. What's the minimum viable script engine (which features are essential vs nice-to-have)?
4. For `_func`/`_funcend` CLI accumulation: where do lines buffer during definition? Arena directly, or a separate temp buffer?
5. Does `_func`/`_funcend` change how ROM-resident script functions are declared in C code?
6. Control flow grammar — still open (`_if`, `_goto`, `_while`, block structures)
7. Lazy argv accessor implementation details — cache in arena, interaction with frame push/pop
8. Array literal syntax details — commas optional? Nesting?
9. `$arr.N` in expander — how to handle dynamic index without double-pass expansion?

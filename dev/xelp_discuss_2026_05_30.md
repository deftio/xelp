# Xelp Script Redesign Discussion - 2026-05-30

Status: **Brainstorming / Nothing decided**

- **Current version**: 0.4.0 (`0x00000400` in `src/xelp.h`)
- **Branch**: `exp_argv_core` (experimental)

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

## Open questions

1. Can the paren evaluation avoid text substitution without giving up parens?
2. What is the right return-value mechanism that handles both int and string without making the engine complex?
3. Should mR[] be expanded/typed, or should there be a separate result mechanism?
4. How to handle unused return values cleanly in the grammar?
5. Should builtins be layered into sub-feature gates (core, math, bitwise, comparison)?
6. Is arena compaction worth the code size for the fragmentation problem?
7. What's the minimum viable script engine (which features are essential vs nice-to-have)?
8. For `_func`/`_funcend` CLI accumulation: where do lines buffer during definition? Arena directly, or a separate temp buffer?
9. Should `_funcend` trigger local variable cleanup (true local scope), or just mark the body boundary?
10. Does `_func`/`_funcend` change how ROM-resident script functions are declared in C code?

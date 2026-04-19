# Xelp Script Engine Design

A Tcl/bash-style text scripting module for xelp. Compiles under
`XELP_ENABLE_SCRIPT`. Independent of the bytecode VM (`XELP_ENABLE_VM`)
-- they can coexist on the same XELP instance but neither requires the
other.

**Target code size:** 2-4 KB compiled (ARM Thumb-2)
**Target RAM per instance:** `XELP_SCRIPT_HEAPSZ` (user chooses, recommended 2 KB+)

---

## 1. Scope and Goals

This module extends xelp's existing one-pass text execution
(`XELPParse`) with:

- **Named variables** (`$x`, `$count`) -- integer-valued
- **Positional parameters** (`$1`, `$2`) for function arguments
- **User-defined functions** (`proc`) -- body strings stored in heap
- **Control flow** (`_if`/`_else`/`_endif`, `_goto`, `_while`/`_endwhile`)
- **Arithmetic and comparison** (`_add`, `_sub`, `_mul`, `_div`, `_eq`, `_lt`, etc.)
- **Return values** via `$?` / `r0`

The syntax remains Tcl/bash-like: `command arg1 arg2 ...` with
semicolons and newlines as statement separators. No new parsing concepts
(no brackets, no infix expressions). The scripting builtins are just
commands that happen to be built in.

### Non-goals (for this module)

- String variables (deferred -- see section 12)
- Nested expression evaluation (`[expr ...]`)
- Closures, anonymous functions, first-class functions
- Dynamic memory (malloc/free) -- the heap is a fixed-size array
  embedded in the XELP struct, managed by a simple arena allocator

---

## 2. The Hard Problems

Three interrelated problems dominate this design:

1. **Variable / parameter expansion** -- when `$x` appears in a command,
   what happens and where does the expanded text go?
2. **Function stack frames** -- when function A calls function B, how are
   A's variables and positional params preserved?
3. **Memory management** -- all of the above must live in a fixed-size
   heap embedded in the XELP struct, with no malloc, and the entire
   instance must remain relocatable via `memcpy`.

These are addressed in sections 4, 5, and 6 respectively.

---

## 3. User-Facing Syntax

### Variables

```
_set x 10              # set variable x = 10
_set x 0xFF            # hex literal
_set x $y              # copy from another variable
_echo $x               # use variable value
led $x                 # pass variable to user command
```

### Positional parameters (inside a proc)

```
$0                     # function name (like $FUNCNAME in bash)
$1, $2, $3 ...         # positional arguments
$#                     # number of arguments (future)
```

### Functions

```
_proc blink "led $1; delay $2; led 0; delay $2"
blink 1 500            # call: $1=1, $2=500
```

### Control flow

```
_if $x                 # execute block if $x != 0
    echo "x is set"
_else
    echo "x is zero"
_endif

loop:
    _dec x
    _if $x
        _goto loop
    _endif

_while $count
    led 1; delay 100; led 0; delay 100
    _dec count
_endwhile
```

### Arithmetic

```
_add result $a $b      # result = a + b
_sub result $a $b      # result = a - b
_mul result $a $b      # result = a * b
_div result $a $b      # result = a / b (integer)
_mod result $a $b      # result = a % b
_inc x                 # x = x + 1
_dec x                 # x = x - 1
_neg result $a         # result = -a
```

### Bitwise

```
_and result $a $b
_or  result $a $b
_xor result $a $b
_not result $a
_shl result $a $n
_shr result $a $n
```

### Comparison (result is 0 or 1)

```
_eq result $a $b       # result = (a == b) ? 1 : 0
_lt result $a $b       # result = (a < b) ? 1 : 0
_gt result $a $b       # result = (a > b) ? 1 : 0
```

### Return values

```
_proc double "_mul r0 $1 2; _ret"
double 5               # r0 = 10, accessible as $?
_echo $?               # prints 10
```

`$?` is an alias for register r0. All functions return their result
in r0. The `_ret` command exits the current function early; reaching
the end of the function body also returns.

---

## 4. Variable and Parameter Expansion

This is the central design challenge. When xelp encounters `$x` in a
command string, it must replace it with the integer value of variable
`x`. But xelp's scripts are const (ROM-able) -- we cannot modify the
source string in place.

### 4.1 The Problem

Consider:

```
_set gain 75
motor $gain 1
```

The C function behind `motor` expects `(const char *args, int len)` and
receives the raw text `"motor $gain 1"`. It doesn't know about `$gain`.
Something must expand `$gain` to `75` before the function sees it.

### 4.2 Solution: Expansion Buffer in the Heap

When a line contains `$`, it is expanded into a scratch region of the
script heap before dispatch. The expansion replaces `$varname` tokens
with their decimal string representation.

```
Source:   "motor $gain 1"
                ↓ expand
Heap:     "motor 75 1"      (written to expansion region)
                ↓ dispatch
C func receives "motor 75 1" with len=10
```

The expansion buffer is a reserved region at the end of the heap (see
section 6 for the full memory layout). Its size is `XELP_EXPAND_BUFSZ`
(default 128 bytes). This is NOT the 64-byte `mCmdMsgBuf` -- users
enabling scripting should expect to provide a heap of at least 512
bytes, and 2 KB+ is recommended.

The expansion buffer is transient -- it's written before dispatch and
its contents are only valid for the duration of that one command
execution. Once the command returns, the buffer can be reused for the
next line.

### 4.3 Expansion Algorithm

```
expand_line(ths, src, src_len):
    dest = heap + expand_region_offset
    dp = dest
    for each char c in src[0..src_len]:
        if c == '$':
            name = parse_varname(src)   # read until non-alnum
            value = resolve(ths, name)  # positional, register, or named
            dp += int_to_str(value, dp) # write decimal digits
        else:
            *dp++ = c
        if dp >= dest + XELP_EXPAND_BUFSZ:
            return XELP_E_CMDBUFFULL    # expansion overflow
    *dp = '\0'
    return XELP_S_OK
```

The expanded line is then passed to the existing dispatch logic
(command lookup + function call).

### 4.4 When Expansion Does NOT Happen

- Lines with no `$` character skip expansion entirely (fast path).
  The original source pointer is passed directly to dispatch.
- Inside quoted strings, `$` is expanded (Tcl behavior). To get a
  literal `$`, use the escape character: `` `$ ``.
- The `_set` command's first argument (the variable name) is NOT
  expanded: `_set x 10` sets variable named `x`, not the value of `$x`.

### 4.5 Variable Name Resolution Order

When `$name` is encountered:

1. **Positional params** (`$0`-`$9`, `$#`): checked first (only valid
   inside a proc call frame)
2. **Registers** (`$r0`-`$r3`, `$?`): checked next
3. **Named variables** (`$x`, `$count`): scan of variable entries in heap
4. **Not found**: expand to `0` (or error if strict mode enabled via
   `XELP_SCRIPT_STRICT`)

---

## 5. Function Definitions and Stack Frames

### 5.1 Proc Storage

User-defined functions are stored as tagged entries in the heap arena
(see section 6). Each proc entry contains:

```
[ TAG_PROC | name_len | name bytes... | body_len (2 bytes) | body bytes... ]
```

The body text is **copied into the heap** when `_proc` is defined. This
is critical: when a user defines a proc from the interactive CLI, the
input line is transient. By copying the body into the heap, the proc
persists beyond the input that created it. It also means the entire
XELP instance (including all proc bodies) is self-contained and
relocatable.

When `_proc` is defined from a ROM-resident script, we still copy it
into the heap. This wastes a few bytes of heap vs. storing a pointer to
ROM, but it keeps the model uniform and avoids a "is this pointer into
our heap or into external memory?" distinction that would complicate
relocation.

If heap space is tight and the proc body is known to be in persistent
ROM, a future optimization could store a pointer + flag instead. But
that breaks `memcpy` relocatability unless the pointer is to truly
fixed ROM.

### 5.2 Command Resolution Order

When a command name is encountered:

1. **Script builtins** (`_set`, `_if`, `_goto`, etc.) -- fast prefix
   check on `_`
2. **Proc table** (user-defined script functions in heap)
3. **C function table** (`mpCLIModeFuncs`)
4. **Default handler** (`mpfDefCLI`) if set
5. **Error**: command not found

Script builtins are checked first because they use the `_` prefix
convention and can be rejected with a single-character check. Procs
are checked before C functions so users can override C commands with
script functions (useful for testing, wrapping, adding logging).

**Lookup is linear scan, deliberately.** A binary search variant was
prototyped (`XELP_USE_BSEARCH` in `xelp-plan-2025.md`) but abandoned
-- it requires command tables to be sorted, which means a compilation
step or manual discipline, and the payoff is negligible for typical
table sizes (< 30 commands). If you need dispatch performance, the VM
path is the right answer, not optimizing text lookup. See section
17.9 for the full rationale.

### 5.3 Call Frames

When a proc is called, a frame is pushed to preserve the caller's state.
The frame must save:

- **Positional parameters** for the caller's context
- **Script position** (so we can return to where we were)
- **Control flow state** (if_depth, skip_depth)

Call frames are allocated from the **top of the heap, growing downward**
(see section 6). Each frame is fixed-size:

```c
typedef struct {
    const char* ret_script_pos;   /* return position in caller's script */
    const char* ret_script_end;   /* end of caller's script             */
    uint8_t     ret_if_depth;     /* caller's if nesting depth          */
    uint8_t     ret_skip_depth;   /* caller's skip state                */
    XELPREG     args[XELP_PROC_MAX_ARGS]; /* positional params $1..$N  */
    uint8_t     argc;             /* number of positional params        */
} XelpCallFrame;
```

```c
#ifndef XELP_PROC_MAX_ARGS
#define XELP_PROC_MAX_ARGS   4     /* max positional params per call  */
#endif
```

**Frame size:** `2 pointers + 2 bytes + (4 args * sizeof(XELPREG)) + 1`
= ~27 bytes on 32-bit, ~19 bytes on 16-bit.

**Maximum call depth** is not a compile-time constant. It's bounded by
available heap space: the frame stack grows down from the top while
variables/procs grow up from the bottom. When they meet, you're out of
memory. On a 2 KB heap, you can fit ~70 frames (far more than needed).
In practice, call depth rarely exceeds 4-8.

A hard limit `XELP_MAX_CALL_DEPTH` (default 8) prevents runaway
recursion even if heap space is available:

```c
#ifndef XELP_MAX_CALL_DEPTH
#define XELP_MAX_CALL_DEPTH  8
#endif
```

### 5.4 Call Mechanics

When `blink 1 500` is executed and `blink` is found in the proc table:

1. **Parse arguments:** Tokenize the rest of the line. Token 1 = "1",
   token 2 = "500". Convert to integers.

2. **Push frame:** Allocate a `XelpCallFrame` from the top of the heap
   (frame stack grows down). Save current script position, if_depth,
   skip_depth. Store the parsed positional args in the new frame.

3. **Execute body:** Set script position to the proc body (which lives
   in the heap arena). Execute it via `XELPParse`. During execution,
   `$1` resolves to `args[0]` of the top frame, `$2` to `args[1]`, etc.

4. **Pop frame:** When the body finishes (or `_ret` is encountered),
   pop the frame (move frame stack pointer up). Restore script position
   and control flow state. Return value is in r0.

### 5.5 Variable Scoping

**All named variables are global** (per instance). There is one flat
set of variable entries in the heap. All functions read and write the
same entries.

This is a deliberate choice:

- **Simplicity.** No scope chains, no variable shadowing, no lifetime
  management. What you `_set` anywhere is visible everywhere (within
  that instance).
- **Low cost.** No per-frame variable storage. The frame only saves
  positional params, not a variable snapshot.
- **Embedded convention.** In firmware, global-ish variables (status
  flags, calibration values, I/O state) are the norm. Local variables
  are a luxury that doesn't match the use case.
- **Explicit sharing.** If function A sets `$gain` and function B reads
  `$gain`, that's intentional communication. The alternative (passing
  everything through positional params) is more correct but far more
  verbose for simple embedded scripts.

**What IS scoped:** Positional parameters (`$1`, `$2`, etc.) are
per-frame. Each function call gets its own set. When A calls B, B's
`$1` is B's first argument, not A's. This is essential for correct
recursion (to the limited depth allowed).

**Naming convention for "locals":** Users who want pseudo-local
variables can adopt a naming convention:

```
_proc blink "_set _bk_i $1; _set _bk_d $2; ..."
```

The `_bk_` prefix acts as a namespace. This is ugly but honest --
it matches what BASIC, early Tcl, and many shell scripts do.

### 5.6 Recursion

Recursion is supported up to `XELP_MAX_CALL_DEPTH` and available heap
space. Each recursive call pushes a frame. Since named variables are
global, only positional parameters are truly "per-call." A recursive
function that needs local state must use different variable names at
each depth, or use the register file (`$r0`-`$r3`) and accept that they
get overwritten.

In practice, recursion in embedded scripts is rare. The main use case
for call depth > 1 is composition: `init` calls `configure`, which calls
`calibrate`. Three levels deep is typical; more than 4 is unusual.

---

## 6. Memory Management: The Script Heap

This is the core architectural decision. All scripting state --
variables, procs, labels, call frames, and the expansion buffer --
lives in a single byte array embedded in the XELP struct.

### 6.1 Design Principles

- **Embedded in struct.** The heap is `uint8_t script_heap[XELP_SCRIPT_HEAPSZ]`
  inside the XELP struct. When you copy/move a struct, everything moves
  with it. No dangling pointers.
- **Offsets, not pointers.** All references within the heap use `uint16_t`
  offsets from `script_heap[0]`. This ensures that `memcpy(&cli2, &cli1,
  sizeof(XELP))` produces a valid, independent copy.
- **No malloc.** Allocation is a simple arena with a bump pointer.
  Deallocation uses swap-and-shrink for named entries, or is deferred
  until a `_clear` / reset.
- **User controls the budget.** One `#define` sets the size. Give 512
  bytes on an MSP430, 4 KB on an ESP32. The engine adapts.

### 6.2 Heap Layout

The heap is divided into three regions:

```
script_heap[XELP_SCRIPT_HEAPSZ]
┌──────────────────────────────────────────────────────────┐
│  Arena (grows →)        │  free  │  Frame stack (← grows)│
│  [vars] [procs] [labels]│        │  [frame2][frame1][f0] │
│                         │        │                       │
0              arena_used ↑        ↑ frame_top    HEAPSZ-EXPANDSZ
                                                  ↑ expand region
                                          [expansion buffer]
                                          HEAPSZ-EXPANDSZ .. HEAPSZ
└──────────────────────────────────────────────────────────┘
```

**Arena region** (bottom, grows up): Tagged variable-length entries for
variables, procs, and labels. Managed by a bump pointer (`arena_used`).

**Frame stack region** (top minus expand, grows down): Fixed-size
`XelpCallFrame` entries pushed/popped LIFO. Managed by a frame pointer
(`frame_top`).

**Expansion buffer** (fixed, at the very end): Reserved space for `$`
expansion of the current line. Size is `XELP_EXPAND_BUFSZ`.

**Out of memory:** When `arena_used` would collide with `frame_top`
(accounting for the expansion buffer), allocation fails and returns an
error.

### 6.3 Arena Entry Format

Each entry in the arena has a small header:

```
Byte 0:  tag (entry type)
Byte 1:  name_len
Bytes 2..2+name_len-1:  name (NOT null-terminated in storage)
Bytes 2+name_len..:     payload (type-dependent)
```

**NOTE (see 17.0B for revised thinking):** The `lang_design.md` notes
pack the tag and name length into a single byte: 3 high bits for type,
5 low bits for name length. This caps names at 31 characters. If we
adopt that packing, the entry header shrinks from 2 bytes to 1 byte.
For reference, Commodore 64 BASIC only compared the first 2 characters
of variable names -- `SCORE` and `SCREAM` were the same variable.
31 characters is generous. On a tight heap, short names save real
space: `_set co 42` costs 16 fewer heap bytes than
`_set calibration_offset 42`, which adds up with 20+ variables.

#### Entry types

| Tag | Type  | Payload                                  | Total size           |
|-----|-------|------------------------------------------|----------------------|
| `V` | Var   | `sizeof(XELPREG)` bytes (integer value)  | 2 + name_len + sizeof(XELPREG) |
| `P` | Proc  | `uint16_t body_len` + body bytes         | 2 + name_len + 2 + body_len |
| `L` | Label | `uint16_t script_offset`                 | 2 + name_len + 2    |
| `_` | Dead  | `uint16_t entry_len` (total incl header) | (used for compaction) |

Dead entries are created when a variable or proc is deleted. They are
skipped during lookup. Compaction (optional) slides live entries down to
reclaim dead space.

#### Example heap contents (2 vars, 1 proc)

```
Offset  Contents
0x000   'V' 1 'x' [10 as XELPREG]           # _set x 10
0x007   'V' 5 'c' 'o' 'u' 'n' 't' [5]       # _set count 5
0x012   'P' 5 'b' 'l' 'i' 'n' 'k' [len=24]  # _proc blink "..."
        'l' 'e' 'd' ' ' '$' '1' ';' ...      # body bytes inline
0x030   (arena_used = 0x30, next alloc here)
...
0x770   [XelpCallFrame for current call]      # frame_top
0x780   [expansion buffer, 128 bytes]         # HEAPSZ - 128
0x800   (end, HEAPSZ = 2048)
```

### 6.4 Offset-Based References

Internal references use `uint16_t` offsets, not pointers. This is what
makes `memcpy` relocation work.

When a proc body needs to be executed, the offset is converted to a
runtime pointer:

```c
const char* body = (const char*)(ths->script_heap + proc_body_offset);
```

This pointer is transient -- used for the duration of one `XELPParse`
call and then discarded. It is never stored persistently.

Similarly, label positions in the heap store an offset into the
*external* script buffer (not into the heap). These are `const char*`
pointers because they reference the user's script text, which is
external to the struct and doesn't move with it. This is the same
pattern as the existing `mpAboutMsg` and `mpCLIModeFuncs` pointers.

Call frame `ret_script_pos` and `ret_script_end` are also external
pointers (they point into the caller's script text). If the caller's
script is a proc body inside the heap, these should be stored as
offsets instead. The frame can include a flag indicating whether the
return position is internal (offset) or external (pointer):

```c
typedef struct {
    union {
        const char* ptr;    /* external script (ROM, user buffer) */
        uint16_t    offset; /* internal (proc body in heap)       */
    } ret_pos;
    union {
        const char* ptr;
        uint16_t    offset;
    } ret_end;
    uint8_t     ret_is_internal;  /* 0 = external ptr, 1 = heap offset */
    uint8_t     ret_if_depth;
    uint8_t     ret_skip_depth;
    XELPREG     args[XELP_PROC_MAX_ARGS];
    uint8_t     argc;
} XelpCallFrame;
```

This adds 1 byte per frame but correctly handles the mixed case where
a top-level script (external ROM) calls a proc (body in heap) which
calls another proc (also in heap).

### 6.5 Arena Operations

#### Allocate (internal)

```c
uint16_t arena_alloc(XELP *ths, uint16_t size) {
    uint16_t avail = ths->frame_top - ths->arena_used;
    if (size > avail) return XELP_HEAP_OOM;  /* out of memory */
    uint16_t offset = ths->arena_used;
    ths->arena_used += size;
    return offset;
}
```

#### Lookup variable

Linear scan from offset 0 to `arena_used`, skipping dead entries:

```c
uint16_t var_find(XELP *ths, const char *name, uint8_t name_len) {
    uint16_t pos = 0;
    while (pos < ths->arena_used) {
        uint8_t tag = ths->script_heap[pos];
        uint8_t nlen = ths->script_heap[pos + 1];
        if (tag == TAG_VAR && nlen == name_len &&
            memcmp(&ths->script_heap[pos + 2], name, name_len) == 0) {
            return pos;  /* found */
        }
        pos += entry_size(tag, nlen, &ths->script_heap[pos]);
    }
    return XELP_HEAP_NOT_FOUND;
}
```

For small heaps (< 2 KB) with typical variable counts (< 20), linear
scan is fast enough. No hash table needed.

#### Set variable

1. Scan for existing entry with matching name.
2. If found: update value in place (same offset, same size).
3. If not found: append new `TAG_VAR` entry at `arena_used`.
4. If no space: return `XELP_E_FULL`.

#### Delete variable

1. Find the entry.
2. Mark as `TAG_DEAD` with stored entry length.
3. If it's the last entry in the arena, reclaim immediately by
   moving `arena_used` back.
4. Otherwise, leave as dead. Compaction reclaims later.

#### Define proc

1. Scan for existing proc with same name.
2. If found: mark old as dead, append new (body length may differ).
3. If not found: append new `TAG_PROC` entry. Copy name and body
   bytes into the arena.
4. If no space: return `XELP_E_FULL`.

#### Compaction

`_compact` (available as a script command and C API):

Slide all live entries down to fill dead gaps. Update `arena_used`.
O(n) in heap size. Only needed after many deletes.

This is optional -- on small heaps with few deletes, fragmentation
is negligible. The compactor is ~50-80 lines of C.

### 6.6 Instance Struct Additions

```c
#ifdef XELP_ENABLE_SCRIPT

    /* Script heap -- all scripting state lives here */
    uint8_t     script_heap[XELP_SCRIPT_HEAPSZ];

    /* Arena management */
    uint16_t    arena_used;     /* bump pointer (bytes used from bottom)  */
    uint16_t    frame_top;      /* frame stack pointer (from top)         */
    uint8_t     frame_count;    /* number of active call frames           */

    /* Execution state (small, always in struct proper) */
    const char* script_pos;     /* current execution position             */
    const char* script_end;     /* end of current script                  */
    uint8_t     if_depth;       /* _if nesting level                      */
    uint8_t     skip_depth;     /* skip-if-false tracking                 */

#endif
```

The heap, management counters, and execution state total:
`XELP_SCRIPT_HEAPSZ + 10 + 2*sizeof(pointer)` bytes added to the struct.

### 6.7 Configuration

```c
#ifndef XELP_SCRIPT_HEAPSZ
#define XELP_SCRIPT_HEAPSZ   2048   /* bytes; user should set this */
#endif

#ifndef XELP_EXPAND_BUFSZ
#define XELP_EXPAND_BUFSZ    128    /* reserved at end of heap */
#endif

#ifndef XELP_MAX_CALL_DEPTH
#define XELP_MAX_CALL_DEPTH  8      /* hard limit on recursion */
#endif

#ifndef XELP_PROC_MAX_ARGS
#define XELP_PROC_MAX_ARGS   4      /* positional params per call */
#endif
```

**That's it.** No `XELP_VAR_COUNT`, `XELP_LABEL_COUNT`,
`XELP_PROC_COUNT`, `XELP_VAR_NAME_LEN`, `XELP_PROC_NAME_LEN`. The
arena handles variable-length entries and the capacity is determined by
heap size. One knob instead of ten.

### 6.8 Memory Budget Examples

| Platform    | HEAPSZ | EXPAND | Approx capacity                         |
|-------------|--------|--------|-----------------------------------------|
| MSP430      | 512    | 64     | ~30 vars, or ~8 vars + 3 small procs    |
| Cortex-M0   | 1024   | 128    | ~60 vars, or ~15 vars + 8 procs         |
| ESP32       | 2048   | 128    | ~120 vars, or ~30 vars + 15 procs       |
| Desktop     | 4096   | 256    | plenty for development/testing          |

These are rough estimates assuming 8-char average name length and
40-byte average proc body.

### 6.9 Relocatability Guarantee

Because:
- The heap is an array in the struct (not a pointer to external memory)
- All intra-heap references use `uint16_t` offsets
- External pointers (script_pos, script_end, label positions) point to
  user-provided script text which is independent of the struct

...the following operations are safe:

```c
/* Copy an instance (deep copy, independent state) */
XELP cli2 = cli1;   /* or memcpy(&cli2, &cli1, sizeof(XELP)) */

/* Move an instance */
XELP *p = malloc(sizeof(XELP));   /* if the platform supports it */
memcpy(p, &cli1, sizeof(XELP));
/* p is now a fully functional independent instance */
```

The one caveat: if execution is mid-flight when you copy (i.e.,
`script_pos` points into a proc body in cli1's heap), the copy's
`script_pos` still points into cli1's heap. Don't copy during
execution. Copy only when the instance is idle (no script running).
Document this.

---

## 7. Variable Design Detail

Variables are `TAG_VAR` entries in the arena:

```
[ 'V' | name_len | name bytes | value (XELPREG) ]
```

### Lookup

Linear scan from arena start to `arena_used`, skipping non-VAR and
dead entries. For typical variable counts (< 20), this is fast.

### Create / Update

`_set x 10`:
1. Scan for existing VAR entry named `x`.
2. If found, update its value in place.
3. If not found, append new entry at `arena_used`.
4. If no space (arena meets frame stack), return error.

### Delete

`_del x`:
1. Scan for VAR entry named `x`.
2. If found, mark as `TAG_DEAD`.
3. If it's the last entry, reclaim by adjusting `arena_used`.

### Register aliases

`$r0` through `$r3` are not stored in the heap. They resolve to
`ths->mR[0..3]` at lookup time by checking the name prefix. This means
registers don't consume heap space.

`$?` is a direct alias for `$r0` / `mR[0]`.

### Interaction with the VM module

If both `XELP_ENABLE_SCRIPT` and `XELP_ENABLE_VM` are compiled in,
the VM's `LVAR`/`SVAR` opcodes read and write the same heap variable
entries. This allows a bytecode program to set a variable that a text
script later reads, or vice versa. The heap is the shared data bridge
between the two execution paths.

---

## 8. Label Handling

Labels are `TAG_LABEL` entries in the arena:

```
[ 'L' | name_len | name bytes | script_offset (uint16_t) ]
```

The `script_offset` is an offset into the *current script buffer*
(external, user-provided). It records where execution should jump to
(the position just after the label definition line).

### Label detection

During script execution, when a line's first token ends with `:`, it's
treated as a label definition. The name (without the colon) and the
current script position are stored as a new arena entry.

Labels are discovered during forward execution and cached. A `_goto`
to a label that hasn't been seen yet requires a forward scan of the
remaining script. This is O(n) but happens at most once per label per
script execution (subsequent gotos hit the cache).

### _goto semantics

`_goto loop` looks up the label `loop` in the heap. If found, set
`script_pos` to the cached position. If not found, scan forward from
current position, wrapping to script start if necessary. If not found
after a full scan, return error.

### Label lifetime

Labels are per-script-execution. At the start of each `XELPParse` call
(for a new script or proc body), existing label entries can be cleared
from the arena. Labels from the calling script are irrelevant inside a
called proc body. This avoids label-name collisions between caller and
callee.

**Implementation:** Track the arena offset at proc entry. On proc
return, discard any arena entries added during the proc call (reset
`arena_used` to the saved offset). This cleanly removes labels AND
any variables the proc created, giving procs quasi-local scope for
free. However, this means variables set inside a proc don't persist
after return -- which conflicts with the "all variables are global"
design in section 5.5.

**Resolution:** Only discard label entries on proc return, not variable
or proc entries. Labels get a special "ephemeral" flag or are stored in
a separate lightweight table (small fixed array of `{name, offset}`
pairs, cleared per `XELPParse` call). This keeps labels scoped without
affecting variable persistence.

Actually, the simplest approach: **labels go in a small fixed array in
the execution state, not in the heap.** They're transient (only valid
during one script execution), small (8 entries is plenty), and
fixed-size. This avoids polluting the persistent heap with ephemeral
data:

```c
#define XELP_LABEL_COUNT  8

typedef struct {
    uint8_t     name_len;
    char        name[XELP_LABEL_NAME_LEN];
    const char* pos;
} XelpLabel;

/* In XELP struct (not in heap): */
XelpLabel    labels[XELP_LABEL_COUNT];
uint8_t      label_count;
```

This costs ~100 bytes in the struct but simplifies heap management.
Labels are cleared at each `XELPParse` entry. No heap fragmentation
from label churn in loops.

---

## 9. Control Flow Implementation

### _if / _else / _endif

State tracking:

```c
uint8_t if_depth;    /* nesting level (0 = top level)         */
uint8_t skip_depth;  /* level at which we started skipping    */
                     /* 0 = not skipping                      */
```

Algorithm:

- `_if $x`: increment `if_depth`. If `$x == 0` and not already
  skipping, set `skip_depth = if_depth` (start skipping).
- `_else`: if `skip_depth == if_depth`, stop skipping
  (`skip_depth = 0`). Else if `skip_depth == 0`, start skipping
  (`skip_depth = if_depth`). (Toggle skip at current depth.)
- `_endif`: if `skip_depth == if_depth`, stop skipping
  (`skip_depth = 0`). Decrement `if_depth`.
- Any other command: if `skip_depth > 0`, skip the line entirely.

This handles arbitrary nesting correctly. Nested `_if` inside a
skipped block increments `if_depth` (so we can count matching
`_endif`s) but doesn't change `skip_depth`.

### _while / _endwhile

```
_while $count       # if $count != 0, execute body; else skip to _endwhile
    ...
_endwhile           # jump back to _while
```

Implementation: `_while` records its script position. `_endwhile`
jumps back to that position for re-evaluation. If condition is false,
skip forward to matching `_endwhile`.

This can be implemented with the label mechanism: `_while` implicitly
creates an anonymous label, and `_endwhile` implicitly does a `_goto`
to it. The skip-forward when false uses the same depth-counting
approach as `_if`/`_endif`.

### Infinite loop protection

Optional: `XELP_MAX_ITERATIONS` (default 10000). A per-execution
counter that decrements for each `_goto` or `_endwhile` backward
jump. When it reaches 0, execution halts with error. Set to 0 to
disable (for production systems where long scripts are expected).

---

## 10. Return Values

Every command in xelp returns `XELPRESULT` to C. The scripting module
extends this:

- **r0 / $?** holds the "logical" return value accessible from script.
  After any command execution, r0 is updated with the command's return
  code (XELP_S_OK = 0, errors are negative).
- **`_ret`** exits the current proc immediately. r0 retains whatever
  was last set.
- **`_ret $x`** (optional): sets r0 = $x, then exits.

For user-defined procs, the pattern is:

```
_proc add3 "_add r0 $1 $2; _add r0 $r0 $3; _ret"
add3 10 20 30    # r0 = 60
_echo $?         # prints 60
```

The C function behind a user command can also set r0 explicitly:

```c
XELPRESULT cmd_sensor(const char *args, int len) {
    int val = read_adc();
    ths->mR[0] = val;  /* set r0 for script access via $? */
    return XELP_S_OK;
}
```

---

## 11. C API Additions

```c
#ifdef XELP_ENABLE_SCRIPT

/* Initialize the script engine (call after XELPInit) */
XELPRESULT XELPScriptInit(XELP *ths);

/* Variable access from C */
XELPRESULT XELPSetVar(XELP *ths, const char *name, XELPREG value);
XELPRESULT XELPGetVar(XELP *ths, const char *name, XELPREG *value);
XELPRESULT XELPDelVar(XELP *ths, const char *name);

/* Proc management from C */
XELPRESULT XELPDefProc(XELP *ths, const char *name,
                       const char *body, uint16_t body_len);
XELPRESULT XELPDelProc(XELP *ths, const char *name);

/* Heap management */
XELPRESULT XELPHeapCompact(XELP *ths);     /* defragment arena      */
uint16_t   XELPHeapFree(XELP *ths);        /* bytes available       */
uint16_t   XELPHeapUsed(XELP *ths);        /* bytes allocated       */
XELPRESULT XELPHeapReset(XELP *ths);       /* clear all state       */

/* Dump state (for debugging / inspection) */
XELPRESULT XELPDumpVars(XELP *ths);    /* print all variables via mpfOut */
XELPRESULT XELPDumpProcs(XELP *ths);   /* print all proc names          */
XELPRESULT XELPDumpHeap(XELP *ths);    /* hex dump of heap (debug)      */

#endif
```

`XELPScriptInit` zeros the heap and sets `arena_used = 0`,
`frame_top = XELP_SCRIPT_HEAPSZ - XELP_EXPAND_BUFSZ`. It should be
called once after `XELPInit`.

These C APIs allow:
- Pre-populating variables before running a script (e.g., set `$adc_val`
  from a sensor reading, then run a calibration script)
- Defining procs from C that are callable from scripts
- Monitoring heap usage and triggering compaction if needed
- Resetting all scripting state without re-initializing the whole XELP
  instance

---

## 12. Deferred: String Variables

Integer-only variables are a pragmatic first step. String variables
add significant complexity but are naturally handled by the arena model:

**With the arena, string storage is straightforward.** A string variable
is just a `TAG_SVAR` entry:

```
[ 'S' | name_len | name bytes | str_len (2 bytes) | string bytes ]
```

Variable-length storage is already what the arena does. No separate
string pool, no fragmentation that isn't already handled by compaction.

**The remaining challenges are:**

- **Expansion:** When `$name` expands to a string (not an integer), the
  expansion buffer must hold the string verbatim. Long strings can
  overflow the expansion buffer.
- **Update semantics:** Changing a string var's value may change its
  length. Can't update in place. Must mark old as dead and append new.
  More heap churn, more need for compaction.
- **Type tracking:** `_set x "hello"` vs `_set x 10` -- the same
  command creates different entry types. Need to parse the value to
  determine type, or use a separate command (`_sets` for string).
- **Operations:** Concatenation, substring, comparison -- each is new
  code. Worth it? For most embedded use cases, probably not.

**Recommendation:** Defer string variables to a later phase. The arena
model makes them implementable when needed without redesigning memory
management. For now, strings are always literals. Variables are integers.

---

## 13. Interaction with Existing xelp Features

### Command dispatch integration

The scripting builtins (`_set`, `_if`, `_goto`, etc.) are NOT entries in
the user's `XELPCLIFuncMapEntry` table. They are handled internally in
an extended `XELPParse` before checking the user table. This means:

- Users cannot accidentally override `_if` with their own command
- The underscore prefix convention prevents name collisions
- The builtin check is a fast string comparison, not a table scan

### Echo and output

`_echo` uses the existing `XELPOut` mechanism. Variables are expanded
before output:

```
_set x 42
_echo "The answer is $x"
# Output: The answer is 42
```

### Help integration

If `XELP_ENABLE_HELP` is compiled in, `XELPHelp` should list user procs
alongside C functions. The proc entries in the heap have names that can
be enumerated. Optional: add a help-string field to proc entries (at the
cost of more heap space per proc).

### Multi-instance

Each XELP instance has its own heap. Two instances running scripts
simultaneously do not interfere. This is guaranteed by the same property
that makes the rest of xelp multi-instance safe: all state is in the
struct, no globals.

---

## 14. Error Handling

| Condition                  | Behavior                           |
|----------------------------|------------------------------------|
| Undefined variable `$x`   | Expand to 0 (or error if strict)   |
| Heap full (arena + frames) | Operation returns XELP_E_FULL      |
| Call stack overflow        | Proc call returns error, no exec   |
| Max call depth exceeded    | Proc call returns error             |
| Expansion buffer overflow  | Line skipped, error returned       |
| Mismatched `_if`/`_endif`  | Detected at runtime, error         |
| Undefined label (`_goto`)  | Error after full scan              |
| Division by zero           | `_div`/`_mod` return error         |
| Iteration limit exceeded   | Execution halts (if limit enabled) |

All errors are reported via `mpfErr` callback (if set) and returned as
negative `XELPRESULT` values. Script execution continues after
non-fatal errors (undefined variable) but halts on fatal errors
(stack overflow, mismatched control flow).

---

## 15. Configuration Summary

```c
/* Master enable */
#define XELP_ENABLE_SCRIPT       /* enable scripting module               */

/* Sub-feature flags */
#define XELP_ENABLE_MATH         /* _add, _sub, _mul, _div, _mod         */
#define XELP_ENABLE_CMP          /* _eq, _lt, _gt                        */
#define XELP_ENABLE_BITWISE      /* _and, _or, _xor, _not, _shl, _shr   */
#define XELP_ENABLE_PROC         /* _proc, call frames                   */
#define XELP_ENABLE_WHILE        /* _while / _endwhile                   */

/* Heap sizing -- THE primary knob */
#define XELP_SCRIPT_HEAPSZ  2048 /* total heap in XELP struct (bytes)    */
#define XELP_EXPAND_BUFSZ   128  /* reserved at end of heap for $-expand */

/* Limits */
#define XELP_MAX_CALL_DEPTH  8   /* hard limit on recursion depth        */
#define XELP_PROC_MAX_ARGS   4   /* positional params per call           */
#define XELP_MAX_ITERATIONS  10000 /* 0 = no limit                       */
#define XELP_LABEL_COUNT     8   /* labels (in struct, not in heap)      */
#define XELP_LABEL_NAME_LEN  8   /* max label name length                */

/* Strictness */
/* #define XELP_SCRIPT_STRICT */  /* undefined vars are errors           */
```

### Compile-time cost matrix

| What you enable              | Additional code | Additional RAM       |
|------------------------------|----------------|----------------------|
| SCRIPT (vars + if/goto)     | ~1.5 KB        | HEAPSZ + ~120 bytes  |
| + MATH                       | ~0.5 KB        | 0                    |
| + CMP                        | ~0.3 KB        | 0                    |
| + BITWISE                    | ~0.4 KB        | 0                    |
| + PROC (call frames)         | ~0.8 KB        | 0 (frames in heap)   |
| + WHILE                      | ~0.3 KB        | 0                    |
| Arena allocator + compactor  | ~0.5 KB        | 0                    |
| **All scripting features**   | **~4.3 KB**    | **HEAPSZ + ~120**    |
| + `()` expression evaluator  | ~0.5 KB        | 0                    |
| + `_readln` (blocking)       | ~0.2 KB        | 0                    |
| + string-as-code dispatch    | ~0.3 KB        | 0                    |
| **Full script engine**       | **~5.3 KB**    | **HEAPSZ + ~120**    |

The "+~120 bytes" covers the label table, execution state pointers,
and arena management counters that live in the struct outside the heap.

**Revised estimate (post-reconciliation):** The 4.3 KB figure assumed
integer-only variables and no expression evaluator. With the
everything-is-a-string model, paren expressions, `_readln`, and
string-as-code dispatch, a realistic total is **~6-8 KB** including
safety margins and platform variance. The 8 KB ceiling is a hard
design target -- if it grows past that, something has been
over-engineered.

---

## 16. Open Questions

1. **Compaction strategy.** When should compaction run? Options: (a) only
   on explicit `_compact` command, (b) automatically when allocation
   fails (try compact, then retry), (c) never (accept fragmentation,
   use `_clear` to reset). Option (b) is the most user-friendly.

2. **Proc body updates.** Redefining a proc (`_proc blink "new body"`)
   marks the old entry dead and appends a new one. With many redefines
   (e.g., iterative development at the CLI), the heap fragments. Auto-
   compaction on alloc failure (question 1) mitigates this. Should we
   also offer `_proc! blink "..."` for "replace in place if same length"?
   Probably over-engineering.

3. **Should the expansion buffer support `[cmd arg]` evaluation?**
   This is Tcl's bracket substitution: `_set x [sensor read]` would
   execute `sensor read`, capture r0, and substitute it. Powerful but
   adds recursive evaluation to the expansion step. Significant
   complexity. Defer to a future version if ever.

4. **Labels in heap vs struct.** Section 8 concluded that labels belong
   in a small fixed array in the struct, not in the heap, because they're
   ephemeral and fixed-size. This is pragmatic but adds ~100 bytes to the
   struct unconditionally (when SCRIPT is enabled). An alternative:
   labels in the heap with a "clear labels" operation at each
   `XELPParse` entry. This saves struct space but adds complexity to
   the arena scan (must distinguish labels from vars when clearing).

5. **Variable types beyond integer.** The `lang_design.md` document
   describes a packed variable format with type tags (string, code,
   integer, float32). The arena model can support this -- different tags
   for different types, different payload sizes. But the expansion logic,
   arithmetic commands, and comparison commands all need to handle
   multiple types. Significant code growth. Defer, but the arena is
   ready for it when the time comes.

6. **Heap size as compile-time vs runtime.** The current design uses a
   compile-time `XELP_SCRIPT_HEAPSZ` for the embedded array. An
   alternative: `XELP_SCRIPT_HEAPSZ 0` means "no embedded heap, user
   provides external buffer via `XELPScriptInitExt(ths, buf, len)`."
   This loses relocatability but allows runtime-sized heaps for
   platforms with dynamic memory. Could support both modes.

---

## 17. Design Scratchpad

Working notes and ideas that need to be explored before implementation.
Nothing here is decided -- it's a thinking-out-loud capture to avoid
losing threads.

### 17.0 Reconciliation with lang_design.md

The file `dev/lang_design.md` contains earlier design notes (circa
2012-2016) that reached several conclusions independently. This section
reconciles those insights with the current design and flags where the
current doc needs to change.

#### A. Function signature: `ths` as first argument (DECIDED)

The old notes explored multiple C function signatures:

```c
funcName(int)                                         // KEY mode only
funcName(xelp *ths, int)                              // KEY + instance
funcName(char *args, int alen)                        // CLI, no instance
funcName(xelp *ths, char *args, int alen)             // CLI + instance  ← winner
funcName(xelp *ths, char *args, int alen,
         char *buf, int blen, int bpos)               // full context
```

**Conclusion (both then and now):** The canonical CLI function
signature should be `XELPRESULT fn(XELP *ths, const char *args, int len)`.
This is a **breaking change** from the current signature
`XELPRESULT fn(const char *args, int len)` but solves multi-instance
and makes script integration natural:

- C functions can read/write variables: `XELPGetVar(ths, ...)`
- C functions can return values to script: `ths->mR[0] = val`
- C functions can output to the correct serial port: `XELPOut(ths, ...)`
- Multi-instance (serial + BLE, two UARTs, etc.) just works

This should happen as part of the scripting feature release. All
examples and the C++ wrapper (`XelpArduino.h`) need updating.

**Return values from C to script:**

```c
XELPRESULT cmdAdc(XELP *ths, const char *args, int len) {
    int val = adc_read(0);
    ths->mR[0] = val;       // script sees this as $?
    return XELP_S_OK;
}
```

Convenience macro: `#define XELP_RETURN(ths, val) ((ths)->mR[0] = (val), XELP_S_OK)`

From script: `_set reading (adc 0)` -- the `(...)` evaluator calls
`cmdAdc`, grabs `ths->mR[0]`, substitutes the result.

**NULL-instance degradation (COMPILE-TIME SAFETY):**

If `ths` is `NULL` (0), the function degrades gracefully -- it can
still do its pure-C work but cannot call `XELPOut`, `XELPGetVar`,
`ths->mR[0]`, etc. This enables a compile-time footgun removal
pattern:

```c
XELPRESULT cmdLed(XELP *ths, const char *args, int len) {
    int pin = XELPStr2Int(args, len);
    gpio_toggle(pin);              // pure C, always works
    if (ths) {                     // guard: only if xelp instance exists
        ths->mR[0] = gpio_read(pin);
        XELPOut(ths, "toggled\n", 0);
    }
    return XELP_S_OK;
}
```

Why this matters: a project can define `XELP_DISABLE_SCRIPT` (or
similar) and still use the same C function signatures. The dispatch
layer passes `NULL` instead of an instance pointer. Functions that
only do hardware work continue to compile and run. Functions that
need the interpreter degrade to no-output. With one compile flag, you
get a "headless" mode where the CLI functions are reusable library
code without any xelp runtime overhead.

Use cases:
- Unit testing command handlers without a full xelp instance
- Stripping the interpreter for production builds that only need
  the C functions
- Calling the same function from both xelp-managed and bare-metal
  code paths

The convenience macro handles this:
```c
#define XELP_RETURN(ths, val) \
    ((ths) ? ((ths)->mR[0] = (val), XELP_S_OK) : XELP_S_OK)
```

#### B. Variables and functions are the same thing (KEY INSIGHT)

The old notes describe this variable packing:

```
Byte 0:
  3 high bits = type (000=string, 001=code, 010=int, 011=float)
  5 low bits  = name length
```

The critical realization: **type 000 (string) and type 001 (code) have
identical storage.** A "function" is just a string variable with the
code bit set. When you call a name and it resolves to a code-typed
variable, the interpreter feeds the string to `XELPParse` with the
arguments.

This eliminates the separate `TAG_PROC` mechanism from section 5.
Instead:

```
_set blink "led @1; delay @2; led 0; delay @2"     # code variable
blink 1 500                                          # call it
```

The resolver finds `blink` in the variable table, sees it's a string,
and executes it. **No separate proc table, no separate `_proc`
command, no separate storage.** Variables and functions are unified.

**Impact on the current design:** Sections 5.1 (proc storage), 5.2
(command resolution), and the arena entry types (section 6.3) need
revision. `TAG_PROC` is replaced by `TAG_SVAR` with a code flag.
Or even simpler: ALL variables are byte sequences. The type tag tells
the expansion/evaluation logic how to interpret them.

**Naming convention:** The old notes used `$name` for retrieving a
value and considered `_set` for storing. This is consistent with the
current design. A possible simplification: `_set` determines the type
from the value syntax:

```
_set x 42                    # integer (no quotes)
_set x 0xFF                  # integer (hex)
_set msg "hello world"       # string (quoted)
_set blink "led @1; delay @2"  # also a string -- becomes code when called
```

No need for `_sets` vs `_set` vs `_proc`. One command, type inferred
from value syntax. Quoted = string/code, unquoted = integer (or
variable reference if starts with `$`).

#### C. Positional args: `@` not `$` (RECONSIDER)

The old notes used `@0 @1 @2` for positional arguments, not `$1 $2`.
The advantage: `$1` could be confused with "variable named 1" vs
"first argument." Using `@` makes the distinction clear:

```
$name     → named variable lookup
@0        → function name ($FUNCNAME equivalent)
@1 @2 @3  → positional arguments
@#        → argument count
$?        → last return value (alias for mR[0])
```

This avoids the bash ambiguity where `$1` in a function shadows `$1`
at global scope. In xelp, `$x` always means the global variable `x`,
and `@1` always means "my first argument." Clean separation.

**Impact:** Sections 3 and 4 of the current design use `$1` notation.
Should be reconsidered for `@1`.

#### D. Namespace resolution order (RECONSIDER)

Old notes had this priority:

```
1. Runtime script functions (user-defined at CLI)
2. Static script functions (defined in C as strings)
3. Programmer-supplied C functions
4. Language builtins (_goto, _if, etc.)
```

Current design (section 5.2) has:

```
1. Script builtins (_set, _if, etc.)
2. Proc table (user-defined)
3. C function table
4. Default handler
```

The old order lets users override C functions with script wrappers.
The new order protects builtins from being overridden. Both are
defensible choices.

**Proposed hybrid:** Builtins first (non-overridable, fast `_` prefix
check), then user script functions, then C functions, then default
handler. This matches the current design but keeps the user-override
capability for C functions:

```
1. Builtins (_set, _if, _goto, etc.)  -- fast _ check, not overridable
2. User script functions/variables     -- can shadow C functions
3. C function table
4. Default handler
```

#### E. Bracket evaluation (PARTIALLY DESIGNED)

The old notes had a parser state machine extension for bracket
evaluation (`[` and `]`), including bracket counting (`bkt_cnt`),
nested bracket support, and recursive `XelpParse` calls when brackets
close. The return value goes on a "result stack."

The current design (section 17.1) proposes `(` and `)` instead of
`[` and `]`. Either delimiter works. Parens feel more natural for
expressions; square brackets are more Tcl-like.

The old state machine approach is concrete and implementable:

```
[ → enter bracket, bkt_cnt++
] → bkt_cnt--, if bkt_cnt == 0: evaluate bracket contents
    via XelpParse, substitute return value
```

This is the recursive evaluation mechanism that enables both
arithmetic expressions and inline command execution:

```
_set x (+ 3 5)            # arithmetic
_set x (adc_read 0)        # C function call, capture return
_set x (+ (adc_read 0) $offset)   # nested
```

#### F. "Everything is a string" model (SIMPLIFICATION)

Combining insights B and the string discussion: if we adopt the Tcl
model where all variables are byte sequences and type is determined
by context:

- `_set x 10` stores bytes "10"
- `_set name "Alice"` stores bytes "Alice"
- `_add` interprets its args as integers (calls `XELPStr2Int`)
- `_echo` outputs bytes as-is
- Calling a variable by name executes it as code

Then: **no type tags needed in storage.** A variable is just
`[name_len][name][value_len][value_bytes]`. The command that uses
the variable decides how to interpret it. This is maximally simple
for storage and eliminates the type-dispatch code.

Trade-off: every arithmetic operation must parse its operands from
string to int and format results back to string. On a 32-bit MCU
this is negligible. On an MSP430 it's measurable but still small.

**Alternative:** Keep the type-tagged format from `lang_design.md`
(3-bit type in byte 0). Integers are stored as binary, avoiding
parse/format overhead. Strings are stored as bytes. The `_set`
command infers type from syntax. This is slightly more code but
faster at runtime for math-heavy scripts.

**Decision needed:** Pure string model (Tcl, simpler) vs type-tagged
model (faster math, more code)?

#### G. Summary of changes needed to current design

| Section | Change | Reason |
|---------|--------|--------|
| 3 (Syntax) | Consider `@1` instead of `$1` for positional args | Avoids ambiguity with `$`-named variables |
| 5.1 (Proc Storage) | Unify procs and string variables | `lang_design.md` insight: code is just a callable string |
| 5.2 (Resolution) | Builtins first, then script vars, then C | Hybrid of old and new ordering |
| 5.3 (Call Frames) | Update for `@` notation | If `@` adopted |
| 6.3 (Arena Entries) | Reconsider entry types | May simplify to one type if "everything is a string" |
| 11 (C API) | All CLI functions get `XELP *ths` | Breaking change, already decided in lang_design.md |
| New | Add bracket/paren expression evaluator | Old parser state machine design exists |

### 17.1 Parentheses and Expression Syntax

The three-operand form (`_add result $a $b`) works but is verbose.
Parentheses could enable grouped expressions:

```
_set result (+ $a $b)          # prefix, one operation
_set result (+ $a (* $b $c))   # nested prefix -- Lisp-like
_set result (+ $a $b $c)       # variadic? sum of three?
```

This requires extending the tokenizer to recognize `(` and `)` as
delimiters. The current tokenizer splits on whitespace -- parens
would need to be treated as token boundaries.

**Key question:** what are parens *for*? Options:

- **Grouping for arithmetic** -- `(op args...)` is an expression that
  returns a value. Used only in `_set` and `_if` contexts.
- **Grouping for commands** -- `(cmd args)` executes a command and
  substitutes its return value (like Tcl's `[cmd args]`). More powerful
  but means recursive evaluation.
- **Just grouping for readability** -- no semantic meaning beyond
  visual clarity. Tokenizer strips them.

If we pick option 1 (arithmetic grouping), the syntax becomes:

```
_set x (+ 3 5)                 # x = 8
_set x (* $x 2)                # x = x * 2
_if (> $x 10)                  # conditional on expression
    _echo "big"
_endif
```

This is S-expression style. The evaluator for `(op ...)` is small:
read op, evaluate args (recursing for nested parens), apply op, return
integer. Maybe 200-300 lines.

**Readability comparison:**

```
# Without parens (current design)
_mul temp $b $c
_add result $a $temp

# With parens
_set result (+ $a (* $b $c))
```

The second is clearly better. But it requires a recursive descent
expression evaluator. Still small at the code level, but it's a
different complexity class than "tokenize and dispatch."

**Decision needed:** Are parens worth ~300 bytes of code for the
expression evaluator?

### 17.2 The String Problem

Strings are the hardest part of the design. There are several tiers:

**Tier 0: Static strings only (current)**
Strings are literals in commands. No string variables.
```
echo "hello world"             # literal, passed as-is to echo handler
```

**Tier 1: String variables, no manipulation**
You can store a string and recall it, but not modify it.
```
_sets greeting "hello world"   # store string in heap
_echo $greeting                # expands to "hello world"
_sets msg $greeting            # copy
```
Expansion: `$greeting` in the expansion buffer becomes the string
contents. Works with the existing expansion model. Cost: a new tag
type (`TAG_SVAR`), expansion logic to handle string vs int.

**Tier 2: Basic string operations**
Concatenation, length, substring.
```
_cat result $a $b              # concatenation
_len result $greeting          # string length
_sub result $greeting 0 5      # substring
```
Each string operation creates new heap entries. Heap churn increases.
Compaction becomes more important.

**Tier 3: String substitution in function arguments**
This is where it gets truly hard. When a proc receives a string
argument, `$1` must expand to the string, which may contain spaces.
The expansion buffer must handle quoting correctly.
```
_proc greet "_echo Hello $1"
greet "world"                  # $1 = "world", output: Hello world
greet "Dr. Smith"              # $1 = "Dr. Smith" -- contains space!
```
The space in `"Dr. Smith"` means the expanded line `Hello Dr. Smith`
could be re-tokenized incorrectly. Need quoting preservation in
expansion.

**Tier 4: Full string manipulation**
Find, replace, format, split, join. At this point you're writing a
real language runtime. Way too heavy for xelp's scope.

**Recommendation:** Start with Tier 0 (current), implement Tier 1
early because it's cheap (just a new tag type + expansion logic).
Tier 2 if users ask for it. Tier 3 needs careful quoting design.
Tier 4 is out of scope.

**Previous experience:** In earlier xelp versions, strings were
compile-time only. You could manipulate numbers and emit text using
character codes, but constructing strings at runtime was impractical.
This was workable but "even Forth was better at strings and Forth is
terrible at strings."

**Revised thinking (see 17.0F):** If we adopt "everything is a string"
then there are no tiers -- all variables hold bytes. `_set x 42` stores
the bytes "42". `_set name "Alice"` stores the bytes "Alice". The
`_add` command parses its operands as integers; `_echo` outputs raw
bytes. No `TAG_SVAR` vs `TAG_VAR` distinction. The string problem
largely dissolves because strings aren't a special case -- they're the
only case.

The remaining hard problem is **Tier 3: multi-word expansion in
function arguments.** When `$name` expands to `Dr Smith` (with a
space), the re-tokenizer splits it into two tokens. Proposed solution:
the expansion pass auto-wraps multi-word values in quotes:

```
_set name "Dr Smith"
greet $name             # expander produces: greet "Dr Smith"
```

Rule: if a variable's value contains whitespace, the expansion wraps
it in `"..."`. This preserves it as a single token through
re-tokenization. Cost: ~10 lines in the expander. The value itself
does NOT contain quotes in storage -- quotes are added only during
expansion when needed.

### 17.3 Single-Key Command Batching

In earlier xelp versions there was a mechanism to run a string of
single-key commands from a script:

```
_sk "fpwbx"        # run single-key commands f, p, w, b, x in sequence
```

This is high-compression for repetitive tasks -- each character is a
complete command. Useful for test sequences, initialization macros,
and automated key-press simulation.

**Implementation:** Iterate over the string, call `XELPParseKey()`
for each character, staying in KEY mode for the duration. Small
(~20 lines) and useful. Should be a script builtin (`_sk`) or a
C-level API (`XELPRunKeySequence`).

**Use cases:**
- Automated test scripts that simulate user key presses
- Compact macros for mode-switching sequences
- Initialization sequences for peripherals controlled via KEY mode

### 17.4 Register Type Interpretation

From the VM design: the 4 registers (`mR[0..3]`) are `XELPREG`-sized
(platform int). Depending on the opcode, a register value can be
interpreted as:

- **s32** -- signed 32-bit integer (default for script `_add` etc.)
- **u32** -- unsigned (for bitwise ops, addresses)
- **f32** -- IEEE 754 float (if `XELP_USE_FR_MATH` or native float)

The register *storage* is the same 32 bits. The *interpretation* is
per-opcode. This is how every CPU works.

**For the script engine:** the `_` commands could offer type suffixes:

```
_addf result $a $b     # float add (interpret as f32)
_adds result $a $b     # signed add (default, maybe omit suffix)
_addu result $a $b     # unsigned add
```

Or a mode command:

```
_mode float            # subsequent math treats regs as f32
_add result $a $b      # float add
_mode int              # back to integer
```

**Recommendation:** Start with integer only. Float support behind
`XELP_ENABLE_FLOAT` or `XELP_USE_FR_MATH`. The register aliasing
between VM and script layers means the VM's typed opcodes are
available anyway if both modules are compiled in.

### 17.5 Proc Storage: Inline vs Reference

Current design copies proc body into heap always. Alternative for
ROM-resident scripts:

```c
typedef struct {
    uint8_t  tag;          /* TAG_PROC */
    uint8_t  name_len;
    /* name bytes... */
    uint8_t  is_ref;       /* 0 = inline body, 1 = external ref */
    union {
        struct { uint16_t body_len; /* followed by body bytes */ } inline_;
        struct { const char *ptr; uint16_t len; } ref;
    } body;
} ProcEntry;
```

Trade-off:
- Saves heap space for ROM procs
- Loses memcpy relocatability for ref procs
- Adds 1 byte + branch per proc lookup
- Only useful if many procs are defined from ROM scripts

**Decision:** Start with inline-only (simpler). Add ref mode later
if heap pressure is a real problem. The arena model doesn't need to
change -- it's just a different payload format for TAG_PROC.

### 17.6 Worked Examples

Concrete scripts to validate the syntax before implementation.

**Syntax conventions used in these examples (reflecting 17.0 reconciliation):**

- `$name` -- named variable lookup
- `@1 @2` -- positional arguments inside a function
- `@#` -- argument count
- `$?` -- last return value (alias for r0)
- `(op args)` -- expression evaluation (returns a value)
- `_set name value` -- assign (type inferred: quoted = string, unquoted = integer)
- Functions are callable strings: `_set fn "body"` then call `fn`
- Semicolons separate statements on one line
- `#` for comments (to end of line)

---

#### Example 1: LED Blink with Configurable Rate

Basic variables, callable string function, loop.

```
# Define a blink-once function
_set blink "led 1; delay @1; led 0; delay @1"

# Blink 5 times at 200ms
_set count 5
_while $count
    blink 200
    _dec count
_endwhile

# Blink 3 times at 500ms (reuse same function)
_set count 3
_while $count
    blink 500
    _dec count
_endwhile
```

**Exercises:** `_set` (integer + string/code), `_while`/`_endwhile`,
`_dec`, `@1` positional arg, callable string dispatch, user C
functions (`led`, `delay`).

---

#### Example 2: Sensor Calibration

Read ADC via C function, compute offset with script math, store
result, apply via C function. Shows C↔script data flow.

```
# C side registered: adc_read <channel>, motor_set <power> <dir>

# Read current sensor value (C function sets r0)
adc_read 0
_set raw $?

# Target is 512 (midpoint of 10-bit ADC)
_set target 512
_set offset (- $target $raw)

# Apply offset to a reading and drive motor
_set adjusted (+ $raw $offset)
motor_set $adjusted 1

_echo "raw=$raw offset=$offset adjusted=$adjusted"
```

With paren expressions:
```
_set adjusted (+ (adc_read 0) $offset)
```

Without paren expressions (three-operand form):
```
adc_read 0
_set raw $?
_add adjusted $raw $offset
```

**Exercises:** C function return values via `$?`, `(op ...)` expression
evaluation, `_echo` with variable expansion.

---

#### Example 3: Menu System

Proc per menu item, dispatch from CLI, mode switching.

```
# Define menu actions as callable strings
_set do_status "echo WiFi: connected; echo IP: 192.168.1.42"
_set do_reboot "echo Rebooting...; sys_reboot"
_set do_cal    "echo Calibrating...; adc_read 0; _set offset $?"

# Menu display function
_set show_menu "echo --- Menu ---; echo 1: Status; echo 2: Reboot; echo 3: Calibrate; echo ----------"

# Dispatch (called by user typing: menu 2)
_set menu "_if (== @1 1); do_status; _endif; _if (== @1 2); do_reboot; _endif; _if (== @1 3); do_cal; _endif"

# Usage from CLI:
#   show_menu
#   menu 1        --> runs do_status
#   menu 3        --> runs do_cal
```

**Note:** This works but the multi-`_if` dispatch is verbose. A
`_case`/`_switch` construct would help here but is not in the MVP.
An alternative using `_goto`:

```
_set menu "  \
    _if (== @1 1); _goto m_status; _endif; \
    _if (== @1 2); _goto m_reboot; _endif; \
    _echo unknown option @1; _ret; \
    m_status:; do_status; _ret; \
    m_reboot:; do_reboot; _ret"
```

**Exercises:** Multiple callable strings, `_if` with expression,
`_goto`/labels, `_ret`, `@1` dispatch.

---

#### Example 4: WiFi Setup (String Handling)

Tests the "everything is a string" model with multi-word values.

```
# Set WiFi credentials (strings with spaces)
_set ssid "My Home Network"
_set pass "hunter2 with spaces"

# Connect -- C function receives the string as-is
wifi_connect $ssid $pass
# Expander auto-wraps: wifi_connect "My Home Network" "hunter2 with spaces"

# Check status
wifi_status
_if $?
    _echo "Connected to $ssid"
_else
    _echo "Connection failed"
_endif

# Build a greeting message
_set name "Alice"
_set greeting "Hello $name, welcome to $ssid"
_echo $greeting
# Output: Hello Alice, welcome to My Home Network
```

**Exercises:** String variables, multi-word expansion with auto-quoting,
`$` expansion inside quoted strings, string variables passed to C
functions.

**Design question surfaced:** Does `_set greeting "Hello $name"` expand
`$name` at define-time or use-time? If at define-time (like bash
double-quotes), `$greeting` stores `"Hello Alice"`. If at use-time
(like Tcl braces), `$greeting` stores the literal `"Hello $name"` and
expansion happens when echoed. Bash-style (define-time) is simpler to
implement and easier to reason about.

---

#### Example 5: Automated Test Sequence

Single-key batching + script flow control.

```
# Single-key mode commands registered:
#   'r' = reset device
#   's' = read sensor
#   'p' = print status
#   'h' = help

# Run a test sequence via single-key batching
_sk "rsp"          # reset, sensor, print -- 3 commands, 3 chars

# Full test with validation
_sk "r"            # reset
delay 100          # wait for reset
_sk "s"            # read sensor
_set reading $?

_if (< $reading 100)
    _echo "FAIL: reading too low ($reading)"
_else
    _if (> $reading 900)
        _echo "FAIL: reading too high ($reading)"
    _else
        _echo "PASS: reading=$reading"
    _endif
_endif
```

**Exercises:** `_sk` single-key batching, nested `_if`/`_endif`,
`(< ...)` and `(> ...)` comparison expressions, `$?` from C function.

---

#### Example 6: Multi-Step Initialization (3 Levels Deep)

Proc calling proc, demonstrating call frames and positional args.

```
# Level 3: configure a single GPIO pin
_set gpio_cfg "gpio_mode @1 @2; gpio_write @1 0; _echo pin @1 mode @2"

# Level 2: configure a peripheral's pins
_set uart_init "gpio_cfg @1 1; gpio_cfg @2 1; uart_baud @3; _echo UART on @1/@2 at @3"
_set spi_init  "gpio_cfg @1 1; gpio_cfg @2 1; gpio_cfg @3 1; spi_speed @4"

# Level 1: full board init
_set board_init " \
    _echo === Board Init ===; \
    uart_init 0 1 115200; \
    spi_init 10 11 12 1000000; \
    _set board_ready 1; \
    _echo === Done ==="

# Run it
board_init

# Call chain: board_init -> uart_init -> gpio_cfg (3 levels)
# Each level has its own @1, @2, @3 -- frame stack preserves them
```

**Exercises:** 3-level call depth, `@1`/`@2`/`@3` at each level
(verifying frame isolation), callable strings calling callable strings,
global variable set from within a function (`$board_ready`).

---

#### Example 7: Interactive Tuning

User sets gain/offset at CLI, script applies to hardware in real time.
Shows the interactive workflow -- not a batch script.

```
# Pre-loaded calibration function
_set apply "adc_read 0; _set raw $?; _set out (+ (* $raw $gain) $offset); dac_write $out"

# Pre-loaded continuous run function
_set run "  \
    _set running 1; \
    _while $running; \
        apply; \
        delay 50; \
    _endwhile"

# User session at the CLI prompt:
#
#   > _set gain 2
#   > _set offset -100
#   > apply              <-- single shot, see the result
#   raw=512 out=924
#   > _set gain 3
#   > apply              <-- try different gain
#   raw=512 out=1436
#   > run                <-- continuous loop (ctrl-C to stop)
#   > _set gain 1        <-- can still set vars while running?
```

**Design question surfaced:** Can the user type commands while a
`_while` loop is running? In the current architecture, `XELPParse`
is blocking -- it runs the entire script to completion. The user can
only interact between script invocations. For real-time tuning, the
script would need to yield between iterations, or the loop would
need to be driven by the C `loop()` function calling `apply` on a
timer. This is more realistic:

```
# C side: loop() calls XELPParse(ths, "apply", 5) every 50ms
# User just sets variables at the CLI prompt between applies:
#
#   > _set gain 2
#   > _set offset -100
#   (hardware starts responding immediately -- C loop calls apply)
#   > _set gain 3
#   (hardware responds to new gain on next cycle)
```

This "C drives the loop, script defines the body" pattern is probably
the right model for real-time tuning. The script defines *what* to do;
C controls *when* to do it.

**Exercises:** C↔script data flow, `(+ (* ...) ...)` nested
expressions, interactive variable modification, discussion of
blocking vs cooperative execution model.

---

#### Syntax coverage matrix

| Syntax element         | Ex.1 | Ex.2 | Ex.3 | Ex.4 | Ex.5 | Ex.6 | Ex.7 | Ex.8 | Ex.9 | Ex.10|
|------------------------|------|------|------|------|------|------|------|------|------|------|
| `_set` integer         |  X   |  X   |      |      |  X   |  X   |  X   |  X   |      |      |
| `_set` string/code     |  X   |      |  X   |  X   |      |  X   |  X   |      |  X   |  X   |
| `$name` expansion      |  X   |  X   |      |  X   |  X   |      |  X   |  X   |  X   |  X   |
| `@1 @2` positional     |  X   |      |  X   |      |      |  X   |      |      |      |      |
| `$?` return value      |      |  X   |      |  X   |  X   |      |  X   |  X   |      |      |
| `(op ...)` expression  |      |  X   |  X   |      |  X   |      |  X   |  X   |  X   |      |
| `_if`/`_else`/`_endif` |      |      |      |  X   |  X   |      |      |  X   |  X   |      |
| `_while`/`_endwhile`   |  X   |      |      |      |      |      |  X   |      |      |      |
| `_goto`/labels         |      |      |  X   |      |      |      |      |      |      |      |
| `_dec`/`_inc`          |  X   |      |      |      |      |      |      |      |      |      |
| `_echo`                |      |  X   |  X   |  X   |  X   |  X   |      |  X   |  X   |  X   |
| `_ret`                 |      |      |  X   |      |      |      |      |  X   |      |      |
| `_sk` (key batching)   |      |      |      |      |  X   |      |      |      |      |      |
| `_readln`              |      |      |      |      |      |      |      |  X   |  X   |  X   |
| callable string        |  X   |      |  X   |      |      |  X   |  X   |      |  X   |  X   |
| C function call        |      |  X   |      |  X   |      |  X   |  X   |  X   |  X   |  X   |
| nested call (2+ deep)  |      |      |      |      |      |  X   |      |      |      |      |
| string w/ spaces       |      |      |      |  X   |      |      |      |      |      |      |
| nested `_if`           |      |      |      |      |  X   |      |      |      |      |      |
| `$` in strings         |      |      |      |  X   |      |      |  X   |  X   |  X   |  X   |
| string comparison      |      |      |      |      |      |      |      |      |  X   |      |

### 17.7 FR_math Integration Points

If `XELP_USE_FR_MATH` is defined:

- Script gets `_fsin`, `_fcos`, `_fsqrt`, `_fmul` etc. commands
  that call FR_math functions on register values
- Registers interpreted as Q16.16 fixed-point for these operations
- `_set x 1.5` would need to parse a decimal and convert to Q16.16
  (or require `_setf x 0x00018000` in hex -- ugly)
- Better: `_setf x 1.5` that parses float-like syntax into Q16.16

**This is future work.** The script engine should not depend on
FR_math. But the architecture should not preclude it either. The
arena and register model already accommodate this.

### 17.8 User Input: `_readln` and Blocking

Scripts often need user input -- "enter a value", "confirm (y/n)",
"type your name." This is fundamentally different from batch script
execution because it requires *waiting for the user to type something.*

#### The problem

`XELPParse` is a blocking call -- it runs lines until the script ends.
There's no natural "pause and wait for keyboard input" because the
script engine doesn't own the input source. Characters come from the
platform's main loop (`Serial.read()`, `uart_getc()`, `getch()`, etc.)
and are fed to xelp one at a time via `XELPParseKey`.

A `_readln` command in a script would need to:
1. Pause script execution
2. Return control to the main loop
3. Accumulate characters until the user presses Enter
4. Store the result in a variable
5. Resume script execution from where it left off

This is a **yield/resume** pattern -- effectively coroutine-like
behavior.

#### Option A: Blocking `_readln` (simple, limited)

`_readln` takes a platform-specific input function pointer and busy-
waits:

```c
// Platform provides this:
typedef int (*XelpReadCharFn)(void);   // returns char or -1 if none

// Script builtin:
// _readln varname
//   reads chars until \n, stores in $varname
```

Implementation: call the read function in a loop until `\n`. This
blocks the entire system -- no other processing happens. Acceptable
for simple interactive prompts on a single-UART device. Unacceptable
for anything with real-time requirements.

```
# Simple blocking example
_echo "Enter calibration offset: "
_readln offset
_echo "You entered: $offset"
```

The function pointer is set during init:
```c
XELP_SET_FN_READCHAR(cli, &uart_getc);   // platform read function
```

#### Option B: Yield/resume (complex, correct)

`_readln` saves the script execution state and returns to the caller.
The main loop continues feeding characters to `XELPParseKey`. When
Enter is pressed, the accumulated line is stored in the target variable
and script execution resumes.

This requires:
- Saving `script_pos`, `script_end`, `if_depth`, `skip_depth` when
  yielding
- A "script is suspended, accumulating readln input" state flag
- When `\n` arrives during readln: store the accumulated line as a
  string variable, clear the flag, resume `XELPParse` from saved pos

The state machine is small but it's a new execution mode -- xelp goes
from "run to completion" to "can be suspended mid-script." This
affects the relocatability guarantee (don't `memcpy` while suspended)
and any assumptions about script atomicity.

```c
typedef enum {
    XELP_EXEC_IDLE,       // no script running
    XELP_EXEC_RUNNING,    // script executing (inside XELPParse)
    XELP_EXEC_READLN,     // script suspended, waiting for input
} XelpExecState;
```

#### Option C: Callback model (pragmatic middle ground)

Don't put `_readln` in the script engine at all. Instead, the C
application implements the interactive flow:

```c
// C side -- the "ask user" flow is in C, not in script
void cmdCalibrate(XELP *ths, const char *args, int len) {
    XELPOut(ths, "Enter offset: ", 0);
    // Set a flag so main loop captures the next line
    gWaitingForInput = 1;
    gInputCallback = &applyCalibration;
}

void applyCalibration(XELP *ths, const char *line, int len) {
    int offset = XELPStr2Int(line, len);
    XELPSetVar(ths, "offset", offset);
    XELPParse(ths, "apply", 5);   // run the script function
}
```

This keeps the script engine simple (no yield/resume) but pushes
the interactive flow into C. Scripts can still be part of it -- they
just can't be the thing that waits for input.

#### Recommendation

Start with **Option A** (blocking readln) behind `XELP_ENABLE_READLN`.
It's simple, it covers the common case (single-UART interactive
prompts), and the blocking limitation is acceptable for most embedded
CLI use cases. The function pointer model means platforms without
blocking input just don't set the pointer, and `_readln` returns an
error.

Option B (yield/resume) is the right long-term answer but it's a
significant complexity jump. Defer it until there's a real use case
that can't be solved with Option A or C.

#### Worked examples with `_readln`

**Example 8: Interactive Calibration Wizard**

```
# Walk the user through a calibration sequence

_echo "=== Sensor Calibration ==="
_echo ""

_echo "Step 1: Apply zero load to sensor"
_echo "Press Enter when ready..."
_readln dummy                    # wait, discard the value

adc_read 0
_set zero_reading $?
_echo "Zero reading: $zero_reading"

_echo ""
_echo "Step 2: Apply known load (enter weight in grams):"
_readln weight

adc_read 0
_set load_reading $?
_echo "Load reading: $load_reading"

# Compute scale factor: counts per gram
_set span (- $load_reading $zero_reading)
_if (== $span 0)
    _echo "ERROR: no change detected. Check sensor."
    _ret
_endif

# Store calibration
_set cal_zero $zero_reading
_set cal_span $span
_set cal_weight $weight

_echo ""
_echo "Calibration complete:"
_echo "  zero=$cal_zero span=$cal_span ref=$cal_weight"
_echo "  Use 'measure' command to read calibrated values"
```

**Example 9: Confirmation Prompt**

```
# Factory reset with confirmation

_set factory_reset " \
    _echo WARNING: This will erase all settings.; \
    _echo Type YES to confirm:; \
    _readln confirm; \
    _if (== $confirm YES); \
        _echo Erasing...; \
        eeprom_erase; \
        _echo Done. Restarting...; \
        sys_reboot; \
    _else; \
        _echo Cancelled.; \
    _endif"
```

**Example 10: Interactive Variable Setting**

```
# Let user set multiple parameters interactively

_set setup " \
    _echo Enter motor speed (0-255):; \
    _readln speed; \
    _echo Enter direction (0=fwd 1=rev):; \
    _readln dir; \
    _echo Enter duration (ms):; \
    _readln duration; \
    _echo Running motor: speed=$speed dir=$dir for $duration ms; \
    motor_set $speed $dir; \
    delay $duration; \
    motor_set 0 0; \
    _echo Done."
```

**Design questions surfaced:**

1. **String comparison in `_if`:** Example 9 uses `_if (== $confirm YES)`.
   In an "everything is a string" model, `==` needs to handle string
   comparison, not just integer. If both operands are valid integers,
   compare as integers. Otherwise compare as strings (`memcmp`). This
   is Tcl's model and it works naturally.

2. **`_readln` for integers vs strings:** `_readln speed` stores whatever
   the user types. If they type `"150"`, it's the string "150". When
   `motor_set $speed $dir` passes it to C, the C function calls
   `XELPStr2Int` as usual. Everything-is-a-string makes this seamless.

3. **Empty input / timeout:** Should `_readln` support a timeout?
   `_readln var 5000` -- wait 5 seconds, then store empty/zero? Useful
   for automated testing but adds complexity. Defer for now.

4. **Echo during readln:** Should characters echo as the user types
   during `_readln`? Probably yes, using the existing `XELPParseKey`
   echo behavior. But if `_readln` is implemented as Option A (blocking
   loop), it needs its own echo logic. Needs thought.

### 17.9 Command Lookup Performance and the VM/Script Split

A binary search for command dispatch was prototyped early on. The idea:
sort the `XELPCLIFuncMapEntry` table alphabetically and use `bsearch()`
instead of linear scan. This turns O(n) lookup into O(log n).

**Why it was abandoned for text scripts:**

1. **Sorted tables require discipline or tooling.** Users hand-write
   command tables in C. Requiring alphabetical order is error-prone.
   A build-time sort tool (or a `XELP_CMD_TABLE_SORTED` macro that
   verifies at startup) adds complexity for marginal gain.

2. **Typical table sizes don't justify it.** Most xelp deployments
   have 5-30 commands. Linear scan of 30 short strings is < 1 us on
   any 32-bit MCU. The bottleneck in text dispatch is tokenization
   and string comparison, not table scan.

3. **Anyone needing real dispatch performance should use C, not
   script.** If a command runs 10,000 times per second, it shouldn't
   be dispatched through text parsing at all. That's a C function
   called directly, or a VM opcode.

4. **The `_` prefix check for builtins is already O(1).** A single
   byte comparison rejects non-builtin commands instantly. The
   remaining linear scan is only over user commands.

**Where it DOES matter: binary protocols.**

The calculus changes completely when the input isn't text. Consider
MIDI: a Control Change message is 3 bytes (`[status][cc#][value]`).
The "command" is a byte, not a string. Dispatch on a byte is a
switch/case or lookup table -- O(1). No tokenizer, no string
comparison, no expansion buffer.

This is exactly the use case that motivated the VM design (see
`xelp_vm.md`). The VM is a byte-oriented dispatch engine:

```
Text path:   "motor 75 1"  →  tokenize → strcmp → dispatch  (slow, flexible)
VM path:     [0x42][0x4B][0x01]  →  opcode switch → dispatch  (fast, compact)
```

The VM is actually simpler to design than the script engine -- no
tokenizer, no expansion buffer, no string handling, no label
resolution. It's a switch statement over a byte stream. But it adds
different baggage:

| Concern            | Script engine     | VM                    |
|--------------------|-------------------|-----------------------|
| Parsing complexity | High (tokenizer)  | None (byte stream)    |
| Human readability  | Yes               | No (needs disasm)     |
| Toolchain needed   | No (type at CLI)  | Yes (assembler/compiler) |
| String handling    | Natural           | Awkward               |
| Dispatch speed     | O(n) per command  | O(1) per opcode       |
| ROM density        | ~5 bytes/command  | ~2 bytes/instruction  |
| Interactive use    | Yes               | No                    |
| Binary protocols   | Awkward           | Natural               |

**The dual design conclusion:** Don't try to make text fast. Don't
try to make bytecode readable. Let each path do what it's good at.
They share the same XELP instance, the same C function table, the
same I/O callbacks. A text script can invoke a VM program
(`_vm run`). A VM `CFUNC` opcode can call a C function that calls
`XELPParse`. They're peers, not layers.

The binary search optimization was an attempt to make text do the
VM's job. Abandoning it clarified the design split.

---

## 18. Design Synthesis

After recovering old design notes and working through 10 examples,
the script engine design converges on a surprisingly compact core.

### What it takes

**Storage model:** Everything is a string. One arena entry format
holds variable names and their byte-sequence values. No type tags
in storage. Commands interpret values by context -- `_add` parses
integers, `_echo` outputs raw bytes, calling a name executes its
value as code. Variables and functions are the same thing.

**~15 builtin commands:**

| Command           | Purpose                              |
|-------------------|--------------------------------------|
| `_set`            | Assign variable (int, string, code)  |
| `_del`            | Delete variable                      |
| `_echo`           | Output with `$` expansion            |
| `_if` / `_else` / `_endif` | Conditional blocks          |
| `_while` / `_endwhile` | Loop with condition             |
| `_goto`           | Jump to label                        |
| `_ret`            | Return from function                 |
| `_add` `_sub` `_mul` `_div` `_mod` | Integer arithmetic  |
| `_inc` `_dec`     | Shorthand increment/decrement        |
| `_eq` `_lt` `_gt` | Comparison (result 0 or 1)          |
| `_readln`         | Read user input into variable        |
| `_sk`             | Single-key command batching          |

Plus `_and`/`_or`/`_xor`/`_not`/`_shl`/`_shr` behind a feature flag.

**4 special characters:**

| Char  | Meaning                                   |
|-------|-------------------------------------------|
| `$`   | Named variable expansion (`$name`)        |
| `@`   | Positional argument (`@1`, `@2`, `@#`)    |
| `()`  | Expression evaluation (`(+ $a $b)`)       |
| `""`  | String quoting (preserves spaces, expands `$`) |

That's it. No brackets, no braces, no backslash escapes beyond `\`$`
for literal dollar sign. The syntax is learnable in minutes.

### Why it works on small targets

- **One heap, one knob.** All state in `script_heap[N]`. User picks N.
- **No malloc.** Arena bump allocator + frame stack from the top.
- **Instance-based.** Copy with `memcpy`. Run N instances independently.
- **~6-8 KB code.** Fits alongside the existing 2 KB xelp core.
  Hard ceiling: 8 KB. If it exceeds that, something was over-designed.

### The compression argument

String-becomes-code enables significant compression over equivalent C.
A C function to blink an LED at a configurable rate:

```c
// C: ~20 lines, separate declaration + registration + implementation
void cmdBlink(XELP *ths, const char *args, int len) {
    XelpBuf b, tok;
    int rate, count;
    XELP_XBInit(b, args, len);
    XELP_XBTOP(b);
    XELPTokN(&b, 1, &tok);
    rate = XELPStr2Int(tok.s, tok.p - tok.s);
    XELP_XBTOP(b);
    XELPTokN(&b, 2, &tok);
    count = XELPStr2Int(tok.s, tok.p - tok.s);
    for (int i = 0; i < count; i++) {
        gpio_write(LED_PIN, 1);
        delay_ms(rate);
        gpio_write(LED_PIN, 0);
        delay_ms(rate);
    }
}
// plus: table entry, extern declaration, header include...
```

Script equivalent:

```
_set blink "led 1; delay @1; led 0; delay @1"
_set count 5
_while $count
    blink 200
    _dec count
_endwhile
```

The script version is shorter, readable, and modifiable at runtime
without recompiling. The C version is faster and type-safe. Both
have their place -- and both run on the same XELP instance.

### What performance ISN'T

Performance is not a goal for the script engine. If you need
performance, use C. If you need fast dispatch of binary protocols,
use the VM. The script engine is for:

- Interactive configuration and debugging
- Prototyping command sequences before committing to C
- Glue logic between C functions
- Runtime-modifiable behavior without reflashing
- User-authored automation on deployed devices

In all of these, human readability and ease of authoring trump
execution speed. The text parser runs at "human typing speed" --
even on an 8-bit MCU, that's more than enough.

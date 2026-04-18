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

The "+~120 bytes" covers the label table, execution state pointers,
and arena management counters that live in the struct outside the heap.

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

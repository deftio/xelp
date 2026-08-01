# XELP Script: Current Design Proposal2

XELP Script is a small, no-malloc, instance-local scripting and orchestration layer for embedded C and C++ systems. It is not trying to become Lua, MicroPython, Tcl, Forth, or a general-purpose VM. Its purpose is narrower: give firmware a tiny live command surface that can sequence, compose, and lightly control native C/C++ functions.

XELP’s core power is that C functions are not foreign bindings. They are the native vocabulary of the system. Script functions, CLI commands, key commands, and C functions all participate in the same command fabric.

## Design Center

XELP Script should preserve the original XELP identity while adding just enough language support to make command streams programmable.

* **C/C++ is the fast path** - Hardware access, drivers, protocol handling, math-heavy routines, timing-sensitive behavior, control loops, parsing, storage, networking, and application-specific work remain compiled native functions.

* **XELP is the orchestration layer** - XELP handles configuration, command sequencing, branching, small loops, diagnostics, factory tests, calibration flows, scripted bring-up, and field-service workflows.

* **Scripts are ROM-able** - Script source may live in ROM or flash. The interpreter must not mutate the source text.

* **No malloc is required** - Runtime state lives in a caller-supplied or instance-owned `XelpBuf`. Values, variables, function frames, temporary returns, and executable script strings are managed inside this buffer.

* **Multiple instances are natural** - A serial XELP instance and a BLE XELP instance can coexist on the same MCU, each with its own buffers, output hooks, mode, command table, and permission surface.

* **Command compression is a feature** - CLI scripts provide readable compression over C calls. Key-mode scripts provide extremely compact byte-like command streams over C functions.

A concise positioning statement:

```text
XELP Script is a no-malloc, ROM-able, instance-local command fabric for embedded C/C++ systems.
It runs at command speed. C/C++ remains the native fast path.
```

## Language Shape

XELP is command-shaped. It is Tcl-like at the surface, but it should not inherit Tcl’s full string-substitution model.

```text
command arg arg arg
```

A command may be:

```text
led 1
read_adc 0
_set x 10
_if (> $x 5) high
```

This keeps the parser small. There is no infix expression grammar, no algebraic precedence, no AST, and no general expression parser.

Math and conditionals use functional, Lisp-like command structures:

```text
_set x (* (+ $foo 4) 3)
_if (> $x 20) big
```

The symbolic operators are just command names. This:

```text
(+ 1 2 3)
```

means “call command `+` with arguments `1`, `2`, and `3`.”

This is not valid XELP math:

```text
1 + 2 * 3
```

If a user needs complex algebra, array math, filtering, PID control, DSP, or protocol parsing, that should be a C function.

## Invocation Model

There are only two invocation forms.

```text
foo arg arg
```

executes `foo` as a statement.

```text
(foo arg arg)
```

executes `foo` in a value-returning inner context.

The second form evaluates the child command and substitutes its returned typed value as one argument into the parent command.

Example:

```text
_set x (* (+ $foo 4) 3)
```

Evaluation:

```text
(+ $foo 4)       -> returns value A
(* A 3)          -> returns value B
_set x B         -> stores B in variable x
```

Parentheses provide explicit precedence without any precedence parser.

```text
_set x (+ 1 (* 2 3))     # x = 7
_set y (* (+ 1 2) 3)     # y = 9
```

The rule is simple:

```text
Arguments are evaluated left-to-right.
When an argument is a parenthesized context, it is evaluated first.
A parenthesized context returns exactly one typed value.
```

## Addressing Model

The grammar should keep variables, function arguments, and function invocation visually distinct.

```text
$name     named variable lookup
@1        positional argument lookup
@2        second positional argument
@name     named function argument lookup, if supported
@#        argument count
foo       command/function name in command position
(foo ...) nested value-returning command invocation
```

This gives XELP three clean addressing domains.

* **Variables use `$`** - `$foo` means “look up variable `foo` in the current variable context.”

* **Arguments use `@`** - `@1`, `@2`, and optionally `@name` refer to the current function call’s arguments.

* **Functions are invoked by name** - `foo a b` invokes `foo` as a statement. `(foo a b)` invokes it as a value-producing subcommand.

This split avoids the bash/Tcl ambiguity around `$1`. In XELP, `$x` is always a variable lookup, while `@1` is always an argument lookup.

## Bare Word Rule

Bare words should be simple and predictable.

In command position:

```text
foo a b
```

`foo` is resolved as a command or function.

In argument position:

```text
print hello
```

`hello` is a literal token, not a variable and not a function call.

To read a variable:

```text
print $hello
```

To call a function as an argument:

```text
print (hello)
```

To pass an argument:

```text
print @1
```

Recommended argument interpretation:

```text
number        -> INT
"quoted text" -> STR
bare word     -> STR/SYM literal
$name         -> variable value
@1 / @name    -> argument value
(...)         -> returned typed value
```

## Minimal Built-In Command Set

The first script profile should be small, but it should still feel complete enough for real orchestration. The goal is not algebra, precedence, or a math language. The goal is enough functional operators to let conditionals, loops, bit tests, counters, and simple state updates work without forcing every branch condition back into C.

All XELP language built-ins should begin with `_`, except for optional symbolic operator aliases such as `+`, `-`, `*`, `/`, `<`, `>`, `=`, `&`, `|`, `^`, `<<`, and `>>`. These symbolic operators are command names, not infix syntax.

This keeps the user/C command namespace open while still allowing compact functional expressions:

```text
_set x (* (+ $foo 4) 3)
_if (> $x 20) big
_if (_and (> $x 0) (< $x 100)) in_range
```

### Core Control

These commands form the smallest useful scripting layer.

```text
_set name value
_if condition label
_goto label
_return value
_ret
```

* **`_set name value`** - Stores a typed value. The name is raw; the value is evaluated.

* **`_if condition label`** - If condition is true, jump to label.

* **`_goto label`** - Jump to a label in the current script/function context.

* **`_return value`** - Return one typed value from the current function or script.

* **`_ret`** - Return or retrieve the current typed return value, depending on final semantics.

This avoids `_while`, `_else`, `_endif`, and block syntax in the minimal version. `_if` plus `_goto` is enough.

### Functional Math Operators

Math remains functional. These are command calls, not algebraic operators.

```text
+   or _add
-   or _sub
*   or _mul
/   or _div
%   or _mod
```

Example:

```text
_set x (* (+ $foo 4) 3)
```

This is evaluated by nested command calls:

```text
(+ $foo 4)       -> returns value A
(* A 3)          -> returns value B
_set x B         -> stores B
```

There is no expression grammar and no precedence parser. Parenthesized command contexts provide explicit evaluation order.

A single math dispatcher may be enough internally. For example, the implementation can have one compact math function that checks the operator token and accepts variadic input:

```text
(+ 1 2 3 4)      # variadic add
(* 2 3 4)        # variadic multiply
(- 10 3)         # subtraction
```

### Bitwise Operators

Bitwise operators are useful for embedded scripts because many hardware and status operations are bit-oriented. They should be available in the first complete profile.

```text
&     bitwise and
|     bitwise or
^     bitwise xor
<<    shift left
>>    shift right
```

Examples:

```text
_set masked (& $flags 0x08)
_if (> $masked 0) flag_set

_set reg (| $reg 0x04)
_set reg (& $reg 0xFB)
_set reg (<< $reg 1)
```

These are also command calls. This is valid:

```text
(& $flags 0x08)
```

This is not XELP syntax:

```text
$flags & 0x08
```

### Comparison Operators

Comparison operators return integer truth values, normally `1` or `0`.

```text
=    or _eq
!=   or _ne
<    or _lt
<=   or _le
>    or _gt
>=   or _ge
```

Example:

```text
_if (<= $n 0) done
```

Comparisons are functional calls. They can compare typed values. In the tiny profile, numeric comparisons should be numeric-only. Equality can support string/byte comparison because string status values are useful.

### Logical Operators

Logical operators are separate from bitwise operators. This distinction matters.

```text
_and
_or
_not
```

These are logical truth-value operators, not bitwise operators.

```text
_if (_and (> $x 0) (< $x 100)) in_range
_if (_not (= $state "ready")) not_ready
```

Truthiness should stay simple:

```text
NIL       false
INT 0     false
INT != 0  true
STR len 0 false
STR len>0 true
ERR       false or halt, depending strictness
```

## Function Arguments and Local State

A simple model is:

```text
@1, @2, @3   original positional function arguments
$name        variables in the current frame or outer frames
_set         writes to the current frame
```

Example:

```text
_fun blink

_set n @1
_set ms @2

:loop
_if (<= $n 0) done
...
_set n (- $n 1)
_goto loop

:done
_return "ok"

_end
```

This keeps function arguments immutable and explicit. Local mutation happens through variables.

A richer model could bind named args as local variables automatically:

```text
_fun blink count ms
```

Then:

```text
$count
$ms
```

would be locals. But this should be optional, not required for the first design.

## Labels and Goto

Labels are simple source positions.

```text
:loop
```

Goto jumps to a label in the current script or function context.

```text
_goto loop
```

A loop needs no `_while`:

```text
_set n 5

:loop
_if (<= $n 0) done

led 1
delay 100
led 0
delay 100

_set n (- $n 1)
_goto loop

:done
_return "done"
```

This is retro, but it is compact and sufficient. It is also very close to the “what BASIC could have been” spirit: labels instead of line numbers, native C functions instead of ROM routines.

## Typed Runtime Values

XELP should not use textual substitution as its primary runtime model.

Bad model:

```text
$msg expands to characters and then gets re-tokenized
```

Good model:

```text
$msg expands to one typed value
```

A value should be a compact binary record inside the XELP runtime buffer.

Temporary stack value:

```text
[type_flags][len][payload bytes]
```

Optionally, for easy pop:

```text
[type_flags][len][payload bytes][cell_size]
```

Basic runtime types:

```text
NIL
INT
STR
EXEC
ERR
FRAME
```

Integer payload:

```text
[INT][sizeof(XELPREG)][little-endian integer bytes]
```

String payload:

```text
[STR][len][bytes...]
```

Executable string payload:

```text
[EXEC][len][script bytes...]
```

The important point is that variables, returns, and intermediate `()` results are typed values, not just pieces of text.

## Heap Object Packing

Persistent named values in the XELP buffer should use the older compact packing idea.

```text
byte0:
  bits 7..5 = type
  bits 4..0 = name_len

byte1..name_len:
  name bytes, not null-terminated

payload:
  depends on type
```

Proposed type map:

```text
000 = STR
001 = EXEC
010 = INT
011 = FIX32, e.g. 8.24 fixed-radix
100 = BLOB / array / binary
101 = REF, optional ROM/static reference
110 = INTERNAL / frame / marker
111 = EXTENDED
```

Payloads:

```text
STR:
  [u16 len][bytes...]

EXEC:
  [u16 len][script bytes...]

INT:
  [sizeof(XELPREG) bytes little-endian]

FIX32:
  [4 bytes little-endian]

BLOB:
  [u16 len][bytes...]

EXTENDED:
  [extended header][payload...]
```

This unifies variables and script functions.

```text
_set x 10        -> named INT object
_set msg "hi"    -> named STR object
_fun blink ...   -> named EXEC object
```

The key insight:

```text
A script function is just a named executable string.
```

That means XELP does not need a separate function table for runtime script functions. Runtime-defined script functions can be stored as `EXEC` heap objects.

## Var/Value Stack and Function/Frame Stack

I agree that XELP probably wants both a var/value stack and a function/frame stack. They can be separate logical regions inside the same caller-supplied runtime buffer.

A practical layout:

```text
+----------------------------------------------------------------+
| var/value stack grows up --->        <--- frame stack grows down |
+----------------------------------------------------------------+
^                                                                ^
s                                                                e
```

The var/value stack stores:

```text
variables
temporary argument values
intermediate () return values
stored typed values
dynamic strings
promoted return cells
```

The function/frame stack stores:

```text
function call frames
parent frame offsets
saved parser/source positions
saved var/value stack marks
argument base/count
return slot location
status/flags
```

A frame record might contain:

```text
parent_frame_offset
saved_source_position
saved_source_end
saved_value_stack_mark
argument_base
argument_count
return_slot
flags
status
```

When a function or parenthesized context exits:

```text
1. Promote/copy its return value into a parent-owned slot.
2. Restore the var/value stack pointer to the frame's saved mark.
3. Pop the function/frame stack record.
```

This makes nested return values manageable.

Example:

```text
_set x (+ 3 (+ 4 3))
```

Execution:

```text
(+ 4 3) -> child frame returns INT 7
child temporaries are popped
(+ 3 7) -> parent expression returns INT 10
_set x 10
```

The important invariant remains:

```text
Every command returns one typed value plus status.
Every parenthesized context returns one typed value.
Every frame owns its temporaries.
Only the promoted return value survives frame cleanup.
```

## Strings as Data vs Executable Code

Strings and executable strings should share similar storage but not identical semantics.

```text
STR   = data
EXEC  = code
```

A bare command name can execute an `EXEC` object:

```text
blink 3 100
```

But variable lookup returns data:

```text
print $blink
```

That should not accidentally execute `blink`. Execution happens in command position or through an explicit eval-like command if one is later added.

This prevents the “everything is code accidentally” problem.

## Namespace Resolution

A hybrid namespace order makes the most sense.

```text
1. Language built-ins beginning with _
2. Runtime EXEC objects in the XELP heap
3. Static EXEC objects stored in ROM/C
4. Programmer-supplied C/C++ command table
5. Default handler
```

Why this order:

* **Built-ins first** - `_if`, `_goto`, `_set`, and `_return` are the language and should not be overridden accidentally.

* **Runtime script functions before C commands** - This allows script functions to wrap or shadow native functions, which gives XELP a Forth-like extensibility.

* **Static script functions before C commands** - Firmware can ship ROM-resident script functions.

* **C commands remain native** - If no script command shadows them, C functions are called directly.

## Runtime Buffer and Frames

The caller-supplied `XelpBuf` is central. It is both a writable heap and a stack-like runtime buffer.

`XelpBuf` already has the right shape:

```text
s = start
p = current cursor / allocation pointer
e = end
```

The runtime buffer stores:

```text
global variables
runtime EXEC objects
frame markers
local variables
function arguments
temporary values
nested () returns
dynamic strings
```

A frame marker lets the engine clean up without individual frees.

```text
[FRAME][frame metadata...]
[VAR ...]
[TEMP ...]
[TEMP ...]
```

When the function or `()` context returns:

```text
runtime.p = frame_start
```

Everything allocated after the frame marker disappears.

The only special rule:

```text
Before popping the frame, promote/copy the return value into the parent frame.
```

## Parenthesized Context Internals

A parenthesized context is a child evaluation frame.

```text
(+ $a (* $b 3))
```

Execution:

```text
enter context for +
evaluate $a
encounter (* $b 3)
  enter child context for *
  evaluate $b
  evaluate 3
  call *
  return INT
  promote result to parent context
call +
return INT
promote result to surrounding command
```

Invariant:

```text
Every parenthesized context returns exactly one typed value.
```

It may also return status/error information, but it still contributes one value to the parent argument list.

## Return Model

XELP already has return registers, and those should remain useful. But strings require a typed return value beyond raw integer registers.

Recommended model:

```text
status           XELPRESULT
typed_return     runtime value cell
R0/R1/etc.       compatibility / fast integer registers
```

Potential convention:

```text
R0 = status or legacy command result, depending existing compatibility
R1 = integer projection when typed_return is INT
typed_return = real script return value
```

The exact `$?` semantics need to be decided carefully.

Possible clean split:

```text
$?   last status
$_   last typed return value
```

But if `$?` is already mentally tied to `r0`, keep that for compatibility and document it clearly.

## Error and Success Semantics

A command returns two things conceptually:

```text
{ status, value }
```

* **Status** - `XELP_S_OK`, warning, or negative error code.

* **Value** - typed return value, such as INT, STR, NIL, or ERR.

For `_if`, truthiness should be simple:

```text
NIL       false
INT 0     false
INT != 0  true
STR len 0 false
STR len>0 true
ERR       false or halt, depending strictness
```

For the tiny build, error handling should be direct and boring. Fatal errors should halt the current script/function. Nonfatal errors can set status and continue only if explicitly allowed.

## C/C++ Command Integration

This is the superpower. XELP is not “a language with C bindings.” It is a C/C++ command surface with script semantics layered on top.

Existing raw command style:

```c
XELPRESULT fn(XELP *x, const char *args, int len);
```

Script-aware command style could be:

```c
XELPRESULT fn(XELP *x, int argc, XelpArgIter *args);
```

The implementation can hide encoded value cells behind helper functions:

```c
XelpArgInt(args, n, &v);
XelpArgStr(args, n, &s, &len);

XelpReturnInt(x, v);
XelpReturnStr(x, s, len);
XelpReturnNil(x);
```

On the script side, C commands and script commands look the same:

```text
_set v (read_adc 0)
_if (> $v 700) high
```

`read_adc` is C. `>` is a built-in. Both return one typed value.

## Command Compression

XELP can store behavior as compact command streams over compiled functions.

Readable CLI compression:

```text
l 1; d 100; l 0; d 100; m 2 50
```

Key-command compression:

```text
jwifbes
```

Key commands are especially compact because each byte can represent one native action. This is almost bytecode-like, but it remains application-defined and XELP-native.

This is useful because a short ROM string can replace a larger compiled sequence of C calls, especially for repetitive initialization, test, or mode-switching workflows.

## ISR and RTOS Considerations

XELP itself should not require malloc. It can be driven from interrupt-level input paths if the instance, output hooks, and bound commands are safe.

Careful phrasing:

```text
XELP can be built without malloc and can be driven from interrupt-level input paths;
ISR safety depends on the configured output hooks and bound commands.
```

Safe examples:

```text
single-key stop
set flag
emergency inhibit
reset parser state
```

Unsafe examples unless specifically designed for ISR use:

```text
flash write
blocking I2C
filesystem access
network send
printf-heavy output
delay
malloc
```

## Proposed Minimal Grammar

```ebnf
script      = { stmt } ;

stmt        = empty
            | label
            | command
            | fun_def ;

empty       = terminator ;

terminator  = "\n" | ";" | EOF ;

label       = ":" ident terminator ;

fun_def     = "_fun" ident [ { ident } ] terminator
              { stmt }
              "_end" terminator ;

command     = word { word } terminator ;

word        = literal
            | varref
            | argref
            | subcall ;

subcall     = "(" command_no_term ")" ;

command_no_term
            = word { word } ;

varref      = "$" ident ;
argref      = "@" ident | "@" number | "@#" ;

literal     = number | quoted | bare ;
```

Clarifying rules:

```text
A word in command position invokes a command.
A parenthesized command invokes a command in value position.
$name addresses a variable.
@name or @N addresses a function argument.
Bare words in argument position are literals.
```

## Example Program

This example shows the current intended style.

```text
# XELP Script Example
# Source can live in ROM.
# Runtime state lives in the XELP buffer.
# Native commands: led, delay, read_adc, relay, print.

_set threshold 700
_set blink_ms 100
_set device_name "pump-1"

_fun blink

_set n @1
_set ms @2

:loop
_if (<= $n 0) done

led 1
delay $ms
led 0
delay $ms

_set n (- $n 1)
_goto loop

:done
_return "blink-ok"

_end


_fun check_sensor

_set channel @1
_set v (read_adc $channel)

_if (> $v $threshold) high

_return "normal"

:high
_return "high"

_end


print "booting" $device_name

_set state (check_sensor 0)

print "sensor state:" $state

_if (= $state "high") alarm_path

print "system ok"
_return "ok"


:alarm_path

print "alarm"
relay 1

blink 3 $blink_ms

_set blink_result (_ret)

print "blink result:" $blink_result

_return "alarm"
```

## What the Example Proves

* **Functional math works** - `(- $n 1)` and `(> $v $threshold)` use command invocation, not infix parsing.

* **`()` provides precedence** - Nested calls can produce values in the desired order without a precedence parser.

* **Variables and arguments are separate** - `$n` is a variable; `@1` is an argument.

* **C commands compose naturally** - `(read_adc $channel)` returns a value like any other function.

* **Labels and goto are enough for loops** - No `_while` is required in the minimal version.

* **String returns are first-class enough** - `_return "high"` returns a typed string value.

* **Functions are native-ish** - A script-defined function becomes part of the command vocabulary.

## Recommended First Implementation

The first complete pass should include:

```text
_set
_if
_goto
_return
_ret

+
-
*
/
%

&
|
^
<<
>>

=
!=
<
<=
>
>=

_and
_or
_not
```

Possibly also include readable aliases:

```text
_add
_sub
_mul
_div
_mod
_eq
_ne
_lt
_le
_gt
_ge
```

Include later behind flags:

```text
_fun
_end
_int
_type
_eval
_sk
_peek
_poke
```

Avoid in the first version:

```text
infix expressions
operator precedence parser
arrays
objects
modules
closures
imports
garbage collection
malloc-backed strings
complex string manipulation
large standard library
```

## Agreement on the Two-Stack Direction

I agree with the var stack plus function/frame stack direction. It fits XELP better than a conventional heap/object runtime.

The reasons are practical:

* **Nested return values need somewhere to live** - `(+ 3 (+ 4 3))` requires the inner result to survive long enough to become an argument to the outer call.

* **Frame cleanup should be deterministic** - Restoring a stack pointer is much simpler than freeing individual temporaries.

* **No malloc stays credible** - The buffer is fixed, bounded, and caller-controlled.

* **C integration stays simple** - C commands can receive typed argument cells and write one return cell.

* **Performance expectations are aligned** - XELP runs at command speed. If a workflow needs fast execution, write a C function and call it.

The main caution is that the implementation must define exactly when values are copied versus referenced. The safest first design is to copy stored and returned strings into the runtime buffer, while allowing raw quoted arguments to remain source spans during immediate dispatch.

## Core Invariant

This is the rule that should keep the design from drifting:

```text
XELP looks like text, but evaluates typed values.

Every word expands to exactly one typed value.
Every command returns exactly one typed value plus status.
Parenthesized contexts evaluate to exactly one typed value.
Variables are addressed with $.
Function arguments are addressed with @.
Commands/functions are invoked by name or with (...).
String and executable-string objects are typed heap records.
All mutable runtime storage lives in the caller-supplied XelpBuf.
C/C++ functions remain the native fast path.
```

That keeps XELP from becoming a tiny bad Lua or a half-Tcl. It makes it what it wants to be: a compact, retro-but-useful, C-native scripting skin for small embedded systems.

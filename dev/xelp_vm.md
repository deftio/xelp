# Xelp VM Design

A register-based virtual machine for xelp. Compiles under `XELP_ENABLE_VM`.
Independent of the text scripting module (`XELP_ENABLE_SCRIPT`) -- they
can coexist on the same XELP instance but neither requires the other.

**Target code size:** 2-3 KB compiled (ARM Thumb-2)
**Target RAM per instance:** ~100-200 bytes (configurable)

---

## 1. Purpose and Motivation

The text-based CLI and scripting path (`XELPParse`, `XELPParseKey`) is
optimized for humans: readable, interactive, easy to type. But it is not
the right interface when:

- **Input is binary** (MIDI, SPI messages, packed protocol frames).
  Tokenizing text just to dispatch is wasted work.
- **Scripts live in constrained ROM.** A 50-byte text script may compress
  to 20 bytes of opcodes. On an 8051 with 256 bytes of RAM, this matters.
- **Deterministic timing matters.** Text parsing time varies with string
  length; bytecode dispatch is fixed-cost per instruction.
- **A front-end other than text is desired.** A visual block editor, a
  phone app, a host-side compiler -- all generate opcodes, none need to
  generate text.

The VM does NOT replace the text path. It runs alongside it. Both share
the same XELP instance, the same C function table, the same I/O callbacks.

---

## 2. Architecture Overview

```
  Text front-ends          Binary front-ends
  (CLI, script)            (MIDI, SPI, host compiler)
       |                          |
       v                          v
  XELPParse / XELPParseKey   XELPVMExec
       |                          |
       +------- shared -----------+
       |  C function table        |
       |  I/O callbacks           |
       |  Variable table *        |
       |  XELP instance state     |
       +--------------------------+

  * If XELP_ENABLE_SCRIPT is also compiled in, the VM can read/write
    the same variable table. If not, VM-only variables live in registers.
```

---

## 3. Registers

Four general-purpose registers, each `XELPREG` (platform int, typically
32-bit). Keeping it to four means:

- Register encoding fits in 2 bits per operand
- Call frame save/restore is 4 words (16 bytes on 32-bit)
- Sufficient for most embedded operations (accumulator, index, temp, return)
- Matches the register pressure of typical xelp commands (command + 1-3 args)

```c
#ifndef XELP_VM_REGS
#define XELP_VM_REGS  4
#endif
```

### Register conventions (advisory, not enforced)

| Register | Convention       | Notes                              |
|----------|------------------|------------------------------------|
| `r0`     | Return value     | Function results deposited here    |
| `r1`     | Argument 1       | First arg to called function       |
| `r2`     | Argument 2       | Second arg                         |
| `r3`     | Scratch / temp   | Caller-saved                       |

If `XELP_ENABLE_SCRIPT` is also compiled in, the script variable table
is accessible via dedicated opcodes (`LVAR`/`SVAR`), so the 4 VM
registers don't limit the number of named variables.

---

## 4. Opcode Encoding

Fixed-width 16-bit instructions. Each instruction is 2 bytes. This is
wider than a 1-byte opcode but eliminates variable-length decoding
complexity and aligns naturally on 16-bit platforms (MSP430, etc.).

```
Byte 0: opcode (6 bits) + dest reg (2 bits)
Byte 1: src1 reg (2 bits) + src2 reg (2 bits) + flags/mode (4 bits)

  [  opcode:6  | Rd:2 ] [ Rs1:2 | Rs2:2 | mode:4 ]
```

For immediate-value instructions, a 16-bit immediate follows in the next
2 bytes (making a 4-byte instruction):

```
  [ opcode:6 | Rd:2 ] [ 0x00 (reserved) ] [ imm16 hi ] [ imm16 lo ]
```

This gives 64 possible opcodes (6 bits). The initial set uses ~25.

---

## 5. Instruction Set

### 5.1 Data Movement

| Opcode   | Encoding          | Description                              |
|----------|-------------------|------------------------------------------|
| `LOADI`  | `Rd, imm16`      | Load 16-bit immediate into Rd            |
| `LOADIL` | `Rd, imm32`      | Load 32-bit immediate (2-word imm)       |
| `MOV`    | `Rd, Rs1`        | Copy Rs1 to Rd                           |
| `LVAR`   | `Rd, idx`        | Load variable[idx] into Rd (if SCRIPT)   |
| `SVAR`   | `idx, Rs1`       | Store Rs1 into variable[idx] (if SCRIPT) |

### 5.2 Arithmetic

All arithmetic operates on registers. Results go to Rd.

| Opcode | Encoding          | Description                |
|--------|-------------------|----------------------------|
| `ADD`  | `Rd, Rs1, Rs2`   | Rd = Rs1 + Rs2             |
| `SUB`  | `Rd, Rs1, Rs2`   | Rd = Rs1 - Rs2             |
| `MUL`  | `Rd, Rs1, Rs2`   | Rd = Rs1 * Rs2             |
| `DIV`  | `Rd, Rs1, Rs2`   | Rd = Rs1 / Rs2 (div by 0 -> error) |
| `MOD`  | `Rd, Rs1, Rs2`   | Rd = Rs1 % Rs2             |
| `NEG`  | `Rd, Rs1`        | Rd = -Rs1                  |
| `ADDI` | `Rd, Rs1, imm16` | Rd = Rs1 + imm16           |

### 5.3 Bitwise

| Opcode | Encoding          | Description                |
|--------|-------------------|----------------------------|
| `AND`  | `Rd, Rs1, Rs2`   | Rd = Rs1 & Rs2             |
| `OR`   | `Rd, Rs1, Rs2`   | Rd = Rs1 \| Rs2            |
| `XOR`  | `Rd, Rs1, Rs2`   | Rd = Rs1 ^ Rs2             |
| `NOT`  | `Rd, Rs1`        | Rd = ~Rs1                  |
| `SHL`  | `Rd, Rs1, Rs2`   | Rd = Rs1 << Rs2            |
| `SHR`  | `Rd, Rs1, Rs2`   | Rd = Rs1 >> Rs2 (arithmetic) |

### 5.4 Comparison

Comparisons set Rd to 1 (true) or 0 (false).

| Opcode | Encoding          | Description                |
|--------|-------------------|----------------------------|
| `EQ`   | `Rd, Rs1, Rs2`   | Rd = (Rs1 == Rs2) ? 1 : 0 |
| `LT`   | `Rd, Rs1, Rs2`   | Rd = (Rs1 < Rs2) ? 1 : 0  |
| `GT`   | `Rd, Rs1, Rs2`   | Rd = (Rs1 > Rs2) ? 1 : 0  |

### 5.5 Control Flow

| Opcode   | Encoding    | Description                                |
|----------|-------------|--------------------------------------------|
| `JMP`    | `offset`    | PC += signed offset (relative jump)        |
| `JZ`     | `Rs1, off`  | If Rs1 == 0, PC += offset                  |
| `JNZ`    | `Rs1, off`  | If Rs1 != 0, PC += offset                  |
| `CALL`   | `offset`    | Push frame (PC + regs), PC += offset       |
| `RET`    |             | Pop frame, restore PC + regs               |
| `HALT`   |             | Stop execution, r0 = return value          |

Jump offsets are signed 8-bit (in the mode field + extension), giving
-128..+127 instruction range. For longer jumps, a `JMPL` variant uses
a 16-bit offset in the following word.

### 5.6 I/O and System

| Opcode   | Encoding     | Description                               |
|----------|--------------|-------------------------------------------|
| `OUT`    | `Rs1`        | Emit Rs1 as char via mpfOut callback      |
| `IN`     | `Rd`         | Read byte from input (platform-specific)  |
| `CFUNC`  | `idx, Rs1`   | Call C function table entry by index, pass Rs1 |
| `PEEK`   | `Rd, Rs1`    | Rd = *(uint8_t*)Rs1  (memory read)        |
| `POKE`   | `Rs1, Rs2`   | *(uint8_t*)Rs1 = Rs2 (memory write)       |
| `NOP`    |              | No operation                              |

`CFUNC` bridges the VM to xelp's existing C function dispatch. The
function receives register values as arguments (exact calling convention
TBD -- may pack r1,r2 into the args string or use a dedicated VM-aware
prototype).

---

## 6. Call Frames

Function calls save the current state to a fixed-depth frame stack.

```c
#ifndef XELP_VM_CALL_DEPTH
#define XELP_VM_CALL_DEPTH  8
#endif

typedef struct {
    XELPREG  regs[XELP_VM_REGS]; /* saved registers     */
    uint16_t ret_pc;              /* return address       */
} XelpVMFrame;
```

**Memory cost per instance:**
  `8 frames * (4 regs * 4 bytes + 2 bytes) = 144 bytes` (32-bit platform)

On a constrained target, reduce to 4 frames (72 bytes).

### Call/Return semantics

- `CALL offset`: push current {r0-r3, PC+1} onto frame stack, jump
- `RET`: pop frame, restore registers and PC
- Caller places arguments in r1, r2 before `CALL`
- Callee returns result in r0
- If frame stack overflows, execution halts with error

---

## 7. Instance State (added to XELP struct)

```c
#ifdef XELP_ENABLE_VM
    /* VM execution state */
    XELPREG          vm_regs[XELP_VM_REGS];
    XelpVMFrame      vm_frames[XELP_VM_CALL_DEPTH];
    uint8_t          vm_fp;           /* frame pointer (0 = empty)   */
    const uint8_t*   vm_code;         /* pointer to bytecode buffer  */
    uint16_t         vm_code_len;     /* bytecode buffer length      */
    uint16_t         vm_pc;           /* program counter             */
    uint8_t          vm_status;       /* 0=idle, 1=running, 2=error  */
#endif
```

**Total RAM:** ~170 bytes with 4 regs and 8 frames. Configurable down to
~50 bytes (2 regs, 2 frames) on very constrained targets.

The bytecode buffer (`vm_code`) is a pointer to user-provided memory --
ROM, RAM, flash, wherever. The VM never writes to it.

---

## 8. C API

```c
#ifdef XELP_ENABLE_VM

/* Load a bytecode program (does not copy -- stores pointer) */
XELPRESULT XELPVMLoad(XELP *ths, const uint8_t *code, uint16_t len);

/* Execute until HALT or error. Returns r0 as result. */
XELPRESULT XELPVMExec(XELP *ths);

/* Execute at most N instructions (cooperative multitasking). */
XELPRESULT XELPVMStep(XELP *ths, uint16_t max_steps);

/* Reset VM state (registers, PC, frame pointer). */
XELPRESULT XELPVMReset(XELP *ths);

/* Read/write registers from C */
XELPREG    XELPVMGetReg(XELP *ths, uint8_t reg);
void       XELPVMSetReg(XELP *ths, uint8_t reg, XELPREG val);

#endif
```

### XELPVMStep -- cooperative execution

`XELPVMStep` runs up to N instructions then returns, preserving all
state. This allows the VM to run inside a main loop alongside other
tasks without blocking. Pattern:

```c
void loop() {
    XELPVMStep(&cli, 100);   /* run up to 100 VM instructions */
    handle_serial();
    handle_sensors();
}
```

This is interrupt-safe provided no ISR calls VM functions on the same
instance simultaneously.

---

## 9. Integration with fr_math

The sister library [fr_math](https://github.com/deftio/fr_math) provides
fixed-point arithmetic (sin, cos, sqrt, log2, atan2, etc.) in ~4 KB on
Cortex-M0 with zero dynamic memory. It is an ideal math backend for the
VM.

### Approach: fr_math as extended opcodes

When both `XELP_ENABLE_VM` and `XELP_USE_FR_MATH` are defined, additional
opcodes become available:

| Opcode    | Encoding         | Description                         |
|-----------|------------------|-------------------------------------|
| `FMUL`   | `Rd, Rs1, Rs2`  | Fixed-point multiply (Q16.16)       |
| `FDIV`   | `Rd, Rs1, Rs2`  | Fixed-point divide                  |
| `FSIN`   | `Rd, Rs1`       | Rd = fr_sin(Rs1, radix)            |
| `FCOS`   | `Rd, Rs1`       | Rd = fr_cos(Rs1, radix)            |
| `FSQRT`  | `Rd, Rs1`       | Rd = FR_sqrt(Rs1, radix)           |
| `FLOG2`  | `Rd, Rs1`       | Rd = FR_log2(Rs1, radix, radix)    |
| `FATAN2` | `Rd, Rs1, Rs2`  | Rd = FR_atan2(Rs1, Rs2, radix)     |

The default radix is set at compile time:

```c
#ifndef XELP_FR_RADIX
#define XELP_FR_RADIX  16   /* Q16.16 fixed-point */
#endif
```

### Why this works well

- fr_math functions take integer inputs and return integer outputs at a
  caller-specified radix -- they already operate on the same `XELPREG`
  type the VM uses.
- fr_math's `FR_printNumF` takes a `int (*f)(char)` callback, which
  maps directly to xelp's `mpfOut` character output.
- fr_math uses no dynamic memory, no globals, no floating point.
- On targets where fr_math is too large, simply don't define
  `XELP_USE_FR_MATH` -- the base VM still has integer MUL/DIV.

### Waveform and DSP use case

fr_math includes waveform generators (sin, square, triangle, sawtooth,
PWM, noise) and an ADSR envelope generator. With VM opcodes mapped to
these, xelp becomes capable of:

- Audio synthesis scripting (bytecode drives waveform parameters)
- MIDI CC -> VM register -> fr_math waveform -> DAC output
- Sensor signal conditioning (sqrt, log, interpolation)

This is where the MIDI front-end idea becomes concrete: MIDI bytes map
to VM opcodes that set waveform parameters via fr_math functions, all
without text parsing.

---

## 10. Binary Front-Ends

Because the VM consumes raw bytes, any byte-oriented protocol can drive
it. The pattern is always the same:

```c
/* Generic: feed bytes from any source */
void handle_protocol_byte(XELP *ths, uint8_t byte) {
    /* accumulate into a bytecode buffer, then: */
    XELPVMLoad(ths, buffer, len);
    XELPVMExec(ths);
}
```

### MIDI example

A MIDI Control Change message is 3 bytes: `[status] [cc#] [value]`.
A thin mapping layer converts this to VM instructions:

```
MIDI CC#1 value=64  -->  LOADI r1, 64; CFUNC 1, r1
```

The mapping table (CC number -> C function index) is user-provided,
just like the CLI command table. The VM is the execution engine; the
MIDI parser is just another front-end.

### SPI / I2C command packets

Same pattern. A 4-byte SPI packet `[cmd] [arg_hi] [arg_lo] [checksum]`
maps to `LOADI r1, arg; CFUNC cmd, r1`. The VM gives a uniform
execution model regardless of transport.

---

## 11. Offline Compiler (Host-Side Tool)

An offline tool (Python script or C program, runs on the development
host) compiles text scripts to bytecode:

```bash
xelpc --radix 16 script.xelp -o script.bin
```

This allows:
- Write scripts in readable text during development
- Deploy as compact bytecode to the target
- No parser needed on the target at all (if only VM is compiled in)
- Version control the text, ship the binary

The compiler is NOT part of the xelp library. It's a separate
development tool. The target only needs the VM executor.

This is a future deliverable -- the VM can be used and tested with
hand-assembled bytecode or C-generated opcode arrays first.

---

## 12. Interaction with Text Scripting

If both `XELP_ENABLE_VM` and `XELP_ENABLE_SCRIPT` are compiled in,
they share the XELP instance but execute independently:

- **Shared:** I/O callbacks, C function table, variable table (if
  `XELP_ENABLE_SCRIPT`), registers (`mR[]` and `vm_regs` are separate
  but can be bridged via LVAR/SVAR opcodes)
- **Independent:** The text interpreter does not generate bytecode.
  The VM does not parse text. They are parallel execution paths.
- **Bridging:** A text script command `_vm run` could invoke
  `XELPVMExec` on a pre-loaded bytecode buffer. A VM `CFUNC` could
  invoke a C function that calls `XELPParse`. But these are explicit
  user actions, not implicit layering.

### When to use which

| Use case                       | Path           |
|--------------------------------|----------------|
| Interactive terminal session   | Text (CLI)     |
| Human-authored config scripts  | Text (script)  |
| Pre-compiled deploy scripts    | VM             |
| Binary protocol handling       | VM             |
| MIDI / audio parameter control | VM             |
| Self-test sequences in ROM     | Either         |
| Factory calibration            | Either         |

---

## 13. Size Budget

| Component                        | Estimated code (ARM) | RAM per instance |
|----------------------------------|---------------------|------------------|
| VM executor (switch loop)        | ~800 bytes          | --               |
| Register + frame management      | ~400 bytes          | 170 bytes *      |
| I/O opcodes (OUT, IN, PEEK, POKE)| ~200 bytes          | --               |
| CFUNC bridge                     | ~200 bytes          | --               |
| fr_math opcodes (if enabled)     | ~300 bytes          | --               |
| **Total VM module**              | **~1.5-2 KB**       | **~170 bytes**   |

\* With XELP_VM_REGS=4, XELP_VM_CALL_DEPTH=8. Reducible to ~50 bytes
by shrinking to 2 regs and 2 frames.

fr_math itself adds ~4 KB (Cortex-M0, -Os) if all functions are used.
Linker dead-code elimination reduces this to only what's actually called.

---

## 14. Open Questions

1. **Opcode width:** 16-bit fixed vs 8-bit variable. Fixed is simpler
   and aligns on 16-bit platforms but wastes space for simple ops like
   NOP and RET. Could use 8-bit for zero-operand instructions and 16-bit
   for register ops. Decision deferred until implementation.

2. **CFUNC calling convention:** How does a VM opcode invoke a C function
   that expects `(const char *args, int len)`? Options:
   - New C function prototype: `XELPRESULT fn(XELP *ths, XELPREG *regs)`
   - Pack register values into a small string buffer and call the
     existing prototype (wasteful but compatible)
   - Both, selected per function entry in the table

3. **Shared registers:** Should `vm_regs[]` and the existing `mR[]`
   register file be unified? They serve different purposes today (mR is
   for script return values, vm_regs for VM computation), but merging
   would save RAM and simplify bridging. Depends on whether SCRIPT and
   VM are expected to interleave on the same instance.

4. **32-bit immediates:** The `LOADIL` instruction needs 6 bytes (2 opcode
   + 4 immediate). Is this common enough to justify, or should large
   constants be built from two `LOADI` + shift/or?

5. **Interrupt-driven VM execution:** Can an ISR call `XELPVMStep` on a
   *different* instance than the main loop? Should work (all state is per
   instance), but needs verification and documentation.

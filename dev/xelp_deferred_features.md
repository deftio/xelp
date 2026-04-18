# Deferred Features

Features that were declared in headers/config but had no implementation.
Disabled in `xelpcfg.h` as of 0.2.5 to avoid wasting RAM. Re-enable
when implemented.

## XELP_STACK_MACHINE / XELP_STACK_DEPTH

**What it was:** An integer stack (`int mS[16]`) in the XELP struct for
a Forth-style stack machine. 64 bytes of RAM per instance, unused.

**Where it was:**
- `xelpcfg.h`: `#define XELP_STACK_MACHINE` and `#define XELP_STACK_DEPTH (16)`
- `xelp.h`: `int mS[XELP_STACK_DEPTH]` in the XELP struct (behind `#ifdef`)

**Why removed:** No code in `xelp.c` reads or writes `mS[]`. Dead RAM.

**Future:** See `dev/xelp_vm.md` for the VM design that supersedes this.
The VM uses a register-based architecture rather than a pure stack
machine, but a Forth-style stack mode could be layered on top if desired.

## XELP_ENABLE_LCORE

**What it was:** A flag to enable built-in `peek`, `poke`, `go` commands
for direct memory access and jump-to-address from scripts.

**Where it was:**
- `xelpcfg.h`: `#define XELP_ENABLE_LCORE 1`
- `xelp.h`: referenced in `XELP_ENABLE_FULL` block

**Why removed:** No implementation exists in `xelp.c`. The define only
appeared in the `XELP_ENABLE_FULL` convenience macro.

**Future:** `peek`/`poke` are planned as VM opcodes (`PEEK`/`POKE` in
`dev/xelp_vm.md`) and could also be added as script builtins under
`XELP_ENABLE_SCRIPT`. The `go` (jump to address) command needs careful
design for safety.

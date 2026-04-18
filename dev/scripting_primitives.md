# Scripting Primitives (Removed from Source)

Code and config that was removed from the source tree because it had
no implementation. Preserved here for reference when these features
are implemented. See also `dev/xelp_vm.md` and `dev/xelp_script.md`.

---

## XELP_STACK_MACHINE

An integer stack for a Forth-style stack machine. Was in the XELP
struct, allocated 64 bytes per instance with no code that used it.

### xelpcfg.h (removed)

```c
/****************************************************************************************************
 XELP Stack Operations is a lightweight stack machine interpreter allowing one to run true commands
 from the CLI/script including simple math and variable passing.
 XELP_STACK_DEPTH specifies depth in integers of the machine
 */
#define XELP_STACK_MACHINE
#define XELP_STACK_DEPTH  (16)
```

### xelp.h XELP struct member (removed)

```c
#ifdef XELP_STACK_MACHINE
    int mS[XELP_STACK_DEPTH]; /* integer stack for XelpStackMachine */
#endif
```

### Future

The register-based VM design (`dev/xelp_vm.md`) supersedes this. A
Forth-style stack mode could be layered on the VM if desired.

---

## XELP_ENABLE_LCORE

A flag to enable built-in `peek`, `poke`, `go` commands for direct
memory access and jump-to-address from scripts. Never implemented.

### xelpcfg.h (removed)

```c
/****************************************************************************************************
 enable built-in language features such poke, peek, go
 */
#define XELP_ENABLE_LCORE  1
```

### xelp.h XELP_ENABLE_FULL block (removed line)

```c
#define XELP_ENABLE_LCORE       1   /* enable script language features such poke, peek, go */
```

### Future

`peek`/`poke` are planned as VM opcodes (`PEEK`/`POKE` in
`dev/xelp_vm.md`) and as possible script builtins under
`XELP_ENABLE_SCRIPT`. The `go` (jump to address) command needs
careful safety design.

# Branch status — `experimental/0.4.1-script-design` (0.4.1)

Orientation note for picking this work back up. Written 2026-08-03, after
merging master (0.3.5) in.

## What this branch is

**The later, smaller of the two 0.4.x script attempts** — a partial restart,
plus the design-document set. The name suggests it holds the script work, but
the *fuller* implementation is on `experimental/0.4.0-script-engine`. This branch is where the
requirements and proposals live and where a second, thinner engine was
started.

Both 0.4.x branches fork from `37cb455` (*0.4.0 native argc,argv draft*,
2026-05-15). This one has the version bump to 0.4.1 (2026-06-08, tagged
`v0.4.1-design`) plus the in-progress engine work.

## What it implements

**Dispatch**
- Handlers are `XELPRESULT fn(XELP *ths, int argc, const char **argv)`
- **Retains** public `XelpBuf2Argv`, `XELP_PARSE_ARGV`, and `XELP_ARGV_MAX`
- **Retains** `XELP_ENABLE_LINE_EDIT` and the `XELP_ENABLE_HISTORY` name
- `XELP_ENABLE_ARGV` removed — argv parsing is always compiled in

**Script engine** (`XELP_ENABLE_SCRIPT`) — partial
- `_xelpExpandVars` — variable expansion
- `_xelpVarSet` / `_xelpVarGet` / `_xelpVarScratch` — variable storage
- `_xelpFrameInit` / `_xelpFramePush` / `_xelpFramePop` — call frames
- `_xelpEvalScript` — evaluator
- **Missing** relative to `experimental/0.4.0-script-engine`: arena init, `_xelpVarFind`,
  `_xelpEvalLoop`, public `XelpCallProc`, script fuzz target

**Design documents** (the main value of this branch)
- `dev/xelp_script_requirements.md` — normative requirements (`A-01`, `C-01`, …)
- `dev/xelp_script_proposal1.md` / `2` / `3` — three design attempts;
  proposal 3 is the unified target (`XelpCall`, `@n` positionals vs `$name`
  variables, typed values, `_`-prefixed builtins)
- `dev/xelp_truthiness_trap_catalog.md` — cross-language truthiness traps
  reduced to a `TR-*` test matrix; pins `_truthy` as the sole coercion entry
  for `_if`
- `dev/xelp_discussion_0.5.x.md` — 0.5.x tier plan and the gate list
- `dev/xelp_script.md`, `dev/xelp_vm.md` — earlier design notes
  (`xelp_script.md` was deleted on `experimental/0.4.0-script-engine`; it survives only here)

## How it differs from `experimental/0.4.0-script-engine` (0.4.0)

They are **not** a linear progression — they are divergent designs from the
same fork point. This branch is *less* aggressive about API removal:

| | `experimental/0.4.1-script-design` (0.4.1) | `experimental/0.4.0-script-engine` (0.4.0) |
| --- | --- | --- |
| `XelpBuf2Argv` | still public | internal only |
| `XELP_ARGV_MAX` | retained | removed → `XELP_ARGV_CAP` |
| `XELP_ENABLE_LINE_EDIT` | retained | removed |
| history flag | `XELP_ENABLE_HISTORY` | `XELP_ENABLE_CLI_HISTORY` |
| script engine | vars + frames + `_xelpEvalScript` | arena + vars + frames + eval + `XelpCallProc` |
| `xelp.c` | ~1590 lines | ~2790 lines |
| tests | 54 units / 665 cases | 249 units / 1008 cases |

## Current state

- Merged with master 0.3.5 (commit `f428dbe`), so it carries the issue #18
  fix and the `tests-unsigned-char` build
- `make validate` passes: 54 units, 665 cases, 100% line / 97.46% branch
  coverage, under both signed and unsigned `char`
- Version is `0.4.1` (`0x00000401`); doc version strings were saying 0.4.0
  and have been corrected
- **Not merge-ready.** Breaking API changes relative to 0.3.x, and the
  script engine is incomplete

## Note on 0.5.x

There is no 0.5.x branch. `dev/xelp_discussion_0.5.x.md` records the plan —
inspector-tier `_*` builtins first (Tier 1), full `$` / `( )` / flow later
(Tier 2) — and states that script implementation is deferred until the gates
in `dev/issues.md` are consciously closed or accepted with written risk.

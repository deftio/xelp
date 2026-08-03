# Branch status — `exp_argv_core` (0.4.0)

Orientation note for picking this work back up. Written 2026-08-03, after
merging master (0.3.5) in.

## What this branch is

**The fuller of the two 0.4.x script attempts.** Despite the name — which
suggests argv plumbing — this is where the script engine actually lives and
runs. If you remember "getting most everything to work," it was here.

Both 0.4.x branches fork from `37cb455` (*0.4.0 native argc,argv draft*,
2026-05-15). This one continued for 15 commits through 2026-05-31.

## What it implements

**Dispatch (0.4.0 breaking change)**
- Handlers are `XELPRESULT fn(XELP *ths, int argc, const char **argv)`
- The engine always tokenizes; handlers never re-parse
- `XelpBuf2Argv` is internal (`_xelpBuf2Argv`); `XELP_ARGV_MAX` is replaced
  by `XELP_ARGV_CAP`, derived from `XELP_ARGVBUFSZ` and pointer width
- `XELP_ENABLE_ARGV` removed — argv is unconditional
- `XELP_ENABLE_LINE_EDIT` removed — line editing is always part of CLI
  (`#error` guard if you define it)
- `XELP_ENABLE_HISTORY` renamed `XELP_ENABLE_CLI_HISTORY` (`#error` guard)

**Script engine** (`XELP_ENABLE_SCRIPT`)
- `_xelpArenaInit` — fixed arena, no malloc
- `_xelpVarFind` / `_xelpVarGet` / `_xelpVarSet` / `_xelpVarEntrySize` —
  variable table
- `_xelpFramePush` / `_xelpFramePop` / `_xelpFrameArg` — call frames
- `_xelpEvalStatement` / `_xelpEvalLoop` — evaluator
- `XelpCallProc` — public entry point for C calling into script
- switch-statement handling (see `4ba40ba`)
- `fuzz-script` target and a script corpus
- KEY / CLI / HIST / SCRIPT size profiles in `tools/compactbuilds-docker.sh`

## How it differs from `exp_script_revisions` (0.4.1)

They are **not** a linear progression — they are divergent designs from the
same fork point. 0.4.1 is *less* aggressive about API removal:

| | `exp_argv_core` (0.4.0) | `exp_script_revisions` (0.4.1) |
| --- | --- | --- |
| `XelpBuf2Argv` | internal only | still public |
| `XELP_ARGV_MAX` | removed → `XELP_ARGV_CAP` | retained |
| `XELP_ENABLE_LINE_EDIT` | removed | retained |
| history flag | `XELP_ENABLE_CLI_HISTORY` | `XELP_ENABLE_HISTORY` |
| script engine | arena + vars + frames + eval + `XelpCallProc` | vars + frames + `_xelpEvalScript`, no arena init or public call API |
| `xelp.c` | ~2790 lines | ~1590 lines |
| tests | 249 units / 1008 cases | 54 units / 665 cases |

## Current state

- Merged with master 0.3.5 (commit `87fa8c8`), so it carries the issue #18
  fix and the `tests-unsigned-char` build
- `make validate` passes: 249 units, 1008 cases, 100% line / 97.17% branch
  coverage, under both signed and unsigned `char`
- Version is still `0.4.0` (`0x00000400`)
- **Not merge-ready.** The script design is still being evaluated and this
  line carries breaking API changes relative to 0.3.x

## Before writing more script code

See `dev/issues.md` for the pre-script engineering gates and
`dev/xelp_script_requirements.md` for normative requirements. The 0.5.x
plan in `dev/xelp_discussion_0.5.x.md` describes a tier structure that was
never branched — 0.5.x exists only as a plan.

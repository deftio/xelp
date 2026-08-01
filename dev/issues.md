# Must-Fix Issues Before xelp-script

Review date: 2026-05-16
Scope: current repo and docs excluding `pages/`, with focus on the v0.4 argc/argv CLI foundation.

These are not polish items. Treat them as gates. If these are not resolved, xelp-script will be built on top of ambiguous dispatch behavior, weak failure handling, and undefined lifetime rules.

## 1. `XelpParseXB()` Must Not Dispatch After argv Tokenization Failure

`XelpParseXB()` calls `_xelpBuf2Argv()` but does not check its return value before invoking a matched command or default handler.

Failure mode: if tokenization fails because the line is longer than `XELP_ARGVBUFSZ` or has more than `XELP_ARGV_MAX` tokens, handlers can be called with `argc == 0` and no valid `argv[0]`. That violates the v0.4 handler contract and can produce undefined behavior in ordinary user commands.

Why this is a hard blocker: xelp-script will generate more complex argument streams than humans typing at a prompt. Overflow and bad tokenization must be deterministic errors, not "call user code with broken argv and hope."

Must pass: tokenization failure prevents handler dispatch, sets `ths->mR[0]` to an error, and returns or preserves a meaningful error status. No command handler may ever be called with `argc == 0` for a matched non-empty command.

## 2. Tests Do Not Prove Handlers Are Skipped On argv Failure

The current argv overflow tests check observed `argc`, but a handler called with `argc == 0` can look the same as a handler not being called.

Failure mode: the most dangerous argv error path can regress while the unit suite still passes.

Why this is a hard blocker: a parser with false-positive tests is worse than an untested parser because it creates confidence around broken safety behavior.

Must pass: add explicit handler call-count tests for too many args and oversized argv buffer cases. Tests must prove both the error status and the fact that no user handler ran.

## 3. Fuzz Harnesses Are Stale After v0.4 API Changes

Fuzz harnesses must stay aligned with the current CLI handler signature (`argc`/`argv`) and the real dispatch tokenization path (public `XelpBuf2Argv` was removed — argv fuzzing rides `XelpParse`).

Failure mode if stale: fuzzing may not build, may not run in CI/release validation, or may not stress the tokenizer + `_xelpBuf2Argv` glue used in production dispatch.

Why this is a hard blocker: v0.4 moved argv tokenization into the core dispatch path. If fuzzing is stale, the most security-relevant parser surface is not being stressed.

Must pass: all fuzz harnesses compile against current headers with warnings treated as errors. They must exercise `XelpParse()`, `XelpParseKey()`, and current argc/argv dispatch behavior, including token overflow, quote handling, escapes, comments, semicolons, and malformed buffers.

## 4. argv Lifetime Is Not Documented Strongly Enough

Handler `argv[]` entries point into per-instance `mArgvBuf` scratch storage.

Failure mode: user code stores `argv[i]` after a handler returns, queues it for later output, or calls `XelpParse()` on the same instance and clobbers strings it still expects to be valid.

Why this is a hard blocker: `argc/argv` looks familiar to C programmers, but unlike hosted `main()`, these pointers are scratch-owned by xelp. If this is not explicit, real integrations will have latent use-after-scratch bugs.

Must pass: public docs and `AGENTS.md` state that argv pointers are transient and valid only until the handler returns or until the next same-instance parse/dispatch, whichever comes first. Any examples that persist an argument must copy it into user-owned storage.

## 5. `XELP_ARGV_MAX` Default Is Inconsistent In Docs

`src/xelpcfg.h` defaults `XELP_ARGV_MAX` to 8, while `AGENTS.md` says 16 in at least two places.

Failure mode: docs, tests, examples, and generated code assume different argument limits than the shipped header.

Why this is a hard blocker: this is a public stack-use and API behavior contract. xelp-script will make argument count matter more, not less. For the planned direction, 16 appears to be the more reasonable default, with tiny targets reducing it via `#define`.

Must pass: deliberately choose the source default, likely 16, and update all docs, tests, size notes, and examples to match. Document stack impact and how small targets reduce `XELP_ARGV_MAX`.

## 6. Size/Profile Tooling Still References Removed `XELP_ENABLE_ARGV`

`dev/size_profiles.sh` and `tools/compactbuilds-docker.sh` still include `XELP_ENABLE_ARGV` profiles or overrides even though v0.4 removed the flag.

Failure mode: size tables report meaningless profile differences, or future release notes claim savings/costs for a flag that no longer exists.

Why this is a hard blocker: xelp's credibility depends on size claims being boringly accurate. Dead feature flags in size tooling are unacceptable.

Must pass: remove `XELP_ENABLE_ARGV` as a profile axis and treat argv as part of `XELP_ENABLE_CLI`. If argv tuning is measured, measure `XELP_ARGV_MAX` and `XELP_ARGVBUFSZ`, not a dead flag.

## 7. Script Proposal Must Settle One Uniform Argument And Value Model

The draft script proposals diverge on `$1` vs `@1`, text expansion vs typed values, function syntax, and whether C command integration changes the handler ABI. Parenthesized calls also need a uniform argument rule: `(fn arg arg)` should evaluate its args, call `fn`, and collapse to exactly one returned value if it appears inside another command.

Failure mode: xelp-script ships with two half-models: text argv for some paths, typed cells for others, raw line access as an escape hatch, and ambiguous semantics for `(fn arg arg)`.

Why this is a hard blocker: argument processing is the language. If this is not one model, every built-in and every C integration point will carry special cases.

Must pass: pick one script source of truth before coding. Decide whether user-supplied C functions called in value position return their collapsed value through `mR[]`, a typed return cell, or a helper API. Decide whether user functions can access the raw original CLI/script line. If raw access exists, it must be a defined view in the same model, not a parallel dispatch mechanism.

## 8. xelp-script Call And Variable Stacks Must Be Specified

The current v0.4 CLI has one `mArgvBuf` per instance, but xelp-script is expected to add separate runtime buffers for proper call and variable stacks.

Failure mode: nested calls appear to work in simple examples but corrupt parent values, leak temporaries, overwrite locals, or produce target-dependent behavior when buffers fill.

Why this is a hard blocker: nested `()` evaluation is a core xelp-script design goal. The new script buffers are not an implementation detail; they are the memory model.

Must pass: define the script call stack, variable/value stack, temporary value lifetime, maximum nesting behavior, overflow results, and copy-vs-reference rules before implementing nested evaluation. Every buffer full case must have a deterministic error result and leave the instance in a usable state.

## 9. Script Return Semantics Must Be Defined Before Implementation

Current C commands return `XELPRESULT` and may set `mR[1..3]`. The script drafts also want typed returns, string returns, and status/truthiness.

Failure mode: `_if (read_adc 0) high` means different things depending on whether `read_adc` returns status, prints output, sets `R1`, or writes a typed return cell.

Why this is a hard blocker: script composition depends on return values. If value and status are blurred, scripts will be non-portable across user commands.

Must pass: define the script return channel, status channel, truthiness table, and mapping to `mR[]` before adding built-ins. Printed output must not accidentally become a return value.

## 10. xelp-script Built-In Namespace Must Separate Logical And Bitwise Operators

The draft script proposals use both logical operators such as `_and`, `_or`, `_not` and bitwise operators such as `_band`, `_bor`, `_xor`, `_shl`, or symbolic aliases.

Failure mode: a script that means "bitwise mask" is interpreted as "boolean truth," or a conditional uses a bitwise result as if it were normalized truth.

Why this is a hard blocker: embedded scripts need both boolean control flow and bit manipulation. Collapsing these into one operator family will create surprising behavior for status flags, register masks, GPIO bit fields, and conditionals.

Must pass: define a built-in namespace where language/control built-ins are protected, logical and bitwise operators are distinct, and symbolic aliases are either deferred or strictly mapped without ambiguity. Tests must cover logical truth values and bitmask behavior separately.

## 11. xelp-script Needs A Configurable Execution Limit

The proposed `_if` plus `_goto` model can create infinite loops.

Failure mode: a malformed or malicious script wedges the device's command surface forever.

Why this is a hard blocker: xelp targets firmware, bootloaders, and possibly ISR-adjacent command paths. Some users may intentionally want a `while(1)`-style construct, so the limit should be configurable rather than hard-coded. But unbounded execution must be a deliberate opt-in, not the silent default.

Must pass: add a compile-time or per-call statement/step budget for script execution, with a clear error result when exhausted. Provide an explicit `#define` or API option for unbounded execution when the application intentionally wants it.

## 12. Parentheses Need A Real Bounded Scanner, Not Ad Hoc String Tricks

Core v0.4 tokenization does not treat balanced parenthesized calls as single evaluated words. xelp-script plans to add this, but it must be specified before implementation.

Failure mode: nested calls work until quotes, comments, escapes, semicolons, or deeper nesting appear, then parsing splits words incorrectly or scans past the intended expression.

Why this is a hard blocker: `(fn arg arg)` is the proposed expression mechanism. If the scanner is weak, the language core is weak.

Must pass: define a bounded scanner for script words that handles literals, quotes, escapes, `$` variable references, `@` argument references, and balanced `(...)` subcalls. It must have maximum depth behavior, malformed-parenthesis errors, and tests for nesting plus quoted parens.

## 13. Raw CLI/Script Access Must Be A Deliberate API, Not A Back Door

Some user-supplied C functions may need access to the raw original command line or source span rather than processed `argc/argv`.

Failure mode: raw access gets added informally through global scratch pointers, mutable source assumptions, or special handler signatures that fragment the API.

Why this is a hard blocker: raw access is useful for commands like logging, pass-through, mini parsers, or payload capture. But if it bypasses the uniform argument model, script and CLI behavior will diverge.

Must pass: decide whether raw access exists. If it exists, define it as a stable span/view with explicit lifetime and const rules. Do not add a second public CLI handler signature unless the size and complexity cost is intentionally accepted.

## 14. Error Recovery Must Leave The Instance Usable

Current and future parser failures must not leave `XELP` with half-mutated cursor, argv, frame, mode, or stack state.

Failure mode: one bad command poisons the next command, corrupts history/line editing state, leaves a frame partially pushed, or makes later dispatch read stale data.

Why this is a hard blocker: embedded command surfaces live for months. They must tolerate garbage input, line noise, BLE fragmentation, user mistakes, and hostile test cases.

Must pass: define recovery invariants for CLI parse errors and script errors. After any parse/tokenize/runtime error, the instance must accept the next valid command. Add tests that send malformed input followed by known-good commands.

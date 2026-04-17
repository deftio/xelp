# Changelog

All notable changes to xelp will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

The version source of truth is `XELP_VERSION` in `src/xelp.h` (hex format).

## [Unreleased]

### Fixed
- Fixed 10 bugs across xelp.h, xelp.c, xelpcfg.h:
  - `XELPOutXB` macro parentheses
  - `XELP_XBGETC` macro read from wrong pointer
  - `_XOUTC` macro used GCC-only statement expressions
  - `XELPOut` with maxlen=0 printed nothing (now prints until null)
  - `XELPStr2Int` and `XELPParseNum` did not handle uppercase hex (A-F)
  - Duplicate `XELP_T_OK` macro definition
  - `XELP_STACK_OPS` / `XELP_STACK_MACHINE` name mismatch in xelpcfg.h
  - C89-incompatible `//` comment in xelpcfg.h
- Fixed 7 bugs in unit tests (unreachable code, wrong fields, stubs, etc.)
- Fixed all remaining `//` comments in src/ for C89 compliance

### Changed
- **API naming consistency**: all public functions now use `XELP` prefix
  - `XelpNumToks` -> `XELPNumToks`
  - `XelpParseNum` -> `XELPParseNum`
  - `XelpBufCmp` -> `XELPBufCmp`
- **Status code naming**: all uppercase per C convention
  - `XELP_W_Warn` -> `XELP_W_WARN`
  - `XELP_E_Err` -> `XELP_E_ERR`
  - `XELP_E_CmdBufFull` -> `XELP_E_CMDBUFFULL`
  - `XELP_E_CmdNotFound` -> `XELP_E_CMDNOTFOUND`
- Reorganized repository structure (pages/, docs/, tools/, dev/)
- Replaced defunct Travis CI with GitHub Actions CI
- Updated `.gitignore` for build artifacts
- Rewrote README.md
- `XELP_VERSION` in xelp.h is now the sole version source of truth

### Added
- 100% line coverage of xelp.c (207 test cases across 19 units)
- Stress and hardening tests for malformed input
- Makefile `coverage` target
- Markdown documentation: API reference, configuration guide, porting guide
- Docker cross-compilation tooling (`tools/crossbuild.sh`)
- Banner generator tool (`tools/generate_banner.py`)
- Release script (`tools/make_release.sh`)
- Bare-metal and multi-instance examples
- `CONTRIBUTING.md`, `CHANGELOG.md`

## [0.2.1] - 2024-06-05

### Added
- `XelpParseNum` function for safer string-to-integer conversion
- Arduino C example (`examples/arduino/`)
- `XelpBufCmp` buffer comparison with multiple comparison modes
- `XELPFindTok` token search function

### Changed
- Expanded unit test coverage

## [0.2.0] - 2024-02-25

### Added
- Initial public release on GitHub
- Char-at-a-time CLI parser with KEY, CLI, and THRU modes
- Tokenizer with quoted strings, escape sequences, comments
- Command dispatch for both single-key and CLI modes
- Multi-instance support with no global state
- No dynamic memory allocation
- Platform abstraction layer (5 function pointers)
- Compile-time feature selection via `xelpcfg.h`
- Posix example with ncurses
- Unit tests with jumpbug framework
- Support for 8-bit through 64-bit architectures

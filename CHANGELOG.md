# Changelog

All notable changes to xelp will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed
- Reorganized repository structure
  - Moved web assets (index.html, libs/) to `site/`
  - Moved build/utility scripts to `scripts/` and `tools/`
  - Moved visual QA report files to `tests/qa_visual_report/`
  - Moved extra example code to `examples/`
- Replaced defunct Travis CI with GitHub Actions CI
- Updated `.gitignore` to cover build artifacts, OS files, editor files
- Removed tracked build artifacts (.gcda, .DS_Store) from repository
- Removed obsolete files (old makefiles, naming brainstorm, code fragments, favicon zips)

### Added
- `VERSION` file as single source of truth for version string
- `release_management.md` documenting the build, test, and release workflow
- `CONTRIBUTING.md` with contribution guidelines
- `CHANGELOG.md` (this file)
- `.github/workflows/ci.yml` for automated testing on push/PR
- `tests/qa_visual_report/README.md` explaining retained visual report assets

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

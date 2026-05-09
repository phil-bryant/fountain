# Makefile Requirements

## Scope

Applies to repository task automation in `Makefile` for consolidated local build, test, run, analysis, and cleanup workflows.

R001  Statement: Expose discoverable consolidated developer entrypoints through a help target.
Design: `make help` lists supported targets and one-line purpose text for each (`build`, `test`, `run`, `sast`,
`sast-report`, `clean`).
Tests:
- Run `make help` and verify all documented targets are present with descriptions.

R005  Statement: Run non-UI repository tests through the primary test lane.
Design: `make test` depends on `build`, runs `ctest --test-dir build --output-on-failure`, then runs shell tests under
`tests/sh`.
Tests:
- Run `make test` and verify CMake build executes, ctest exits zero, and Bats tests execute.
- Force ctest failure and verify `make test` exits non-zero.

R010  Statement: Build the repository using deterministic CMake configure and build commands.
Design: `make build` runs `cmake -S . -B build` and `cmake --build build` in-order and fails on non-zero command exits.
Tests:
- Run `make build` and verify both configure and build commands are invoked in order.
- Force `cmake --build build` to fail and verify `make build` exits non-zero.

R015  Statement: Keep Makefile paths and artifact controls configurable through variables.
Design: Define variables for build directory, test lanes, SAST tooling commands, and run artifact paths near the file top.
Tests:
- Verify target commands use declared variables instead of repeated hardcoded literals.

R020  Statement: Launch a deterministic repository run target through Make.
Design: `make run` depends on `build`, checks configured executable path exists and is executable, then launches it with clear
guidance on failure.
Tests:
- Run `make run` with a valid executable path and verify launch command executes.
- Run `make run` with missing executable and verify explicit non-zero guidance output.

R030  Statement: Expose a consolidated SAST lane for repository security checks.
Design: `make sast` runs internal `_sast_shell`, `_sast_semgrep`, `_sast_clang_tidy`, and `_sast_secrets` lanes and reports
success only when all pass.
Tests:
- Run `make sast` with tool stubs and verify all `_sast_*` lanes execute.
- Force one lane to fail and verify `make sast` exits non-zero.

R035  Statement: Run shell-script static analysis through ShellCheck.
Design: `_sast_shell` fails clearly when `shellcheck` is unavailable; when present, it scans discovered `*.sh` files and emits
an explicit no-files message when none are found.
Tests:
- Run `_sast_shell` with `shellcheck` available and verify shell scripts are scanned.
- Remove `shellcheck` from `PATH` and verify `_sast_shell` fails non-zero with guidance.

R045  Statement: Run Semgrep through a repository SAST lane.
Design: `_sast_semgrep` fails clearly when `semgrep` is unavailable; otherwise runs `semgrep --config auto --error --quiet .`.
Tests:
- Run `_sast_semgrep` with stubbed semgrep and verify expected arguments.
- Remove `semgrep` from `PATH` and verify `_sast_semgrep` fails non-zero with guidance.

R050  Statement: Run clang-tidy static analysis across repository C++ sources.
Design: `_sast_clang_tidy` fails clearly when `clang-tidy` is unavailable; when present, it runs clang-tidy for discovered
`src/*.cpp` files with repository include paths.
Tests:
- Run `_sast_clang_tidy` with stubbed clang-tidy and verify C++ source invocation occurs.
- Remove `clang-tidy` from `PATH` and verify `_sast_clang_tidy` fails non-zero with guidance.

R055  Statement: Run repository secret scanning through gitleaks.
Design: `_sast_secrets` fails clearly when `gitleaks` is unavailable; when present, it runs
`gitleaks detect --source . --no-banner --redact --exit-code 1`.
Tests:
- Run `_sast_secrets` with stubbed gitleaks and verify expected detect arguments.
- Remove `gitleaks` from `PATH` and verify `_sast_secrets` fails non-zero with guidance.

R060  Statement: Expose a non-blocking extended SAST reporting lane.
Design: `make sast-report` runs `_sast_clang_tidy_report`; report command failures are printed and do not fail the target.
Tests:
- Run `make sast-report` and verify completion output is emitted.
- Force report clang-tidy failure and verify target remains successful.

R065  Statement: Keep blocking and report-only clang-tidy policy separation explicit.
Design: `_sast_clang_tidy` is blocking; `_sast_clang_tidy_report` is non-blocking and prints diagnostic context.
Tests:
- Verify blocking lane fails on clang-tidy failure.
- Verify report lane prints failure context but returns success.

R080  Statement: Remove generated local artifacts through a safe clean target.
Design: `make clean` moves generated artifacts to timestamped `~/.Trash` recovery locations using `mv`, never `rm`, and returns
success when targets are already absent.
Tests:
- Create build artifacts, run `make clean`, and verify artifacts are moved from workspace.
- Run `make clean` repeatedly and verify idempotent success.

R040  Statement: Declare orchestration targets as phony with operator-readable status output.
Design: `.PHONY` includes `help`, `build`, `test`, `run`, `sast`, `sast-report`, `clean`, and internal SAST helper
targets; each orchestration target emits concise status lines.
Tests:
- Verify all orchestration targets are present in `.PHONY`.
- Run each major lane and verify human-readable status output is emitted.

## Changelog

- 2026-05-08: Initial reverse-engineered requirements for Fountain `Makefile` target behavior.
- 2026-05-08: Mirrored email-style target-family contract and adapted to Fountain CMake/Bats workflow.

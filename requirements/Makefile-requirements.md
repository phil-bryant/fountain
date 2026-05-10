# Makefile Requirements

## Scope

Applies to repository task automation in `Makefile` for consolidated local build, test, run, analysis, and cleanup workflows.

R001  Statement: Expose discoverable consolidated developer entrypoints through a help target.
Design: `make help` lists supported targets and one-line purpose text for each (`build`, `test`, `run`, `sast`,
`clean`).
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
Design: Define variables for build directory, test lanes, SAST tooling commands/checks, and run artifact paths near the file top.
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
Design: `_sast_semgrep` fails clearly when `semgrep` is unavailable; otherwise runs `semgrep --config auto --error  .`.
Tests:
- Run `_sast_semgrep` with stubbed semgrep and verify expected arguments.
- Remove `semgrep` from `PATH` and verify `_sast_semgrep` fails non-zero with guidance.

R046  Statement: Preserve Semgrep findings visibility in SAST output.
Design: `_sast_semgrep` MUST NOT pass `--quiet` so console output includes scan context and findings summary.
Tests:
- Run `_sast_semgrep` with stubbed semgrep and verify invocation does not include `--quiet`.
- Verify `_sast_semgrep` still includes expected baseline arguments (`--config auto --error .`).

R047  Statement: Print explanatory headers before each SAST tool invocation.
Design: `make sast` lanes MUST print a bounded ASCII header before each tool run (`shellcheck`, `semgrep`, `clang-tidy`, and
`gitleaks`) including tool name, a short explainer line, and official documentation URL.
Tests:
- Run `make sast` with tool stubs and verify output contains boxed headers for all SAST tools.
- Verify each boxed header includes both an explainer line and an official URL line.

R050  Statement: Enforce zero-warning clang-tidy policy for first-party C++.
Design: `_sast_clang_tidy` fails clearly when `clang-tidy` is unavailable; when present, it MUST fail on first-party (`src/`,
`include/`) clang-tidy warnings without suppression filters and uses explicit check globs plus repository include paths.
Tests:
- Run `_sast_clang_tidy` with stubbed first-party warning output and verify the lane exits non-zero.
- Remove `clang-tidy` from `PATH` and verify `_sast_clang_tidy` fails non-zero with guidance.

R051  Statement: Keep third-party clang-tidy scanning while constraining suppression scope.
Design: clang-tidy output from third-party paths (for example `build/_deps`) is still scanned and reported, but only
`portability-*`, `performance-*`, and `bugprone-*` warning families may be suppressed for third-party paths.
Tests:
- Run `_sast_clang_tidy` with mixed third-party warning families and verify only portability/performance/bugprone warnings are
filtered.
- Verify non-filtered third-party warning families remain visible in clang-tidy output.

R052  Statement: Prohibit broad clang-tidy suppression patterns in SAST lanes.
Design: Makefile clang-tidy policy MUST NOT use broad suppression patterns that hide all third-party diagnostics; filtering must
be narrowly scoped to the allowed third-party warning families.
Tests:
- Verify Makefile clang-tidy command does not use a broad suppression regex that drops all third-party lines.
- Verify filtered output logic is anchored to third-party paths plus the three allowed warning families.

R053  Statement: Emit explicit first-party clang-tidy summary in the blocking lane.
Design: `_sast_clang_tidy` MUST print deterministic summary lines for first-party warning count and first-party suppression
count (`NOLINT`) so operators do not infer first-party status from global suppression totals.
Tests:
- Run `_sast_clang_tidy` with stubbed clang-tidy output and verify summary lines include first-party warning and suppression counts.
- Verify summary lines are present when counts are zero and when counts are non-zero.

R054  Statement: Reject first-party suppression-based clang-tidy compliance.
Design: `_sast_clang_tidy` MUST fail when first-party diagnostics are suppressed via `NOLINT` markers, even when first-party
warning count is otherwise zero.
Tests:
- Run `_sast_clang_tidy` with first-party warning lines containing `NOLINT` and verify the lane exits non-zero.
- Run `_sast_clang_tidy` with only third-party `NOLINT` lines and verify blocking failure does not trigger from those lines alone.

R055  Statement: Run repository secret scanning through gitleaks.
Design: `_sast_secrets` fails clearly when `gitleaks` is unavailable; when present, it runs
`gitleaks detect --source . --no-banner --redact --exit-code 1`.
Tests:
- Run `_sast_secrets` with stubbed gitleaks and verify expected detect arguments.
- Remove `gitleaks` from `PATH` and verify `_sast_secrets` fails non-zero with guidance.

R060  Statement: Expose a non-blocking extended SAST reporting lane.
Design: `make sast` includes `_sast_clang_tidy_report`; report command failures are printed and do not fail the target.
Tests:
- Run `make sast` and verify non-blocking report output is emitted.
- Force report clang-tidy failure and verify `make sast` remains successful.

R065  Statement: Keep blocking and report-only clang-tidy policy separation explicit.
Design: `_sast_clang_tidy` is blocking; `_sast_clang_tidy_report` is non-blocking and prints diagnostic context.
Tests:
- Verify blocking lane fails on clang-tidy failure.
- Verify `make sast` report lane prints failure context while overall target remains successful.

R080  Statement: Remove generated local artifacts through a safe clean target.
Design: `make clean` moves generated artifacts to timestamped `~/.Trash` recovery locations using `mv`, never `rm`, and returns
success when targets are already absent.
Tests:
- Create build artifacts, run `make clean`, and verify artifacts are moved from workspace.
- Run `make clean` repeatedly and verify idempotent success.

R040  Statement: Declare orchestration targets as phony with operator-readable status output.
Design: `.PHONY` includes `help`, `build`, `test`, `run`, `sast`, `clean`, and internal SAST helper
targets; each orchestration target emits concise status lines.
Tests:
- Verify all orchestration targets are present in `.PHONY`.
- Run each major lane and verify human-readable status output is emitted.

## Changelog

- 2026-05-08: Initial reverse-engineered requirements for Fountain `Makefile` target behavior.
- 2026-05-08: Mirrored email-style target-family contract and adapted to Fountain CMake/Bats workflow.
- 2026-05-10: Added clang-tidy proof summary and first-party NOLINT-blocking requirements (R053, R054).

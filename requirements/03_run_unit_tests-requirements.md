# Run Unit Tests Requirements

## Scope

Applies to `03_run_unit_tests.sh`.

R001  Statement: Run in strict shell mode for deterministic unit-test execution.
Design: Use `umask 007` and `set -euo pipefail` at script start.
Tests:
- Verify script contains strict-mode declarations.

R005  Statement: Execute from the repository root regardless of caller directory.
Design: Resolve the script directory and change to it before invoking repository tooling.
Tests:
- Run from another directory and verify the delegated make command runs in the repository root.

R010  Statement: Delegate unit-test execution to the Makefile test lane.
Design: Invoke `make test` as the only unit-test implementation.
Tests:
- Use a stub `make` and verify the `test` target is requested.

R015  Statement: Emit concise success guidance after unit tests pass.
Design: Print a clear pass message only after `make test` succeeds.
Tests:
- Verify successful execution prints a unit-test pass message.

R020  Statement: Propagate Makefile test failures.
Design: Rely on strict mode so a failing `make test` exits the wrapper non-zero before pass output.
Tests:
- Use a failing stub `make` and verify the wrapper exits non-zero without pass output.

## Changelog

- 2026-05-13: Initial unit-test-wrapper requirements.

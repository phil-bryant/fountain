# Check Lints Requirements

## Scope

Applies to `02_check_lints.sh`.

R001  Statement: Run in strict shell mode for deterministic lint checking.
Design: Use `umask 007` and `set -euo pipefail` at script start.
Tests:
- Verify script contains strict-mode declarations.

R005  Statement: Execute from the repository root regardless of caller directory.
Design: Resolve the script directory and change to it before invoking repository tooling.
Tests:
- Run from another directory and verify the delegated make command runs in the repository root.

R010  Statement: Delegate lint checking to the Makefile lint lane.
Design: Invoke `make lint` as the only lint implementation.
Tests:
- Use a stub `make` and verify the `lint` target is requested.

R015  Statement: Emit concise success guidance after lint checks pass.
Design: Print a clear pass message only after `make lint` succeeds.
Tests:
- Verify successful execution prints a lint pass message.

R020  Statement: Propagate Makefile lint failures.
Design: Rely on strict mode so a failing `make lint` exits the wrapper non-zero before pass output.
Tests:
- Use a failing stub `make` and verify the wrapper exits non-zero without pass output.

## Changelog

- 2026-05-13: Initial lint-wrapper requirements.

# Run SAST Requirements

## Scope

Applies to `04_run_sast.sh`.

R001  Statement: Run in strict shell mode for deterministic SAST execution.
Design: Use `umask 007` and `set -euo pipefail` at script start.
Tests:
- Verify script contains strict-mode declarations.

R005  Statement: Execute from the repository root regardless of caller directory.
Design: Resolve the script directory and change to it before invoking repository tooling.
Tests:
- Run from another directory and verify the delegated make command runs in the repository root.

R010  Statement: Delegate SAST execution to the Makefile SAST lane.
Design: Invoke `make sast` as the only SAST implementation.
Tests:
- Use a stub `make` and verify the `sast` target is requested.

R015  Statement: Emit concise success guidance after SAST checks pass.
Design: Print a clear pass message only after `make sast` succeeds.
Tests:
- Verify successful execution prints a SAST pass message.

R020  Statement: Propagate Makefile SAST failures.
Design: Rely on strict mode so a failing `make sast` exits the wrapper non-zero before pass output.
Tests:
- Use a failing stub `make` and verify the wrapper exits non-zero without pass output.

## Changelog

- 2026-05-13: Initial SAST-wrapper requirements.

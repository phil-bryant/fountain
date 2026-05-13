# Start Heartbeat Requirements

## Scope

Applies to `05_start_heartbeat.sh`.

R001  Statement: Run in strict shell mode for deterministic failures.
Design: Use `umask 007` and `set -euo pipefail` at script start.
Tests:
- Verify script contains strict-mode declarations.

R005  Statement: Require explicit rollout target install identifier.
Design: Fail non-zero with guidance unless `--install-id <value>` is supplied.
Tests:
- Run without `--install-id` and verify explicit failure output.

R010  Statement: Provide default heartbeat cadence and event identity values.
Design: Default to `interval_seconds=900`, `event_name=fountain.heartbeat`, and `component=fountain.runtime`.
Tests:
- Run with only `--install-id` and verify state file contains defaults.

R015  Statement: Allow operator overrides for heartbeat settings.
Design: Support `--interval-seconds`, `--event-name`, `--component`, and `--state-file` arguments.
Tests:
- Run with custom arguments and verify persisted values.

R020  Statement: Persist heartbeat activation state atomically.
Design: Write state to temporary file then move into final state-file path.
Tests:
- Run script and verify final state file exists with expected key/value rows.

R025  Statement: Emit operator guidance for integrating activation state.
Design: Print concise success output including state file path and next-step guidance.
Tests:
- Verify success output includes state path and next-step language.

R030  Statement: Keep start operation idempotent across reruns.
Design: Rerunning with updated arguments replaces the state file values without duplicate records.
Tests:
- Run twice with different interval values and verify final state reflects the latest value.

## Changelog

- 2026-05-11: Initial heartbeat start-script requirements.

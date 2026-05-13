# Verify Heartbeat Requirements

## Scope

Applies to `06_verify_heartbeat.sh`.

R001  Statement: Run in strict shell mode for deterministic verification behavior.
Design: Use `umask 007` and `set -euo pipefail` at script start.
Tests:
- Verify script contains strict-mode declarations.

R005  Statement: Fail clearly when heartbeat state file is missing.
Design: Accept `--state-file` path and fail non-zero with explicit message when absent.
Tests:
- Run with missing state path and verify explicit non-zero guidance.

R010  Statement: Validate required heartbeat keys and active state.
Design: Require non-empty values for `heartbeat_event_name`, `heartbeat_component`, and `heartbeat_target_install_id`; require `heartbeat_enabled=1`.
Tests:
- Provide malformed state values and verify non-zero failure.
- Provide valid active state and verify pass.

R015  Statement: Validate heartbeat interval semantics.
Design: Require `heartbeat_interval_seconds` to be positive integer.
Tests:
- Provide non-numeric or non-positive interval and verify non-zero failure.

R020  Statement: Validate rollout target against expected install identifier when provided.
Design: Support `--expect-install-id <value>` and fail when state install id differs.
Tests:
- Run with mismatch and verify explicit non-zero output.

R025  Statement: Emit concise verification summary output on success.
Design: Print active status summary including install id, interval, event name, and component.
Tests:
- Verify success output includes those summary fields.

## Changelog

- 2026-05-11: Initial heartbeat verify-script requirements.

# Stop Heartbeat Requirements

## Scope

Applies to `04_stop_heartbeat.sh`.

R001  Statement: Run in strict shell mode for deterministic stop behavior.
Design: Use `umask 007` and `set -euo pipefail` at script start.
Tests:
- Verify script contains strict-mode declarations.

R005  Statement: Fail clearly when heartbeat state file is missing.
Design: Accept `--state-file` path and return non-zero with explicit message when absent.
Tests:
- Run with missing state file and verify explicit failure output.

R010  Statement: Disable heartbeat while preserving existing configuration fields.
Design: Rewrite state as `heartbeat_enabled=0` and preserve install id, interval, event name, and component.
Tests:
- Stop from an active state and verify preserved values plus disabled state.

R015  Statement: Preserve prior state snapshot safely before replacement.
Design: Move prior state file to a timestamped archive location under configurable `--archive-root` (default `~/.Trash`), then write replacement state.
Tests:
- Run stop and verify archived copy exists in expected archive root.

R020  Statement: Keep stop operation idempotent.
Design: If state already indicates disabled heartbeat, print "already stopped" guidance and exit success without rewriting archives.
Tests:
- Run stop against already disabled state and verify zero exit with "already stopped" output.

R025  Statement: Emit concise operator guidance after stop.
Design: Print stop confirmation and archived state location when stop transitions from active to disabled.
Tests:
- Verify successful stop output includes both state path and archive path context.

## Changelog

- 2026-05-11: Initial heartbeat stop-script requirements.

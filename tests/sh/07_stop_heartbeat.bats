#!/usr/bin/env bats

setup() {
  export REPO_ROOT
  REPO_ROOT="$(cd "${BATS_TEST_DIRNAME}/../.." && pwd)"
  export SCRIPT_PATH="${REPO_ROOT}/07_stop_heartbeat.sh"
  export TMP_ROOT
  TMP_ROOT="$(mktemp -d)"
  export STATE_FILE="${TMP_ROOT}/heartbeat.state"
  export ARCHIVE_ROOT="${TMP_ROOT}/archive"
  export PID_FILE="${TMP_ROOT}/heartbeat.pid"
  export DATABASE_PATH="${TMP_ROOT}/heartbeat.sqlite3"
}

write_state() {
  cat > "${STATE_FILE}" <<EOF
heartbeat_enabled=$1
heartbeat_interval_seconds=900
heartbeat_event_name=fountain.heartbeat
heartbeat_component=fountain.runtime
heartbeat_target_install_id=install-a
heartbeat_started_at_utc=2026-05-11T00:00:00Z
heartbeat_pid_file=${PID_FILE}
heartbeat_database_path=${DATABASE_PATH}
EOF
}

@test "R001: script uses strict fail-fast mode" {
  #R001: Strict shell mode traceability.
  run rg "set -euo pipefail" "${SCRIPT_PATH}"
  [ "$status" -eq 0 ]
}

@test "R005: fails when state file is missing" {
  #R005: Missing state-file failure traceability.
  run bash "${SCRIPT_PATH}" --state-file "${STATE_FILE}" --archive-root "${ARCHIVE_ROOT}"
  [ "$status" -ne 0 ]
  [[ "${output}" == *"Heartbeat state file not found"* ]]
}

@test "R010,R015,R025: stops heartbeat, archives previous state, and emits guidance" {
  #R010: Disable and preserve configuration traceability.
  #R015: Timestamped archive move traceability.
  #R025: Stop confirmation output traceability.
  write_state "1"
  touch "${DATABASE_PATH}"
  sleep 10 &
  helper_pid=$!
  echo "${helper_pid}" > "${PID_FILE}"
  run bash "${SCRIPT_PATH}" --state-file "${STATE_FILE}" --archive-root "${ARCHIVE_ROOT}"
  [ "$status" -eq 0 ]
  [[ "${output}" == *"archived_state="* ]]
  [[ "${output}" == *"runner_process_stopped=true"* ]]
  run rg "^heartbeat_enabled=0$" "${STATE_FILE}"
  [ "$status" -eq 0 ]
  run rg "^heartbeat_target_install_id=install-a$" "${STATE_FILE}"
  [ "$status" -eq 0 ]
  [ -d "${ARCHIVE_ROOT}" ]
}

@test "R020: stop remains idempotent when already disabled" {
  #R020: Idempotent already-stopped handling traceability.
  write_state "0"
  run bash "${SCRIPT_PATH}" --state-file "${STATE_FILE}" --archive-root "${ARCHIVE_ROOT}"
  [ "$status" -eq 0 ]
  [[ "${output}" == *"Heartbeat already stopped."* ]]
}

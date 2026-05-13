#!/usr/bin/env bats

setup() {
  export REPO_ROOT
  REPO_ROOT="$(cd "${BATS_TEST_DIRNAME}/../.." && pwd)"
  export SCRIPT_PATH="${REPO_ROOT}/02_start_heartbeat.sh"
  export TMP_ROOT
  TMP_ROOT="$(mktemp -d)"
  export STATE_FILE="${TMP_ROOT}/heartbeat.state"
  export PID_FILE="${TMP_ROOT}/heartbeat.pid"
  export DATABASE_PATH="${TMP_ROOT}/heartbeat.sqlite3"
  export LOG_FILE="${TMP_ROOT}/heartbeat.log"
  export RUNNER_PATH="${TMP_ROOT}/runner.sh"
  cat > "${RUNNER_PATH}" <<'EOF'
#!/bin/bash
while true; do
  sleep 1
done
EOF
  chmod +x "${RUNNER_PATH}"
}

teardown() {
  if [ -f "${PID_FILE}" ]; then
    runner_pid="$(awk 'NR==1 {print $1}' "${PID_FILE}")"
    if [ -n "${runner_pid}" ] && kill -0 "${runner_pid}" >/dev/null 2>&1; then
      kill "${runner_pid}"
    fi
  fi
}

@test "R001: script uses strict fail-fast mode" {
  #R001: Strict shell mode traceability.
  run rg "set -euo pipefail" "${SCRIPT_PATH}"
  [ "$status" -eq 0 ]
}

@test "R005: fails clearly when install id is missing" {
  #R005: Required install-id argument traceability.
  run bash "${SCRIPT_PATH}" --state-file "${STATE_FILE}" --runner-path "${RUNNER_PATH}" --pid-file "${PID_FILE}" --database-path "${DATABASE_PATH}" --log-file "${LOG_FILE}"
  [ "$status" -ne 0 ]
  [[ "${output}" == *"Missing required --install-id"* ]]
}

@test "R010,R020,R025: writes default heartbeat state and prints guidance" {
  #R010: Default interval and event identity traceability.
  #R020: Atomic state persistence traceability.
  #R025: Success guidance output traceability.
  run bash "${SCRIPT_PATH}" --install-id "install-a" --state-file "${STATE_FILE}" --runner-path "${RUNNER_PATH}" --pid-file "${PID_FILE}" --database-path "${DATABASE_PATH}" --log-file "${LOG_FILE}"
  [ "$status" -eq 0 ]
  [ -f "${STATE_FILE}" ]
  [ -f "${PID_FILE}" ]
  run rg "^heartbeat_enabled=1$" "${STATE_FILE}"
  [ "$status" -eq 0 ]
  run rg "^heartbeat_interval_seconds=900$" "${STATE_FILE}"
  [ "$status" -eq 0 ]
  run rg "^heartbeat_event_name=fountain.heartbeat$" "${STATE_FILE}"
  [ "$status" -eq 0 ]
  run rg "^heartbeat_component=fountain.runtime$" "${STATE_FILE}"
  [ "$status" -eq 0 ]
  run rg "^heartbeat_target_install_id=install-a$" "${STATE_FILE}"
  [ "$status" -eq 0 ]
  run bash "${SCRIPT_PATH}" --install-id "install-a" --state-file "${STATE_FILE}" --runner-path "${RUNNER_PATH}" --pid-file "${PID_FILE}" --database-path "${DATABASE_PATH}" --log-file "${LOG_FILE}"
  [[ "${output}" == *"Next step: run ./03_verify_heartbeat.sh"* ]]
}

@test "R015,R030: supports overrides and remains idempotent on rerun" {
  #R015: Override arguments traceability.
  #R030: Rerun idempotent replacement traceability.
  run bash "${SCRIPT_PATH}" --install-id "install-a" --state-file "${STATE_FILE}" --interval-seconds 600 --event-name "a.b" --component "c.d" --runner-path "${RUNNER_PATH}" --pid-file "${PID_FILE}" --database-path "${DATABASE_PATH}" --log-file "${LOG_FILE}"
  [ "$status" -eq 0 ]
  run bash "${SCRIPT_PATH}" --install-id "install-a" --state-file "${STATE_FILE}" --interval-seconds 1200 --runner-path "${RUNNER_PATH}" --pid-file "${PID_FILE}" --database-path "${DATABASE_PATH}" --log-file "${LOG_FILE}"
  [ "$status" -eq 0 ]
  run rg "^heartbeat_interval_seconds=1200$" "${STATE_FILE}"
  [ "$status" -eq 0 ]
}

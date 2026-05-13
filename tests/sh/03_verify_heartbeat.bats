#!/usr/bin/env bats

setup() {
  export REPO_ROOT
  REPO_ROOT="$(cd "${BATS_TEST_DIRNAME}/../.." && pwd)"
  export SCRIPT_PATH="${REPO_ROOT}/03_verify_heartbeat.sh"
  export TMP_ROOT
  TMP_ROOT="$(mktemp -d)"
  export STATE_FILE="${TMP_ROOT}/heartbeat.state"
  export PID_FILE="${TMP_ROOT}/heartbeat.pid"
  export DATABASE_PATH="${TMP_ROOT}/heartbeat.sqlite3"
  export STUB_BIN="${TMP_ROOT}/bin"
  mkdir -p "${STUB_BIN}"
  cat > "${STUB_BIN}/sqlite3" <<'EOF'
#!/bin/bash
echo 1
EOF
  chmod +x "${STUB_BIN}/sqlite3"
}

write_state() {
  cat > "${STATE_FILE}" <<EOF
heartbeat_enabled=$1
heartbeat_interval_seconds=$2
heartbeat_event_name=$3
heartbeat_component=$4
heartbeat_target_install_id=$5
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
  run env PATH="${STUB_BIN}:/usr/bin:/bin" bash "${SCRIPT_PATH}" --state-file "${STATE_FILE}"
  [ "$status" -ne 0 ]
  [[ "${output}" == *"Heartbeat state file not found"* ]]
}

@test "R010,R015: fails malformed active state and invalid interval semantics" {
  #R010: Required key and active-state validation traceability.
  #R015: Interval validation traceability.
  write_state "0" "abc" "fountain.heartbeat" "fountain.runtime" "install-a"
  touch "${DATABASE_PATH}"
  sleep 5 &
  helper_pid=$!
  echo "${helper_pid}" > "${PID_FILE}"
  run env PATH="${STUB_BIN}:/usr/bin:/bin" bash "${SCRIPT_PATH}" --state-file "${STATE_FILE}"
  kill "${helper_pid}"
  [ "$status" -ne 0 ]
  [[ "${output}" == *"Heartbeat is not active"* ]]
}

@test "R020,R025: validates expected install id and prints success summary" {
  #R020: Expected install-id comparison traceability.
  #R025: Success summary traceability.
  write_state "1" "900" "fountain.heartbeat" "fountain.runtime" "install-a"
  touch "${DATABASE_PATH}"
  sleep 5 &
  helper_pid=$!
  echo "${helper_pid}" > "${PID_FILE}"
  run env PATH="${STUB_BIN}:/usr/bin:/bin" bash "${SCRIPT_PATH}" --state-file "${STATE_FILE}" --expect-install-id "install-b"
  [ "$status" -ne 0 ]
  [[ "${output}" == *"does not match"* ]]
  run env PATH="${STUB_BIN}:/usr/bin:/bin" bash "${SCRIPT_PATH}" --state-file "${STATE_FILE}" --expect-install-id "install-a"
  kill "${helper_pid}"
  [ "$status" -eq 0 ]
  [[ "${output}" == *"install_id=install-a"* ]]
  [[ "${output}" == *"interval_seconds=900"* ]]
}

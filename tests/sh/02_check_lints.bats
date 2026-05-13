#!/usr/bin/env bats

setup() {
  export REPO_ROOT
  REPO_ROOT="$(cd "${BATS_TEST_DIRNAME}/../.." && pwd)"
  export SCRIPT_PATH="${REPO_ROOT}/02_check_lints.sh"
  export TMP_ROOT
  TMP_ROOT="$(mktemp -d)"
  export STUB_BIN="${TMP_ROOT}/bin"
  export MAKE_LOG="${TMP_ROOT}/make.log"
  mkdir -p "${STUB_BIN}" "${TMP_ROOT}/caller"
}

create_make_stub() {
  local exit_code="$1"
  cat > "${STUB_BIN}/make" <<EOF
#!/bin/bash
printf 'PWD=%s\n' "\$PWD" >> "\${MAKE_LOG}"
printf 'ARGS=%s\n' "\$*" >> "\${MAKE_LOG}"
exit ${exit_code}
EOF
  chmod +x "${STUB_BIN}/make"
}

@test "R001: script uses strict fail-fast mode" {
  #R001: Strict shell mode traceability.
  run rg "set -euo pipefail" "${SCRIPT_PATH}"
  [ "$status" -eq 0 ]
}

@test "R005,R010,R015: runs make lint from repo root and prints success" {
  #R005: Repository-root execution traceability.
  #R010: make lint delegation traceability.
  #R015: Success guidance traceability.
  create_make_stub 0
  cd "${TMP_ROOT}/caller"
  run env PATH="${STUB_BIN}:/usr/bin:/bin" MAKE_LOG="${MAKE_LOG}" "${SCRIPT_PATH}"
  [ "$status" -eq 0 ]
  [[ "${output}" == *"Lint checks passed"* ]]
  run rg "^PWD=${REPO_ROOT}$" "${MAKE_LOG}"
  [ "$status" -eq 0 ]
  run rg "^ARGS=lint$" "${MAKE_LOG}"
  [ "$status" -eq 0 ]
}

@test "R020: propagates make lint failures before success output" {
  #R020: make failure propagation traceability.
  create_make_stub 7
  run env PATH="${STUB_BIN}:/usr/bin:/bin" MAKE_LOG="${MAKE_LOG}" "${SCRIPT_PATH}"
  [ "$status" -eq 7 ]
  [[ "${output}" != *"Lint checks passed"* ]]
}

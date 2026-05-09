#!/usr/bin/env bats

setup() {
  export REAL_HOME="${HOME}"
  mkdir -p "${REAL_HOME}/.Trash"
  export REPO_ROOT
  REPO_ROOT="$(cd "${BATS_TEST_DIRNAME}/../.." && pwd)"
  export TMP_ROOT
  TMP_ROOT="$(mktemp -d)"
  export SANDBOX="${TMP_ROOT}/sandbox"
  export STUB_BIN="${TMP_ROOT}/bin"
  export LOG_FILE="${TMP_ROOT}/tool.log"
  mkdir -p "${SANDBOX}" "${STUB_BIN}" "${SANDBOX}/tests/sh"
  cp "${REPO_ROOT}/Makefile" "${SANDBOX}/Makefile"
  export HOME="${TMP_ROOT}/home"
  mkdir -p "${HOME}/.Trash"
}

teardown() {
  :
}

create_stub() {
  local name="$1" body="$2"
  cat > "${STUB_BIN}/${name}" <<EOF
#!/bin/bash
${body}
EOF
  chmod +x "${STUB_BIN}/${name}"
}

create_common_stubs() {
  create_stub "rg" "if [ \"\$1\" = \"--files\" ] && [ \"\$2\" = \"src\" ]; then echo src/stub.cpp; exit 0; fi; echo tests/sh/example.bats; exit 0"
  create_stub "cmake" "echo cmake \"\$@\" >> \"${LOG_FILE}\"; exit 0"
  create_stub "ctest" "echo ctest \"\$@\" >> \"${LOG_FILE}\"; exit 0"
  create_stub "bats" "echo bats \"\$@\" >> \"${LOG_FILE}\"; exit 0"
  create_stub "shellcheck" "echo shellcheck \"\$@\" >> \"${LOG_FILE}\"; exit 0"
  create_stub "semgrep" "echo semgrep \"\$@\" >> \"${LOG_FILE}\"; exit 0"
  create_stub "clang-tidy" "echo clang-tidy \"\$@\" >> \"${LOG_FILE}\"; exit 0"
  create_stub "gitleaks" "echo gitleaks \"\$@\" >> \"${LOG_FILE}\"; exit 0"
  cat > "${SANDBOX}/tests/sh/example.bats" <<'EOF'
#!/usr/bin/env bats
@test "fixture" { [ 1 -eq 1 ]; }
EOF
  chmod +x "${SANDBOX}/tests/sh/example.bats"
  cat > "${SANDBOX}/00_verify_requirements_traceability.sh" <<'EOF'
#!/bin/bash
exit 0
EOF
  cat > "${SANDBOX}/01_install_prerequisites.sh" <<'EOF'
#!/bin/bash
exit 0
EOF
  chmod +x "${SANDBOX}/00_verify_requirements_traceability.sh" "${SANDBOX}/01_install_prerequisites.sh"
}

@test "R001,R040: help lists required target entrypoints" {
  #R001 #R040
  run env PATH="${STUB_BIN}:/usr/bin:/bin" make -f "${SANDBOX}/Makefile" -C "${SANDBOX}" help
  [ "$status" -eq 0 ]
  [[ "$output" == *"build"* ]]
  [[ "$output" == *"test"* ]]
  [[ "$output" == *"run"* ]]
  [[ "$output" == *"sast"* ]]
  [[ "$output" == *"sast-report"* ]]
  [[ "$output" == *"clean"* ]]
}

@test "R010,R015: build invokes cmake configure then build" {
  #R010 #R015
  create_common_stubs
  run env PATH="${STUB_BIN}:/usr/bin:/bin" make -f "${SANDBOX}/Makefile" -C "${SANDBOX}" build
  [ "$status" -eq 0 ]
  run rg "^cmake -S \\. -B build$" "${LOG_FILE}"
  [ "$status" -eq 0 ]
  run rg "^cmake --build build$" "${LOG_FILE}"
  [ "$status" -eq 0 ]
}

@test "R005: test lane runs ctest and bats" {
  #R005
  create_common_stubs
  run env PATH="${STUB_BIN}:/usr/bin:/bin" make -f "${SANDBOX}/Makefile" -C "${SANDBOX}" test
  [ "$status" -eq 0 ]
  run rg "^ctest --test-dir build --output-on-failure$" "${LOG_FILE}"
  [ "$status" -eq 0 ]
  run rg "^bats tests/sh/example.bats$" "${LOG_FILE}"
  [ "$status" -eq 0 ]
}

@test "R020: run fails with guidance when executable is missing" {
  #R020
  create_common_stubs
  run env PATH="${STUB_BIN}:/usr/bin:/bin" make -f "${SANDBOX}/Makefile" -C "${SANDBOX}" run
  [ "$status" -ne 0 ]
  [[ "$output" == *"Missing executable"* ]]
}

@test "R030,R035,R045,R050,R055: sast runs all blocking lanes" {
  #R030 #R035 #R045 #R050 #R055
  create_common_stubs
  run env PATH="${STUB_BIN}:/usr/bin:/bin" make -f "${SANDBOX}/Makefile" -C "${SANDBOX}" sast
  [ "$status" -eq 0 ]
  run rg "^shellcheck 00_verify_requirements_traceability\\.sh 01_install_prerequisites\\.sh$" "${LOG_FILE}"
  [ "$status" -eq 0 ]
  run rg "^semgrep --config auto --error --quiet \\.$" "${LOG_FILE}"
  [ "$status" -eq 0 ]
  run rg "^clang-tidy src/.+" "${LOG_FILE}"
  [ "$status" -eq 0 ]
  run rg "^gitleaks detect --source \\. --no-banner --redact --exit-code 1$" "${LOG_FILE}"
  [ "$status" -eq 0 ]
}

@test "R060,R065: sast-report is non-blocking for clang-tidy failures" {
  #R060 #R065
  create_common_stubs
  create_stub "clang-tidy" "echo clang-tidy \"\$@\" >> \"${LOG_FILE}\"; exit 2"
  run env PATH="${STUB_BIN}:/usr/bin:/bin" make -f "${SANDBOX}/Makefile" -C "${SANDBOX}" sast-report
  [ "$status" -eq 0 ]
  [[ "$output" == *"Completed non-blocking report lane"* ]]
}

@test "R080: clean moves artifacts into Trash and stays idempotent" {
  #R080
  create_common_stubs
  mkdir -p "${SANDBOX}/build"
  mkdir -p "${SANDBOX}/src"
  touch "${SANDBOX}/src/stub.cpp"
  touch "${SANDBOX}/CMakeCache.txt"
  run env PATH="${STUB_BIN}:/usr/bin:/bin" HOME="${HOME}" make -f "${SANDBOX}/Makefile" -C "${SANDBOX}" clean
  [ "$status" -eq 0 ]
  [ ! -e "${SANDBOX}/build" ]
  [ ! -e "${SANDBOX}/CMakeCache.txt" ]
  run env PATH="${STUB_BIN}:/usr/bin:/bin" HOME="${HOME}" make -f "${SANDBOX}/Makefile" -C "${SANDBOX}" clean
  [ "$status" -eq 0 ]
}

#!/usr/bin/env bats

setup() {
  export REPO_ROOT
  REPO_ROOT="$(cd "${BATS_TEST_DIRNAME}/../.." && pwd)"
  export SCRIPT_PATH="${REPO_ROOT}/01_install_prerequisites.sh"
  export TMP_ROOT
  TMP_ROOT="$(mktemp -d)"
  export STUB_BREW_PREFIX
  STUB_BREW_PREFIX="${TMP_ROOT}/homebrew"
  export STUB_BIN="${TMP_ROOT}/bin"
  mkdir -p "${STUB_BREW_PREFIX}/bin" "${STUB_BREW_PREFIX}/opt/llvm/bin"
  mkdir -p "${STUB_BIN}"
}

teardown() {
  rm -rf "${TMP_ROOT}"
}

create_common_toolchain_stubs() {
  cat > "${STUB_BIN}/clang++" <<'EOF'
#!/bin/bash
exit 0
EOF
  chmod +x "${STUB_BIN}/clang++"

  cat > "${STUB_BIN}/xcrun" <<'EOF'
#!/bin/bash
if [ "$1" = "--find" ] && [ "$2" = "swiftc" ]; then
  echo "/usr/bin/swiftc"
  exit 0
fi
exit 1
EOF
  chmod +x "${STUB_BIN}/xcrun"

  cat > "${STUB_BIN}/xcodebuild" <<'EOF'
#!/bin/bash
if [ "$1" = "-checkFirstLaunchStatus" ]; then
  exit 0
fi
if [ "$1" = "-license" ] && [ "$2" = "check" ]; then
  exit 0
fi
if [ "$1" = "-runFirstLaunch" ]; then
  exit 0
fi
if [ "$1" = "-license" ] && [ "$2" = "accept" ]; then
  exit 0
fi
exit 1
EOF
  chmod +x "${STUB_BIN}/xcodebuild"
}

create_brew_stub() {
  cat > "${STUB_BIN}/brew" <<'EOF'
#!/bin/bash
if [ "$1" = "--prefix" ]; then
  if [ -z "${2:-}" ]; then
    printf "%s\n" "${STUB_BREW_PREFIX}"
    exit 0
  fi
  if [ "$2" = "llvm" ]; then
    printf "%s\n" "${STUB_BREW_PREFIX}/opt/llvm"
    exit 0
  fi
  printf "%s\n" "${STUB_BREW_PREFIX}/opt/$2"
  exit 0
fi
if [ "$1" = "install" ]; then
  FORMULA="$2"
  printf "install %s\n" "${FORMULA}" >> "${BREW_LOG}"
  if [ "${FORMULA}" = "clang-tools-extra" ]; then
    cat > "${STUB_BIN}/clang-tidy" <<'INNER'
#!/bin/bash
exit 0
INNER
    chmod +x "${STUB_BIN}/clang-tidy"
    exit 0
  fi
  if [ "${FORMULA}" = "llvm" ]; then
    mkdir -p "${STUB_BREW_PREFIX}/opt/llvm/bin"
    cat > "${STUB_BREW_PREFIX}/opt/llvm/bin/clang-tidy" <<'INNER'
#!/bin/bash
exit 0
INNER
    chmod +x "${STUB_BREW_PREFIX}/opt/llvm/bin/clang-tidy"
    exit 0
  fi
  cat > "${STUB_BIN}/${FORMULA}" <<'INNER'
#!/bin/bash
exit 0
INNER
  chmod +x "${STUB_BIN}/${FORMULA}"
  if [ "${FORMULA}" = "cmake" ]; then
    cat > "${STUB_BIN}/ctest" <<'INNER'
#!/bin/bash
exit 0
INNER
    chmod +x "${STUB_BIN}/ctest"
  fi
  exit 0
fi
exit 0
EOF
  chmod +x "${STUB_BIN}/brew"
}

@test "R001: script uses strict fail-fast mode" {
  #R001
  run rg "set -euo pipefail" "${SCRIPT_PATH}"
  [ "$status" -eq 0 ]
}

@test "R005: fails with guidance when Homebrew is missing" {
  #R005
  run env PATH="/usr/bin:/bin" bash "${SCRIPT_PATH}"
  [ "$status" -ne 0 ]
  [[ "${output}" == *"[Homebrew] Not installed."* ]]
  [[ "${output}" == *"install.sh"* ]]
}

@test "R010,R030: fails clearly when Xcode toolchain commands are missing" {
  #R010 #R030
  create_brew_stub
  run env PATH="${STUB_BIN}:/bin" BREW_LOG="${TMP_ROOT}/brew.log" STUB_BIN="${STUB_BIN}" STUB_BREW_PREFIX="${STUB_BREW_PREFIX}" /bin/bash "${SCRIPT_PATH}"
  [ "$status" -ne 0 ]
  [[ "${output}" == *"[Xcode Toolchain] xcodebuild not found"* ]]
  [[ "${output}" == *"[Xcode Toolchain] xcrun"* ]]
}

@test "R015,R020,R025,R030,R035: installs required formulas and prints Fountain guidance" {
  #R015 #R020 #R025 #R030 #R035
  create_common_toolchain_stubs
  create_brew_stub

  run env PATH="${STUB_BIN}:/bin" BREW_LOG="${TMP_ROOT}/brew.log" STUB_BIN="${STUB_BIN}" STUB_BREW_PREFIX="${STUB_BREW_PREFIX}" bash "${SCRIPT_PATH}"
  [ "$status" -eq 0 ]
  [[ "${output}" == *"[cmake] Checking..."* ]]
  [[ "${output}" == *"[ctest] Available on PATH"* ]]
  [[ "${output}" == *"cmake -S . -B build"* ]]
  [[ "${output}" == *"cmake --build build"* ]]
  [[ "${output}" == *"ctest --test-dir build --output-on-failure"* ]]
  run rg "^install cmake$" "${TMP_ROOT}/brew.log"
  [ "$status" -eq 0 ]
  run rg "^install sqlite3$" "${TMP_ROOT}/brew.log"
  [ "$status" -eq 0 ]
  run rg "^install pkg-config$" "${TMP_ROOT}/brew.log"
  [ "$status" -eq 0 ]
  run rg "^install shellcheck$" "${TMP_ROOT}/brew.log"
  [ "$status" -eq 0 ]
  run rg "^install semgrep$" "${TMP_ROOT}/brew.log"
  [ "$status" -eq 0 ]
  run rg "^install clang-tools-extra$" "${TMP_ROOT}/brew.log"
  [ "$status" -eq 0 ]
  run rg "^install gitleaks$" "${TMP_ROOT}/brew.log"
  [ "$status" -eq 0 ]
}

@test "R020: fails when ctest remains unavailable after cmake install" {
  #R020
  create_common_toolchain_stubs
  cat > "${STUB_BIN}/brew" <<'EOF'
#!/bin/bash
if [ "$1" = "install" ] && [ "$2" = "cmake" ]; then
  cat > "${STUB_BIN}/cmake" <<'INNER'
#!/bin/bash
exit 0
INNER
  chmod +x "${STUB_BIN}/cmake"
  exit 0
fi
if [ "$1" = "install" ]; then
  cat > "${STUB_BIN}/$2" <<'INNER'
#!/bin/bash
exit 0
INNER
  chmod +x "${STUB_BIN}/$2"
  exit 0
fi
exit 0
EOF
  chmod +x "${STUB_BIN}/brew"
  run env PATH="${STUB_BIN}:/bin" STUB_BIN="${STUB_BIN}" BREW_LOG="${TMP_ROOT}/brew.log" STUB_BREW_PREFIX="${STUB_BREW_PREFIX}" bash "${SCRIPT_PATH}"
  [ "$status" -ne 0 ]
  [[ "${output}" == *"[ctest] Missing after cmake installation"* ]]
}

@test "R040: reruns are idempotent and skip redundant installs" {
  #R040
  create_common_toolchain_stubs
  create_brew_stub

  run env PATH="${STUB_BIN}:/bin" BREW_LOG="${TMP_ROOT}/brew.log" STUB_BIN="${STUB_BIN}" STUB_BREW_PREFIX="${STUB_BREW_PREFIX}" bash "${SCRIPT_PATH}"
  [ "$status" -eq 0 ]

  run env PATH="${STUB_BIN}:/bin" BREW_LOG="${TMP_ROOT}/brew.log" STUB_BIN="${STUB_BIN}" STUB_BREW_PREFIX="${STUB_BREW_PREFIX}" bash "${SCRIPT_PATH}"
  [ "$status" -eq 0 ]

  run rg "^install cmake$" "${TMP_ROOT}/brew.log" --count
  [ "$status" -eq 0 ]
  [ "${output}" = "1" ]
  run rg "^install sqlite3$" "${TMP_ROOT}/brew.log" --count
  [ "$status" -eq 0 ]
  [ "${output}" = "1" ]
  run rg "^install pkg-config$" "${TMP_ROOT}/brew.log" --count
  [ "$status" -eq 0 ]
  [ "${output}" = "1" ]
  run rg "^install shellcheck$" "${TMP_ROOT}/brew.log" --count
  [ "$status" -eq 0 ]
  [ "${output}" = "1" ]
  run rg "^install semgrep$" "${TMP_ROOT}/brew.log" --count
  [ "$status" -eq 0 ]
  [ "${output}" = "1" ]
  run rg "^install clang-tools-extra$" "${TMP_ROOT}/brew.log" --count
  [ "$status" -eq 0 ]
  [ "${output}" = "1" ]
  run rg "^install gitleaks$" "${TMP_ROOT}/brew.log" --count
  [ "$status" -eq 0 ]
  [ "${output}" = "1" ]
}

# Install Prerequisites Requirements

## Scope

Applies to `01_install_prerequisites.sh` and local macOS setup required before running Fountain C++ build/test targets.

R001  Statement: Run with `bash` in strict fail-fast mode.
Design: Use `set -euo pipefail` and exit non-zero on unrecoverable failures.
Tests:
- Force a failing command and verify installer exits non-zero.

R005  Statement: Verify Homebrew exists before Homebrew package actions.
Design: Check `brew` on `PATH`; print install guidance when missing.
Tests:
- Run with `brew` unavailable and verify clear failure guidance.

R010  Statement: Ensure Xcode command-line tooling required by Fountain is available.
Design: Require `xcodebuild`, `xcrun`, and `clang++` to resolve before continuing.
Tests:
- Simulate missing toolchain command and verify clear failure message.

R015  Statement: Install required Homebrew formulas when missing.
Design: Use shared idempotent helper logic that checks command availability first, installs with `brew install` only when missing, and fails clearly if still unavailable.
Tests:
- Run without a required formula and verify installer runs `brew install <formula>`.
- Rerun with the same formula installed and verify no reinstall.

R020  Statement: Ensure CMake and CTest are installed and available on `PATH`.
Design: Install Homebrew `cmake` when missing, then verify both `cmake` and `ctest` commands resolve.
Tests:
- Run without `cmake` and `ctest` and verify installer runs `brew install cmake`.
- Verify installer fails when `ctest` is still missing after install.

R025  Statement: Ensure core tooling for local Fountain development is available.
Design: Ensure `sqlite3` and `pkg-config` are installed and available via Homebrew helper flow.
Tests:
- Run without `sqlite3` and `pkg-config` and verify install attempts and command availability checks.

R030  Statement: Ensure SAST tools required by Fountain are available.
Design: Ensure `shellcheck`, `semgrep`, `clang-tidy`, and `gitleaks` are installed and available via Homebrew helper flow.
Tests:
- Run without those tools and verify install attempts.
- Rerun and verify no redundant installs.

R035  Statement: Print final readiness guidance for local development.
Design: End with success output that references CMake/CTest commands used by this repository (`cmake -S . -B build`, `cmake --build build`, and `ctest --test-dir build --output-on-failure`).
Tests:
- On successful run, verify final guidance includes those commands.

R040  Statement: Keep installer idempotent across reruns.
Design: Check command availability before install actions so reruns skip unnecessary work and stay deterministic.
Tests:
- Run installer twice and verify required formulas are installed at most once in the test harness.

## Changelog

- 2026-05-07: Reswizzled copied prerequisites for Fountain C++ workflow and required CMake/CTest setup.
- 2026-05-09: Added explicit `clang-tidy` prerequisite coverage for Makefile SAST lanes.

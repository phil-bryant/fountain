#!/bin/bash
umask 007

#R001: Run unit-test wrapper with deterministic strict shell behavior.
set -euo pipefail

#R005: Execute Makefile tooling from the repository root.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "[run-unit-tests] Running make test"
#R010: Delegate unit-test execution to the Makefile test lane.
#R020: Strict mode propagates any make test failure before success guidance.
make test
#R015: Print concise confirmation only after tests succeed.
echo "✅ Unit tests passed."

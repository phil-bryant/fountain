#!/bin/bash
umask 007

#R001: Run lint wrapper with deterministic strict shell behavior.
set -euo pipefail

#R005: Execute Makefile tooling from the repository root.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "[check-lints] Running make lint"
#R010: Delegate lint checks to the Makefile lint lane.
#R020: Strict mode propagates any make lint failure before success guidance.
make lint
#R015: Print concise confirmation only after lint succeeds.
echo "✅ Lint checks passed."

#!/bin/bash
umask 007

#R001: Run SAST wrapper with deterministic strict shell behavior.
set -euo pipefail

#R005: Execute Makefile tooling from the repository root.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "[run-sast] Running make sast"
#R010: Delegate SAST checks to the Makefile SAST lane.
#R020: Strict mode propagates any make sast failure before success guidance.
make sast
#R015: Print concise confirmation only after SAST succeeds.
echo "✅ SAST checks passed."

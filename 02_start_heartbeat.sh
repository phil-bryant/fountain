#!/bin/bash
umask 007

#R001: Run start flow with deterministic strict shell behavior.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
INSTALL_ID=""
INTERVAL_SECONDS="900"
EVENT_NAME="fountain.heartbeat"
COMPONENT_NAME="fountain.runtime"
STATE_FILE=".heartbeat/heartbeat.state"
PID_FILE=".heartbeat/heartbeat.pid"
DATABASE_PATH=".heartbeat/heartbeat.sqlite3"
RUNNER_PATH="build/examples/cpp_basic/fountain_heartbeat_runner"
LOG_FILE=".heartbeat/heartbeat.log"

while [ "$#" -gt 0 ]; do
    if [ "$1" = "--install-id" ] && [ "$#" -ge 2 ]; then
        #R005: Require explicit rollout target install id via CLI.
        INSTALL_ID="$2"
        shift 2
    elif [ "$1" = "--interval-seconds" ] && [ "$#" -ge 2 ]; then
        #R015: Support operator override for heartbeat interval.
        INTERVAL_SECONDS="$2"
        shift 2
    elif [ "$1" = "--event-name" ] && [ "$#" -ge 2 ]; then
        #R015: Support operator override for heartbeat event name.
        EVENT_NAME="$2"
        shift 2
    elif [ "$1" = "--component" ] && [ "$#" -ge 2 ]; then
        #R015: Support operator override for heartbeat component.
        COMPONENT_NAME="$2"
        shift 2
    elif [ "$1" = "--state-file" ] && [ "$#" -ge 2 ]; then
        #R015: Support operator override for persisted state-file path.
        STATE_FILE="$2"
        shift 2
    elif [ "$1" = "--pid-file" ] && [ "$#" -ge 2 ]; then
        #R015: Support operator override for pid-file path.
        PID_FILE="$2"
        shift 2
    elif [ "$1" = "--database-path" ] && [ "$#" -ge 2 ]; then
        #R015: Support operator override for heartbeat SQLite path.
        DATABASE_PATH="$2"
        shift 2
    elif [ "$1" = "--runner-path" ] && [ "$#" -ge 2 ]; then
        #R015: Support operator override for heartbeat runner executable.
        RUNNER_PATH="$2"
        shift 2
    elif [ "$1" = "--log-file" ] && [ "$#" -ge 2 ]; then
        #R015: Support operator override for runner log path.
        LOG_FILE="$2"
        shift 2
    else
        echo "Usage: $0 --install-id <id> [--interval-seconds <seconds>] [--event-name <name>] [--component <name>] [--state-file <path>] [--pid-file <path>] [--database-path <path>] [--runner-path <path>] [--log-file <path>]"
        exit 1
    fi
done

if [ -z "$INSTALL_ID" ]; then
    #R005: Fail clearly when rollout target install id is not provided.
    echo "❌ Missing required --install-id <value>."
    exit 1
fi
if ! [[ "$INTERVAL_SECONDS" =~ ^[0-9]+$ ]] || [ "$INTERVAL_SECONDS" -le 0 ]; then
    echo "❌ interval_seconds must be a positive integer."
    exit 1
fi
if [ -z "$EVENT_NAME" ] || [ -z "$COMPONENT_NAME" ]; then
    echo "❌ event and component values must be non-empty."
    exit 1
fi

STATE_DIR="$(dirname "$STATE_FILE")"
mkdir -p "$STATE_DIR"
mkdir -p "$(dirname "$PID_FILE")"
mkdir -p "$(dirname "$DATABASE_PATH")"
mkdir -p "$(dirname "$LOG_FILE")"

if [ ! -x "$RUNNER_PATH" ]; then
    if [ "$RUNNER_PATH" = "build/examples/cpp_basic/fountain_heartbeat_runner" ] && command -v cmake >/dev/null 2>&1; then
        cmake -S "$SCRIPT_DIR" -B "${SCRIPT_DIR}/build" -DFOUNTAIN_BUILD_EXAMPLES=ON -DFOUNTAIN_BUILD_TESTS=ON
        cmake --build "${SCRIPT_DIR}/build" --target fountain_heartbeat_runner
    fi
fi
if [ ! -x "$RUNNER_PATH" ]; then
    echo "❌ Heartbeat runner not found or not executable: ${RUNNER_PATH}"
    echo "Build it first: cmake -S . -B build -DFOUNTAIN_BUILD_EXAMPLES=ON && cmake --build build --target fountain_heartbeat_runner"
    exit 1
fi

if [ -f "$PID_FILE" ]; then
    current_pid="$(awk 'NR==1 {print $1}' "$PID_FILE")"
    if [ -n "$current_pid" ] && kill -0 "$current_pid" >/dev/null 2>&1; then
        kill "$current_pid"
    fi
fi

#R020: Persist start state atomically via temp-file then move.
TMP_STATE_FILE="$(mktemp "${STATE_FILE}.tmp.XXXXXX")"
chmod 660 "$TMP_STATE_FILE"
{
    echo "heartbeat_enabled=1"
    #R010: Persist default cadence/identity unless overridden.
    echo "heartbeat_interval_seconds=${INTERVAL_SECONDS}"
    echo "heartbeat_event_name=${EVENT_NAME}"
    echo "heartbeat_component=${COMPONENT_NAME}"
    echo "heartbeat_target_install_id=${INSTALL_ID}"
    echo "heartbeat_pid_file=${PID_FILE}"
    echo "heartbeat_database_path=${DATABASE_PATH}"
    echo "heartbeat_started_at_utc=$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
} > "$TMP_STATE_FILE"
mv "$TMP_STATE_FILE" "$STATE_FILE"

#R020: Start real heartbeat runner process and persist pid atomically.
nohup "$RUNNER_PATH" --database-path "$DATABASE_PATH" --install-id "$INSTALL_ID" --interval-seconds "$INTERVAL_SECONDS" --event-name "$EVENT_NAME" --component "$COMPONENT_NAME" >> "$LOG_FILE" 2>&1 &
heartbeat_pid="$!"
TMP_PID_FILE="$(mktemp "${PID_FILE}.tmp.XXXXXX")"
chmod 660 "$TMP_PID_FILE"
echo "$heartbeat_pid" > "$TMP_PID_FILE"
mv "$TMP_PID_FILE" "$PID_FILE"

if ! kill -0 "$heartbeat_pid" >/dev/null 2>&1; then
    echo "❌ Heartbeat runner failed to stay running. Check log: ${LOG_FILE}"
    exit 1
fi

#R025: Emit concise next-step guidance for activation handoff.
echo "✅ Heartbeat started."
echo "- state_file=${STATE_FILE}"
echo "- pid_file=${PID_FILE}"
echo "- heartbeat_pid=${heartbeat_pid}"
echo "- database_path=${DATABASE_PATH}"
echo "- interval_seconds=${INTERVAL_SECONDS}"
echo "- target_install_id=${INSTALL_ID}"
echo "Next step: run ./03_verify_heartbeat.sh to confirm active process and queued heartbeat events."

#R030: Reruns overwrite existing state with latest values (idempotent replacement).

#!/bin/bash
umask 007

#R001: Run verify flow with deterministic strict shell behavior.
set -euo pipefail

STATE_FILE=".heartbeat/heartbeat.state"
EXPECTED_INSTALL_ID=""
PID_FILE=""
DATABASE_PATH=""

while [ "$#" -gt 0 ]; do
    if [ "$1" = "--state-file" ] && [ "$#" -ge 2 ]; then
        STATE_FILE="$2"
        shift 2
    elif [ "$1" = "--expect-install-id" ] && [ "$#" -ge 2 ]; then
        #R020: Accept optional expected install-id for rollout-target verification.
        EXPECTED_INSTALL_ID="$2"
        shift 2
    else
        echo "Usage: $0 [--state-file <path>] [--expect-install-id <id>]"
        exit 1
    fi
done

if [ ! -f "$STATE_FILE" ]; then
    #R005: Fail explicitly when heartbeat state file cannot be found.
    echo "❌ Heartbeat state file not found: ${STATE_FILE}"
    exit 1
fi

enabled_value="$(awk -F= '$1=="heartbeat_enabled"{print $2}' "$STATE_FILE")"
interval_value="$(awk -F= '$1=="heartbeat_interval_seconds"{print $2}' "$STATE_FILE")"
event_name_value="$(awk -F= '$1=="heartbeat_event_name"{print $2}' "$STATE_FILE")"
component_value="$(awk -F= '$1=="heartbeat_component"{print $2}' "$STATE_FILE")"
install_id_value="$(awk -F= '$1=="heartbeat_target_install_id"{print $2}' "$STATE_FILE")"
pid_file_value="$(awk -F= '$1=="heartbeat_pid_file"{print $2}' "$STATE_FILE")"
database_path_value="$(awk -F= '$1=="heartbeat_database_path"{print $2}' "$STATE_FILE")"

if [ "$enabled_value" != "1" ]; then
    #R010: Require active state marker during verification.
    echo "❌ Heartbeat is not active (heartbeat_enabled must equal 1)."
    exit 1
fi
if ! [[ "$interval_value" =~ ^[0-9]+$ ]] || [ "$interval_value" -le 0 ]; then
    #R015: Enforce positive integer interval semantics.
    echo "❌ heartbeat_interval_seconds must be a positive integer."
    exit 1
fi
if [ -z "$event_name_value" ] || [ -z "$component_value" ] || [ -z "$install_id_value" ]; then
    #R010: Validate required heartbeat identity and rollout fields are present.
    echo "❌ Heartbeat state is missing required key values."
    exit 1
fi
if [ -z "$pid_file_value" ] || [ -z "$database_path_value" ]; then
    #R010: Validate runtime process/database pointers are present in state.
    echo "❌ Heartbeat state is missing pid/database configuration."
    exit 1
fi
PID_FILE="$pid_file_value"
DATABASE_PATH="$database_path_value"
if [ -n "$EXPECTED_INSTALL_ID" ] && [ "$EXPECTED_INSTALL_ID" != "$install_id_value" ]; then
    #R020: Fail when expected install target does not match persisted rollout target.
    echo "❌ Expected install id '${EXPECTED_INSTALL_ID}' does not match '${install_id_value}'."
    exit 1
fi
if [ ! -f "$PID_FILE" ]; then
    #R020: Verify running process state from persisted pid-file.
    echo "❌ Heartbeat pid file not found: ${PID_FILE}"
    exit 1
fi
heartbeat_pid="$(awk 'NR==1 {print $1}' "$PID_FILE")"
if [ -z "$heartbeat_pid" ] || ! kill -0 "$heartbeat_pid" >/dev/null 2>&1; then
    #R020: Fail when heartbeat process is not currently running.
    echo "❌ Heartbeat process is not running (pid_file=${PID_FILE})."
    exit 1
fi
if [ ! -f "$DATABASE_PATH" ]; then
    #R015: Fail when heartbeat database is missing.
    echo "❌ Heartbeat database file not found: ${DATABASE_PATH}"
    exit 1
fi
if ! command -v sqlite3 >/dev/null 2>&1; then
    echo "❌ sqlite3 is required for heartbeat verification."
    exit 1
fi
event_count="$(sqlite3 "$DATABASE_PATH" "SELECT COUNT(*) FROM fountain_event_queue WHERE event_name='${event_name_value}';")"
if ! [[ "$event_count" =~ ^[0-9]+$ ]] || [ "$event_count" -le 0 ]; then
    #R015: Require at least one queued heartbeat event to verify actual emission.
    echo "❌ No heartbeat events found yet in queue (event_name=${event_name_value})."
    exit 1
fi

#R025: Print concise success summary with key heartbeat values.
echo "✅ Heartbeat verification passed."
echo "- state_file=${STATE_FILE}"
echo "- pid_file=${PID_FILE}"
echo "- heartbeat_pid=${heartbeat_pid}"
echo "- database_path=${DATABASE_PATH}"
echo "- queued_events=${event_count}"
echo "- install_id=${install_id_value}"
echo "- interval_seconds=${interval_value}"
echo "- event_name=${event_name_value}"
echo "- component=${component_value}"

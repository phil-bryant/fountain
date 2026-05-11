#!/bin/bash
umask 007

#R001: Run stop flow with deterministic strict shell behavior.
set -euo pipefail

STATE_FILE=".heartbeat/heartbeat.state"
ARCHIVE_ROOT="${HOME}/.Trash"

while [ "$#" -gt 0 ]; do
    if [ "$1" = "--state-file" ] && [ "$#" -ge 2 ]; then
        STATE_FILE="$2"
        shift 2
    elif [ "$1" = "--archive-root" ] && [ "$#" -ge 2 ]; then
        ARCHIVE_ROOT="$2"
        shift 2
    else
        echo "Usage: $0 [--state-file <path>] [--archive-root <path>]"
        exit 1
    fi
done

if [ ! -f "$STATE_FILE" ]; then
    #R005: Fail explicitly when heartbeat state file is missing.
    echo "❌ Heartbeat state file not found: ${STATE_FILE}"
    exit 1
fi

enabled_value="$(awk -F= '$1=="heartbeat_enabled"{print $2}' "$STATE_FILE")"
interval_value="$(awk -F= '$1=="heartbeat_interval_seconds"{print $2}' "$STATE_FILE")"
event_name_value="$(awk -F= '$1=="heartbeat_event_name"{print $2}' "$STATE_FILE")"
component_value="$(awk -F= '$1=="heartbeat_component"{print $2}' "$STATE_FILE")"
install_id_value="$(awk -F= '$1=="heartbeat_target_install_id"{print $2}' "$STATE_FILE")"
started_at_value="$(awk -F= '$1=="heartbeat_started_at_utc"{print $2}' "$STATE_FILE")"
pid_file_value="$(awk -F= '$1=="heartbeat_pid_file"{print $2}' "$STATE_FILE")"
database_path_value="$(awk -F= '$1=="heartbeat_database_path"{print $2}' "$STATE_FILE")"

runner_stopped="0"
if [ -n "$pid_file_value" ] && [ -f "$pid_file_value" ]; then
    heartbeat_pid="$(awk 'NR==1 {print $1}' "$pid_file_value")"
    if [ -n "$heartbeat_pid" ] && kill -0 "$heartbeat_pid" >/dev/null 2>&1; then
        kill "$heartbeat_pid"
        runner_stopped="1"
    fi
fi

if [ "$enabled_value" = "0" ]; then
    #R020: Treat already-stopped heartbeat as idempotent success.
    echo "✅ Heartbeat already stopped."
    echo "- state_file=${STATE_FILE}"
    exit 0
fi

timestamp="$(date -u +"%Y%m%d-%H%M%S")"
archive_dir="${ARCHIVE_ROOT}/fountain-heartbeat-stop-${timestamp}"
mkdir -p "$archive_dir"
archive_path="${archive_dir}/$(basename "$STATE_FILE")"

#R015: Preserve existing state by moving it to a timestamped archive location.
mv "$STATE_FILE" "$archive_path"

state_dir="$(dirname "$STATE_FILE")"
mkdir -p "$state_dir"
tmp_state_file="$(mktemp "${STATE_FILE}.tmp.XXXXXX")"
chmod 660 "$tmp_state_file"
{
    #R010: Persist disabled state while keeping previous heartbeat configuration values.
    echo "heartbeat_enabled=0"
    echo "heartbeat_interval_seconds=${interval_value}"
    echo "heartbeat_event_name=${event_name_value}"
    echo "heartbeat_component=${component_value}"
    echo "heartbeat_target_install_id=${install_id_value}"
    echo "heartbeat_pid_file=${pid_file_value}"
    echo "heartbeat_database_path=${database_path_value}"
    echo "heartbeat_started_at_utc=${started_at_value}"
    echo "heartbeat_stopped_at_utc=$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
} > "$tmp_state_file"
mv "$tmp_state_file" "$STATE_FILE"

#R025: Emit concise stop confirmation including archived-state location.
echo "✅ Heartbeat stopped and state updated."
echo "- state_file=${STATE_FILE}"
echo "- archived_state=${archive_path}"
if [ "$runner_stopped" = "1" ]; then
    echo "- runner_process_stopped=true"
else
    echo "- runner_process_stopped=false"
fi

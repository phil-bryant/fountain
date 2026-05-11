# Fountain

Fountain is a C++ structured event logger for macOS applications. It exposes a plain C ABI for Swift, Objective-C, C, and C++ callers, persists uploadable event payloads into a durable SQLite queue, and provides batch claim/result APIs for an external uploader (Piston).

Fountain does not perform network upload.

## What Fountain Does

- Accepts structured events with:
  - log level
  - `event_name`
  - `component`
  - typed fields (`string`, `int64`, `double`, `bool`)
  - per-field privacy classification (`public`, `private`, `forbidden`)
- Builds the final JSON event envelope.
- Filters fields before persistence:
  - `public`: included in uploadable JSON
  - `private`: excluded from uploadable JSON
  - `forbidden`: excluded from uploadable JSON
- Persists events in SQLite for durable queueing.
- Lets an external uploader claim batches and mark success/failure.

## Architecture

Application code (`Swift`/`ObjC`/`C`/`C++`) -> Fountain C ABI -> Fountain C++ core -> JSON serialization -> SQLite queue -> external uploader (Piston)

## Repository Layout

- `include/fountain/fountain.h`: Public C ABI
- `src/fountain_c_api.cpp`: C ABI boundary wrappers
- `src/fountain_runtime.*`: Thread-safe runtime/state
- `src/event_json.*`: Event envelope construction
- `src/queue_repository.*`: SQLite schema and queue operations
- `src/sqlite/*`: SQLite C API RAII wrappers
- `tests/`: Unit and integration-oriented tests
- `examples/cpp_basic/main.cpp`: Minimal C++ usage example
- `docs/swift-interop.md`: Swift interop and ownership notes

## Build

Prerequisites on macOS:

- Xcode command line tools (`xcodebuild`, `xcrun`, `clang++`)
- Homebrew
- `cmake` (includes `ctest`)

Configure and build:

```bash
cmake -S . -B build
cmake --build build
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## C API Summary

Key functions in `fountain.h`:

- `FountainConfigure(database_path)`
- `FountainSetInstallID(install_id)`
- `FountainSetSessionID(session_id)`
- `FountainSetAppMetadata(metadata_ptr)`
- `FountainLogEvent(level, event_name, component, fields, field_count)`
- `FountainCreateUploadBatch(max_events, max_bytes, out_batch)`
- `FountainMarkUploadBatchSucceeded(batch_id)`
- `FountainMarkUploadBatchFailed(batch_id, http_status, error_message)`
- `FountainFreeUploadBatch(batch)`
- `FountainRunMaintenance()`
- `FountainStartHeartbeat(config_ptr)`
- `FountainStopHeartbeat()`

## Heartbeat Generator (Optional)

Fountain now supports an optional heartbeat generator that logs a heartbeat event on a fixed interval.

- Default interval: `900` seconds (`15` minutes)
- Default event identity:
  - `event_name`: `fountain.heartbeat`
  - `component`: `fountain.runtime`
- Rollout gate: set `target_install_id` to only emit heartbeat for one selected install initially.

The scheduler uses standard C++ threading primitives and is portable across macOS, iOS, Windows, and Linux builds.

Example:

```c
FountainHeartbeatConfig heartbeat = {0};
heartbeat.interval_seconds = 900;
heartbeat.event_name = "fountain.heartbeat";
heartbeat.component = "fountain.runtime";
heartbeat.target_install_id = "install-id-for-initial-rollout";
FountainStartHeartbeat(&heartbeat);
```

Stop when your app is shutting down:

```c
FountainStopHeartbeat();
```

## Heartbeat Operator Flow (02 -> 03 -> 04)

Use the numbered helper scripts to manage one-install heartbeat rollout state:

```bash
# 02: start (starts heartbeat runner and writes state with heartbeat_enabled=1)
./02_start_heartbeat.sh --install-id install-id-for-initial-rollout

# 03: verify (checks active process, rollout target, and queued heartbeat events)
./03_verify_heartbeat.sh --expect-install-id install-id-for-initial-rollout

# 04: stop (archives previous state and writes heartbeat_enabled=0)
./04_stop_heartbeat.sh
```

Notes:

- Default state file path: `.heartbeat/heartbeat.state`
- Default pid file path: `.heartbeat/heartbeat.pid`
- Default heartbeat DB path: `.heartbeat/heartbeat.sqlite3`
- `02_start_heartbeat.sh` supports overrides: `--interval-seconds`, `--event-name`, `--component`, `--state-file`
- `02_start_heartbeat.sh` auto-builds `build/examples/cpp_basic/fountain_heartbeat_runner` when needed
- `03_verify_heartbeat.sh` supports `--state-file` and optional `--expect-install-id`
- `04_stop_heartbeat.sh` supports `--state-file` and `--archive-root` (defaults to `~/.Trash`)

## Safety and Runtime Behavior

- C ABI functions catch exceptions and do not throw across the boundary.
- Logging failures are contained and do not crash the host app.
- SQLite access and runtime metadata are synchronized for multi-threaded callers.

## Example

See `examples/cpp_basic/main.cpp` for a complete configure -> log -> claim -> mark-success flow.

## End-to-End Diagram

```text
BACKEND
┌──────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│  ┌────────────────────────────┐        ┌──────────────────────────────────┐  │
│  │           Valve            │        │             Manifold             │  │
│  │                            │        │                                  │  │
│  │ - runs after user sign-in  │        │ - verifies signature / credential│  │
│  │ - verifies Account A access│        │ - derives tenant_id + install_id │  │
│  │ - provisions per-tenant /  │        │   from credential                │  │
│  │   per-install credential   │        │ - stores tenant-scoped events    │  │
│  │ - rotates / revokes creds  │        │ - rejects tenant mismatches      │  │
│  └─────────────┬──────────────┘        │                                  │  │
│                │                       │  ┌───────────────────────┐       │  │
│                │                       │  │ Ingest Endpoint       │       │  │
│                │                 ┌─────┼─►│ POST /v1/events/batch │       │  │
│                │                 │     │  └───────────────────────┘       │  │
│                │                 │     │                                  │  │
│                │                 │     └──────────────────────┬───────────┘  │
│                │                 │                            │              │
└────────────────┼─────────────────┼────────────────────────────┼──────────────┘
                 │                 │                            │
                 │                 │                            │ tenant-scoped
                 │                 │                            │ events
                 │ credential      │             NOC/SOC        ▼
                 │ for Account A   │             ┌──────────────────────────┐
                 │ + Install 123   │             │          Vortex          │
                 │                 │             │                          │
                 │                 │             │ - downstream all-in-one  │
                 │                 │             │ - storage / analytics    │
                 │                 │             │ - dashboards / alerts    │
                 │                 │             │ - incident review        │
    credential   │                 │             │ - strict tenant_id reads │
    provisioned  │                 │             └──────────────────────────┘
   after sign-in │                 │
                 │                 │ HTTPS + signed batch
                 │                 │ credential proves Account A + Install 123
CUSTOMER DEVICE  ▼                 │
┌──────────────────────────────────┼─────────┐
│                                  │         │
│  ┌────────────────────┐   ┌─────────────┐  │
│  │      Fountain      │   │   Piston    │  │
│  │                    │   │             │  │
│  │ - C++ event logger │   │ - Swift     │  │
│  │ - SQLite queue     │   │   uploader  │  │
│  │ - tags events with │   │ - stores    │  │
│  │   tenant_scope     │   │   credential│  │
│  └─────────┬──────────┘   │ - claims    │  │
│            │              │   matching  │  │
│            │ Account A    │   scope     │  │
│            │ events       │ - signs     │  │
│            └─────────────►│   uploads   │  │
│                           └─────────────┘  │
│                                            │
└────────────────────────────────────────────┘
```

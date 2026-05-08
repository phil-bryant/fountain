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
- `FountainSetAppMetadata(bundle_id, app_version, build, os_name, os_version, arch)`
- `FountainLogEvent(level, event_name, component, fields, field_count)`
- `FountainCreateUploadBatch(max_events, max_bytes, out_batch)`
- `FountainMarkUploadBatchSucceeded(batch_id)`
- `FountainMarkUploadBatchFailed(batch_id, http_status, error_message)`
- `FountainFreeUploadBatch(batch)`
- `FountainRunMaintenance()`

## Safety and Runtime Behavior

- C ABI functions catch exceptions and do not throw across the boundary.
- Logging failures are contained and do not crash the host app.
- SQLite access and runtime metadata are synchronized for multi-threaded callers.

## Example

See `examples/cpp_basic/main.cpp` for a complete configure -> log -> claim -> mark-success flow.

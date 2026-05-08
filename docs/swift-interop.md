# Fountain Swift Interop (C ABI)

Fountain exposes a plain C ABI in `include/fountain/fountain.h`, which Swift can call through a bridging header or Clang module map.

## Basic Swift flow

1. Call `FountainConfigure(dbPath)` once early in app startup.
2. Set identity and app metadata with:
   - `FountainSetInstallID`
   - `FountainSetSessionID`
   - `FountainSetAppMetadata`
3. Log events with `FountainLogEvent`.
4. In uploader code (Piston), call:
   - `FountainCreateUploadBatch`
   - upload JSON payload externally
   - `FountainMarkUploadBatchSucceeded` or `FountainMarkUploadBatchFailed`
   - `FountainFreeUploadBatch`

## Mapping Swift values to FountainLogField

- `String` -> `FountainLogValueString`, `value.string_value`
- `Int64` -> `FountainLogValueInt64`, `value.int64_value`
- `Double` -> `FountainLogValueDouble`, `value.double_value`
- `Bool` -> `FountainLogValueBool`, `value.bool_value`

Privacy controls:
- `FountainLogPrivacyPublic`: included in uploadable `fields`.
- `FountainLogPrivacyPrivate`: excluded from uploadable payload.
- `FountainLogPrivacyForbidden`: excluded from uploadable payload.

## Memory ownership

- `FountainCreateUploadBatch` allocates:
  - `FountainUploadBatch.batch_id`
  - `FountainUploadBatch.json_payload`
- Caller must always release both using `FountainFreeUploadBatch(&batch)` once finished.
- Safe to call `FountainFreeUploadBatch` with null pointers or an already-zeroed struct.

## Error model and thread safety

- Fountain C ABI never throws exceptions to Swift.
- Logging failures are contained internally and never crash host app.
- Fountain is internally synchronized for multi-thread callers.

## Minimal Swift sketch

```swift
var batch = FountainUploadBatch()
if FountainCreateUploadBatch(100, 512 * 1024, &batch) {
    defer { FountainFreeUploadBatch(&batch) }
    let payloadData = Data(
        bytes: batch.json_payload!,
        count: Int(batch.json_payload_length)
    )
    // Piston uploads payloadData...
    FountainMarkUploadBatchSucceeded(batch.batch_id)
}
```

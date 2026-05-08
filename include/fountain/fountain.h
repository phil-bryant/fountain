#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum FountainLogLevel {
    FountainLogLevelDebug = 0,
    FountainLogLevelInfo = 1,
    FountainLogLevelWarning = 2,
    FountainLogLevelError = 3,
    FountainLogLevelFault = 4
} FountainLogLevel;

typedef enum FountainLogPrivacy {
    FountainLogPrivacyPublic = 0,
    FountainLogPrivacyPrivate = 1,
    FountainLogPrivacyForbidden = 2
} FountainLogPrivacy;

typedef enum FountainLogValueType {
    FountainLogValueString = 0,
    FountainLogValueInt64 = 1,
    FountainLogValueDouble = 2,
    FountainLogValueBool = 3
} FountainLogValueType;

typedef struct FountainLogField {
    const char *key;
    FountainLogPrivacy privacy;
    FountainLogValueType type;

    union {
        const char *string_value;
        int64_t int64_value;
        double double_value;
        bool bool_value;
    } value;
} FountainLogField;

typedef struct FountainUploadBatch {
    char *batch_id;
    char *json_payload;
    size_t json_payload_length;
} FountainUploadBatch;

// Configures Fountain with a SQLite database path. Returns false on failure.
bool FountainConfigure(const char *database_path);

void FountainSetInstallID(const char *install_id);
void FountainSetSessionID(const char *session_id);
void FountainSetAppMetadata(
    const char *bundle_id,
    const char *app_version,
    const char *build,
    const char *os_name,
    const char *os_version,
    const char *arch
);

// Logs one event. Never throws and never crashes caller.
void FountainLogEvent(
    FountainLogLevel level,
    const char *event_name,
    const char *component,
    const FountainLogField *fields,
    size_t field_count
);

// Creates a batch for uploader ownership. Caller must free with FountainFreeUploadBatch.
bool FountainCreateUploadBatch(
    size_t max_events,
    size_t max_bytes,
    FountainUploadBatch *out_batch
);

void FountainMarkUploadBatchSucceeded(const char *batch_id);

void FountainMarkUploadBatchFailed(
    const char *batch_id,
    int http_status,
    const char *error_message
);

void FountainFreeUploadBatch(FountainUploadBatch *batch);

void FountainRunMaintenance(void);

#ifdef __cplusplus
}
#endif

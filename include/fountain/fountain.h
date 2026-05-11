#ifndef FOUNTAIN_INCLUDE_FOUNTAIN_H_
#define FOUNTAIN_INCLUDE_FOUNTAIN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t FountainLogLevel;
#define FountainLogLevelDebug ((FountainLogLevel)0u)
#define FountainLogLevelInfo ((FountainLogLevel)1u)
#define FountainLogLevelWarning ((FountainLogLevel)2u)
#define FountainLogLevelError ((FountainLogLevel)3u)
#define FountainLogLevelFault ((FountainLogLevel)4u)

typedef uint8_t FountainLogPrivacy;
#define FountainLogPrivacyPublic ((FountainLogPrivacy)0u)
#define FountainLogPrivacyPrivate ((FountainLogPrivacy)1u)
#define FountainLogPrivacyForbidden ((FountainLogPrivacy)2u)

typedef uint8_t FountainLogValueType;
#define FountainLogValueString ((FountainLogValueType)0u)
#define FountainLogValueInt64 ((FountainLogValueType)1u)
#define FountainLogValueDouble ((FountainLogValueType)2u)
#define FountainLogValueBool ((FountainLogValueType)3u)

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

typedef struct FountainAppMetadata {
    const char *bundle_id;
    const char *app_version;
    const char *build;
    const char *os_name;
    const char *os_version;
    const char *arch;
} FountainAppMetadata;

typedef struct FountainHeartbeatConfig {
    uint32_t interval_seconds;
    const char *event_name;
    const char *component;
    const char *target_install_id;
} FountainHeartbeatConfig;

// Configures Fountain with a SQLite database path. Returns false on failure.
bool FountainConfigure(const char *database_path);

void FountainSetInstallID(const char *install_id);
void FountainSetSessionID(const char *session_id);
void FountainSetAppMetadata(const FountainAppMetadata *metadata);

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

// Starts a background heartbeat generator. Returns true when scheduled.
// - interval_seconds: heartbeat cadence; 0 uses default 900 seconds.
// - event_name/component: optional; defaults to "fountain.heartbeat"/"fountain.runtime".
// - target_install_id: optional rollout gate. When set, only emits if install_id matches.
bool FountainStartHeartbeat(const FountainHeartbeatConfig *config);

// Stops the background heartbeat generator.
void FountainStopHeartbeat(void);

#ifdef __cplusplus
}
#endif
#endif

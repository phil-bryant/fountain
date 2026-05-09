#include "fountain/fountain.h"

#include <cstdlib>
#include <cstring>
#include <new>
#include <string>

#include "fountain_runtime.h"

namespace {

char *DupCString(const std::string &value) {
    char *buffer = static_cast<char *>(std::malloc(value.size() + 1));
    if (buffer == nullptr) {
        return nullptr;
    }
    std::memcpy(buffer, value.c_str(), value.size() + 1);
    return buffer;
}

void HandleApiException() noexcept {}

}  // namespace

extern "C" {

bool FountainConfigure(const char *database_path) {
    try {
        if (database_path == nullptr || database_path[0] == '\0') {
            return false;
        }
        return fountain::GetRuntime().Configure(database_path);
    } catch (...) {
        HandleApiException();
        return false;
    }
}

void FountainSetInstallID(const char *install_id) {
    try {
        if (install_id == nullptr) {
            return;
        }
        fountain::GetRuntime().SetInstallId(install_id);
    } catch (...) {
        HandleApiException();
    }
}

void FountainSetSessionID(const char *session_id) {
    try {
        if (session_id == nullptr) {
            return;
        }
        fountain::GetRuntime().SetSessionId(session_id);
    } catch (...) {
        HandleApiException();
    }
}

void FountainSetAppMetadata(
    const char *bundle_id, // NOLINT(bugprone-easily-swappable-parameters)
    const char *app_version,
    const char *build,
    const char *os_name,
    const char *os_version,
    const char *arch
) {
    try {
        fountain::AppMetadata metadata;
        metadata.bundle_id = bundle_id != nullptr ? bundle_id : "";
        metadata.app_version = app_version != nullptr ? app_version : "";
        metadata.build = build != nullptr ? build : "";
        metadata.os_name = os_name != nullptr ? os_name : "";
        metadata.os_version = os_version != nullptr ? os_version : "";
        metadata.arch = arch != nullptr ? arch : "";
        fountain::GetRuntime().SetAppMetadata(metadata);
    } catch (...) {
        HandleApiException();
    }
}

void FountainLogEvent(
    const FountainLogLevel level,
    const char *event_name,
    const char *component,
    const FountainLogField *fields,
    const size_t field_count
) {
    try {
        if (event_name == nullptr || component == nullptr || event_name[0] == '\0' || component[0] == '\0') {
            return;
        }
        fountain::EventInput input;
        input.level = level;
        input.event_name = event_name;
        input.component = component;
        if (fields != nullptr && field_count > 0) {
            input.fields = fountain::CopyFields(fields, field_count);
        }
        fountain::GetRuntime().LogEvent(input);
    } catch (...) {
        HandleApiException();
    }
}

bool FountainCreateUploadBatch(const size_t max_events, const size_t max_bytes, FountainUploadBatch *out_batch) {
    try {
        if (out_batch == nullptr) {
            return false;
        }
        out_batch->batch_id = nullptr;
        out_batch->json_payload = nullptr;
        out_batch->json_payload_length = 0;

        auto batch = fountain::GetRuntime().CreateUploadBatch(max_events, max_bytes);
        if (!batch.has_value()) {
            return false;
        }

        out_batch->batch_id = DupCString(batch->batch_id);
        out_batch->json_payload = DupCString(batch->json_payload);
        out_batch->json_payload_length = batch->json_payload.size();

        if (out_batch->batch_id == nullptr || out_batch->json_payload == nullptr) {
            FountainFreeUploadBatch(out_batch);
            return false;
        }
        return true;
    } catch (...) {
        HandleApiException();
        return false;
    }
}

void FountainMarkUploadBatchSucceeded(const char *batch_id) {
    try {
        if (batch_id == nullptr || batch_id[0] == '\0') {
            return;
        }
        fountain::GetRuntime().MarkUploadBatchSucceeded(batch_id);
    } catch (...) {
        HandleApiException();
    }
}

void FountainMarkUploadBatchFailed(const char *batch_id, const int http_status, const char *error_message) {
    try {
        if (batch_id == nullptr || batch_id[0] == '\0') {
            return;
        }
        const std::string error = error_message != nullptr ? error_message : "";
        fountain::GetRuntime().MarkUploadBatchFailed(batch_id, http_status, error);
    } catch (...) {
        HandleApiException();
    }
}

void FountainFreeUploadBatch(FountainUploadBatch *batch) {
    if (batch == nullptr) {
        return;
    }
    if (batch->batch_id != nullptr) {
        std::free(batch->batch_id);
        batch->batch_id = nullptr;
    }
    if (batch->json_payload != nullptr) {
        std::free(batch->json_payload);
        batch->json_payload = nullptr;
    }
    batch->json_payload_length = 0;
}

void FountainRunMaintenance(void) {
    try {
        fountain::GetRuntime().RunMaintenance();
    } catch (...) {
        HandleApiException();
    }
}

}  // extern "C"

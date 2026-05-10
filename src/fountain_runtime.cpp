#include "fountain_runtime.h"

#include "constants.h"
#include "id_utils.h"
#include "time_utils.h"

namespace fountain {

bool FountainRuntime::Configure(const std::string &database_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (database_path.empty()) {
        return false;
    }
    if (!repository_.Open(database_path) || !repository_.InitializeSchema()) {
        configured_ = false;
        return false;
    }
    configured_ = true;
    return true;
}

void FountainRuntime::SetInstallId(const std::string &install_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    identity_.install_id = install_id;
}

void FountainRuntime::SetSessionId(const std::string &session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    identity_.session_id = session_id;
}

void FountainRuntime::SetAppMetadata(const AppMetadata &metadata) {
    std::lock_guard<std::mutex> lock(mutex_);
    app_metadata_ = metadata;
}

void FountainRuntime::LogEvent(const EventInput &event) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!configured_) {
        return;
    }
    const auto now_ms = NowMs();
    const auto event_id = GenerateUuidV4();
    const auto payload = event_json_.BuildEventEnvelope(event, app_metadata_, identity_, event_id, now_ms);
    if (payload.size() > kMaxEventPayloadBytes) {
        return;
    }
    repository_.InsertEvent(
        event_id,
        now_ms,
        LevelToString(event.level),
        event.event_name,
        event.component,
        payload
    );
}

std::optional<BatchPayload> FountainRuntime::CreateUploadBatch(const UploadBatchLimits limits) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!configured_) {
        return std::nullopt;
    }
    const auto bounded_limits = UploadBatchLimits(
        MaxEventCount(limits.max_events()),
        MaxBatchBytes(std::min(limits.max_bytes(), kDefaultMaxBatchPayloadBytes))
    );
    return repository_.CreateUploadBatch(bounded_limits, NowMs());
}

void FountainRuntime::MarkUploadBatchSucceeded(const std::string &batch_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!configured_ || batch_id.empty()) {
        return;
    }
    repository_.MarkUploadBatchSucceeded(batch_id);
}

void FountainRuntime::MarkUploadBatchFailed(
    const std::string &batch_id,
    const int http_status,
    const std::string &error_message
) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!configured_ || batch_id.empty()) {
        return;
    }
    repository_.MarkUploadBatchFailed(batch_id, http_status, error_message, NowMs());
}

void FountainRuntime::RunMaintenance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!configured_) {
        return;
    }
    repository_.RunMaintenance(NowMs());
}

FountainRuntime &GetRuntime() {
    static FountainRuntime runtime;
    return runtime;
}

std::vector<EventField> CopyFields(const FountainLogField *fields, const std::size_t field_count) {
    std::vector<EventField> out;
    out.reserve(field_count);
    for (std::size_t i = 0; i < field_count; ++i) {
        const auto &src = fields[i];
        if (src.key == nullptr || src.key[0] == '\0') {
            continue;
        }
        EventField dst;
        dst.key = src.key;
        dst.privacy = src.privacy;
        dst.type = src.type;
        switch (src.type) {
            case FountainLogValueString:
                dst.string_value = src.value.string_value != nullptr ? src.value.string_value : "";
                break;
            case FountainLogValueInt64:
                dst.int64_value = src.value.int64_value;
                break;
            case FountainLogValueDouble:
                dst.double_value = src.value.double_value;
                break;
            case FountainLogValueBool:
                dst.bool_value = src.value.bool_value;
                break;
            default:
                break;
        }
        out.push_back(std::move(dst));
    }
    return out;
}

}  // namespace fountain

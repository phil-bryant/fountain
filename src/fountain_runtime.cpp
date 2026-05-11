#include "fountain_runtime.h"

#include "constants.h"
#include "id_utils.h"
#include "time_utils.h"

namespace fountain {

FountainRuntime::~FountainRuntime() {
    StopHeartbeat();
}

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
    LogEventLocked(event);
}

void FountainRuntime::LogEventLocked(const EventInput &event) {
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

bool FountainRuntime::StartHeartbeat(
    std::chrono::seconds interval,
    const std::string &event_name,
    const std::string &component,
    const std::string &target_install_id
) {
    bool started = false;
    std::chrono::seconds bounded_interval = interval;
    if (bounded_interval.count() <= 0) {
        bounded_interval = std::chrono::seconds(900);
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const bool can_start = !heartbeat_running_;
        if (can_start) {
            heartbeat_interval_ = bounded_interval;
            heartbeat_event_name_ = event_name;
            heartbeat_component_ = component;
            heartbeat_target_install_id_ = target_install_id;
            heartbeat_running_ = true;
            started = true;
        }
    }
    if (started) {
        try {
            heartbeat_thread_ = std::thread(&FountainRuntime::HeartbeatLoop, this);
        } catch (...) {
            std::lock_guard<std::mutex> lock(mutex_);
            heartbeat_running_ = false;
            throw;
        }
    }
    return started;
}

void FountainRuntime::StopHeartbeat() {
    std::thread thread_to_join;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (heartbeat_running_) {
            heartbeat_running_ = false;
            heartbeat_cv_.notify_all();
            thread_to_join = std::move(heartbeat_thread_);
        } else if (heartbeat_thread_.joinable()) {
            thread_to_join = std::move(heartbeat_thread_);
        }
    }
    if (thread_to_join.joinable()) {
        thread_to_join.join();
    }
}

void FountainRuntime::HeartbeatLoop() {
    std::unique_lock<std::mutex> lock(mutex_);
    bool keep_running = heartbeat_running_;
    while (keep_running) {
        const auto interval = heartbeat_interval_;
        const bool stop_signaled = heartbeat_cv_.wait_for(lock, interval, [this] { return !heartbeat_running_; });
        const bool should_emit = !stop_signaled && heartbeat_running_;
        if (should_emit) {
            bool rollout_match = heartbeat_target_install_id_.empty();
            if (!rollout_match) {
                rollout_match = !identity_.install_id.empty() && identity_.install_id == heartbeat_target_install_id_;
            }
            if (configured_ && rollout_match) {
                EventInput heartbeat_event;
                heartbeat_event.level = FountainLogLevelInfo;
                heartbeat_event.event_name = heartbeat_event_name_;
                heartbeat_event.component = heartbeat_component_;
                EventField interval_field;
                interval_field.key = "interval_seconds";
                interval_field.privacy = FountainLogPrivacyPublic;
                interval_field.type = FountainLogValueInt64;
                interval_field.int64_value = static_cast<std::int64_t>(heartbeat_interval_.count());
                heartbeat_event.fields.push_back(std::move(interval_field));
                LogEventLocked(heartbeat_event);
            }
        }
        keep_running = heartbeat_running_;
    }
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

#ifndef FOUNTAIN_SRC_FOUNTAIN_RUNTIME_H_
#define FOUNTAIN_SRC_FOUNTAIN_RUNTIME_H_

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "event_json.h"
#include "fountain_types.h"
#include "queue_repository.h"

namespace fountain {

class FountainRuntime {
public:
    FountainRuntime() = default;
    ~FountainRuntime();

    bool Configure(const std::string &database_path);

    void SetInstallId(const std::string &install_id);
    void SetSessionId(const std::string &session_id);
    void SetAppMetadata(const AppMetadata &metadata);

    void LogEvent(const EventInput &event);
    std::optional<BatchPayload> CreateUploadBatch(UploadBatchLimits limits);
    void MarkUploadBatchSucceeded(const std::string &batch_id);
    void MarkUploadBatchFailed(const std::string &batch_id, int http_status, const std::string &error_message);
    void RunMaintenance();
    bool StartHeartbeat(std::chrono::seconds interval, const std::string &event_name, const std::string &component,
                        const std::string &target_install_id);
    void StopHeartbeat();

private:
    void LogEventLocked(const EventInput &event);
    void HeartbeatLoop();

    mutable std::mutex mutex_;
    bool configured_ = false;
    QueueRepository repository_;
    EventJson event_json_;
    AppMetadata app_metadata_;
    IdentityMetadata identity_;
    bool heartbeat_running_ = false;
    std::chrono::seconds heartbeat_interval_ = std::chrono::seconds(900);
    std::string heartbeat_event_name_ = "fountain.heartbeat";
    std::string heartbeat_component_ = "fountain.runtime";
    std::string heartbeat_target_install_id_;
    std::thread heartbeat_thread_;
    std::condition_variable heartbeat_cv_;
};

FountainRuntime &GetRuntime();

std::vector<EventField> CopyFields(const FountainLogField *fields, std::size_t field_count);

}  // namespace fountain
#endif

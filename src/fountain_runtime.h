#ifndef FOUNTAIN_SRC_FOUNTAIN_RUNTIME_H_
#define FOUNTAIN_SRC_FOUNTAIN_RUNTIME_H_

#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "event_json.h"
#include "fountain_types.h"
#include "queue_repository.h"

namespace fountain {

class FountainRuntime {
public:
    bool Configure(const std::string &database_path);

    void SetInstallId(const std::string &install_id);
    void SetSessionId(const std::string &session_id);
    void SetAppMetadata(const AppMetadata &metadata);

    void LogEvent(const EventInput &event);
    std::optional<BatchPayload> CreateUploadBatch(UploadBatchLimits limits);
    void MarkUploadBatchSucceeded(const std::string &batch_id);
    void MarkUploadBatchFailed(const std::string &batch_id, int http_status, const std::string &error_message);
    void RunMaintenance();

private:
    mutable std::mutex mutex_;
    bool configured_ = false;
    QueueRepository repository_;
    EventJson event_json_;
    AppMetadata app_metadata_;
    IdentityMetadata identity_;
};

FountainRuntime &GetRuntime();

std::vector<EventField> CopyFields(const FountainLogField *fields, std::size_t field_count);

}  // namespace fountain
#endif

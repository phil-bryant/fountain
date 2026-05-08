#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "fountain_types.h"
#include "sqlite/db.h"

namespace fountain {

class QueueRepository {
public:
    QueueRepository() = default;

    bool Open(const std::string &db_path);
    bool InitializeSchema() const;

    bool InsertEvent(
        const std::string &event_id,
        std::int64_t created_at_ms,
        const std::string &level,
        const std::string &event_name,
        const std::string &component,
        const std::string &payload_json
    ) const;

    std::optional<BatchPayload> CreateUploadBatch(
        std::size_t max_events,
        std::size_t max_bytes,
        std::int64_t now_ms
    ) const;

    void MarkUploadBatchSucceeded(const std::string &batch_id) const;
    void MarkUploadBatchFailed(
        const std::string &batch_id,
        int http_status,
        const std::string &error_message,
        std::int64_t now_ms
    ) const;
    void RunMaintenance(std::int64_t now_ms) const;

private:
    static bool IsPermanentFailure(int http_status);
    void EnforceRowLimit() const;

    sqlite::Database db_;
};

}  // namespace fountain

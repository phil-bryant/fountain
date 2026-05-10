#ifndef FOUNTAIN_SRC_QUEUE_REPOSITORY_H_
#define FOUNTAIN_SRC_QUEUE_REPOSITORY_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "fountain_types.h"
#include "sqlite/db.h"

namespace fountain {

class MaxEventCount {
public:
    explicit MaxEventCount(std::size_t value) : value_(value) {}
    [[nodiscard]] std::size_t value() const { return value_; }

private:
    std::size_t value_;
};

class MaxBatchBytes {
public:
    explicit MaxBatchBytes(std::size_t value) : value_(value) {}
    [[nodiscard]] std::size_t value() const { return value_; }

private:
    std::size_t value_;
};

class UploadBatchLimits {
public:
    UploadBatchLimits(MaxEventCount max_events, MaxBatchBytes max_bytes)
        : max_events_(max_events.value()), max_bytes_(max_bytes.value()) {}
    [[nodiscard]] std::size_t max_events() const { return max_events_; }
    [[nodiscard]] std::size_t max_bytes() const { return max_bytes_; }

private:
    std::size_t max_events_;
    std::size_t max_bytes_;
};

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

    std::optional<BatchPayload> CreateUploadBatch(UploadBatchLimits limits, std::int64_t now_ms) const;

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
#endif

#include "queue_repository.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include <nlohmann/json.hpp>

#include "backoff.h"
#include "constants.h"
#include "id_utils.h"
#include "sqlite/statement.h"
#include "time_utils.h"

namespace fountain {

namespace {

std::string JoinIdsCsv(const std::vector<std::int64_t> &ids) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << ids[i];
    }
    return oss.str();
}

}  // namespace

bool QueueRepository::Open(const std::string &db_path) {
    return db_.Open(db_path);
}

bool QueueRepository::InitializeSchema() const {
    return db_.Exec("PRAGMA journal_mode=WAL;") && db_.Exec("PRAGMA synchronous=NORMAL;") &&
           db_.Exec(
               "CREATE TABLE IF NOT EXISTS fountain_event_queue ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "event_id TEXT NOT NULL UNIQUE,"
               "batch_id TEXT,"
               "created_at_ms INTEGER NOT NULL,"
               "next_attempt_at_ms INTEGER NOT NULL DEFAULT 0,"
               "level TEXT NOT NULL,"
               "event_name TEXT NOT NULL,"
               "component TEXT NOT NULL,"
               "payload_json TEXT NOT NULL,"
               "attempt_count INTEGER NOT NULL DEFAULT 0,"
               "last_error TEXT,"
               "state TEXT NOT NULL DEFAULT 'pending' CHECK (state IN ('pending','uploading','uploaded','dropped'))"
               ");"
           ) &&
           db_.Exec(
               "CREATE INDEX IF NOT EXISTS idx_fountain_queue_state_next_attempt "
               "ON fountain_event_queue(state, next_attempt_at_ms, id);"
           ) &&
           db_.Exec("CREATE INDEX IF NOT EXISTS idx_fountain_queue_batch_id ON fountain_event_queue(batch_id);") &&
           db_.Exec("CREATE INDEX IF NOT EXISTS idx_fountain_queue_created_at ON fountain_event_queue(created_at_ms);");
}

bool QueueRepository::InsertEvent(
    const std::string &event_id,
    const std::int64_t created_at_ms,
    const std::string &level,
    const std::string &event_name,
    const std::string &component,
    const std::string &payload_json
) const {
    auto stmt = db_.Prepare(
        "INSERT INTO fountain_event_queue("
        "event_id, batch_id, created_at_ms, next_attempt_at_ms, level, event_name, component, payload_json, attempt_count, state"
        ") VALUES(?, NULL, ?, 0, ?, ?, ?, ?, 0, 'pending');"
    );
    if (!stmt.IsValid()) {
        return false;
    }

    return stmt.BindText(1, event_id) && stmt.BindInt64(2, created_at_ms) && stmt.BindText(3, level) &&
           stmt.BindText(4, event_name) && stmt.BindText(5, component) && stmt.BindText(6, payload_json) &&
           !stmt.Step() && stmt.Done();
}

std::optional<BatchPayload> QueueRepository::CreateUploadBatch(
    const std::size_t max_events,
    const std::size_t max_bytes,
    const std::int64_t now_ms
) const {
    if (max_events == 0 || max_bytes == 0) {
        return std::nullopt;
    }

    if (!db_.Exec("BEGIN IMMEDIATE TRANSACTION;")) {
        return std::nullopt;
    }

    auto query = db_.Prepare(
        "SELECT id, payload_json FROM fountain_event_queue "
        "WHERE state='pending' AND next_attempt_at_ms<=? "
        "ORDER BY id ASC;"
    );
    if (!query.IsValid() || !query.BindInt64(1, now_ms)) {
        db_.Exec("ROLLBACK;");
        return std::nullopt;
    }

    std::vector<std::int64_t> ids;
    nlohmann::json event_array = nlohmann::json::array();
    std::size_t estimated_bytes = 0;

    while (query.Step()) {
        if (ids.size() >= max_events) {
            break;
        }

        const auto id = query.ColumnInt64(0);
        const auto payload_json = query.ColumnText(1);
        const std::size_t candidate_size = payload_json.size();

        if (!ids.empty() && estimated_bytes + candidate_size > max_bytes) {
            break;
        }
        if (ids.empty() && candidate_size > max_bytes) {
            continue;
        }

        ids.push_back(id);
        estimated_bytes += candidate_size;
        event_array.push_back(nlohmann::json::parse(payload_json, nullptr, false));
        if (event_array.back().is_discarded()) {
            event_array.pop_back();
            ids.pop_back();
            estimated_bytes -= candidate_size;
        }
    }

    if (ids.empty()) {
        db_.Exec("ROLLBACK;");
        return std::nullopt;
    }

    const std::string batch_id = GenerateUuidV4();
    nlohmann::json batch = {
        {"batch_id", batch_id},
        {"sent_at", ToIso8601UtcMs(now_ms)},
        {"events", event_array},
    };
    std::string payload = batch.dump();

    while (!ids.empty() && payload.size() > max_bytes) {
        ids.pop_back();
        event_array.erase(event_array.end() - 1);
        batch["events"] = event_array;
        payload = batch.dump();
    }

    if (ids.empty()) {
        db_.Exec("ROLLBACK;");
        return std::nullopt;
    }

    const std::string ids_csv = JoinIdsCsv(ids);
    auto update = db_.Prepare(
        "UPDATE fountain_event_queue "
        "SET state='uploading', batch_id=?, next_attempt_at_ms=? "
        "WHERE id IN (" +
        ids_csv + ");"
    );
    if (!update.IsValid() || !update.BindText(1, batch_id) ||
        !update.BindInt64(2, now_ms + kUploadingStaleTimeoutMs) || update.Step() || !update.Done()) {
        db_.Exec("ROLLBACK;");
        return std::nullopt;
    }

    if (!db_.Exec("COMMIT;")) {
        db_.Exec("ROLLBACK;");
        return std::nullopt;
    }

    return BatchPayload{batch_id, payload};
}

void QueueRepository::MarkUploadBatchSucceeded(const std::string &batch_id) const {
    auto stmt = db_.Prepare("DELETE FROM fountain_event_queue WHERE batch_id=?;");
    if (!stmt.IsValid() || !stmt.BindText(1, batch_id)) {
        return;
    }
    stmt.Step();
}

void QueueRepository::MarkUploadBatchFailed(
    const std::string &batch_id,
    const int http_status,
    const std::string &error_message,
    const std::int64_t now_ms
) const {
    auto rows = db_.Prepare("SELECT id, attempt_count FROM fountain_event_queue WHERE batch_id=?;");
    if (!rows.IsValid() || !rows.BindText(1, batch_id)) {
        return;
    }

    std::vector<std::pair<std::int64_t, int>> attempts;
    while (rows.Step()) {
        attempts.emplace_back(rows.ColumnInt64(0), static_cast<int>(rows.ColumnInt64(1)));
    }

    if (attempts.empty()) {
        return;
    }

    if (!db_.Exec("BEGIN IMMEDIATE TRANSACTION;")) {
        return;
    }

    for (const auto &[id, attempt] : attempts) {
        if (IsPermanentFailure(http_status)) {
            auto update = db_.Prepare(
                "UPDATE fountain_event_queue SET state='dropped', batch_id=NULL, last_error=?, next_attempt_at_ms=0 WHERE id=?;"
            );
            if (!update.IsValid() || !update.BindText(1, error_message) || !update.BindInt64(2, id) || update.Step() ||
                !update.Done()) {
                db_.Exec("ROLLBACK;");
                return;
            }
        } else {
            const int next_attempt = attempt + 1;
            const auto next_attempt_at_ms = now_ms + ComputeBackoffDelayMs(next_attempt);
            auto update = db_.Prepare(
                "UPDATE fountain_event_queue "
                "SET state='pending', batch_id=NULL, attempt_count=?, last_error=?, next_attempt_at_ms=? "
                "WHERE id=?;"
            );
            if (!update.IsValid() || !update.BindInt(1, next_attempt) || !update.BindText(2, error_message) ||
                !update.BindInt64(3, next_attempt_at_ms) || !update.BindInt64(4, id) || update.Step() ||
                !update.Done()) {
                db_.Exec("ROLLBACK;");
                return;
            }
        }
    }

    db_.Exec("COMMIT;");
}

void QueueRepository::RunMaintenance(const std::int64_t now_ms) const {
    {
        auto reset_stale = db_.Prepare(
            "UPDATE fountain_event_queue "
            "SET state='pending', batch_id=NULL "
            "WHERE state='uploading' AND next_attempt_at_ms<=?;"
        );
        if (reset_stale.IsValid() && reset_stale.BindInt64(1, now_ms)) {
            reset_stale.Step();
        }
    }

    {
        auto prune_old = db_.Prepare("DELETE FROM fountain_event_queue WHERE created_at_ms < ?;");
        if (prune_old.IsValid() && prune_old.BindInt64(1, now_ms - kMaxEventAgeMs)) {
            prune_old.Step();
        }
    }

    {
        auto prune_dropped = db_.Prepare("DELETE FROM fountain_event_queue WHERE state='dropped';");
        if (prune_dropped.IsValid()) {
            prune_dropped.Step();
        }
    }

    EnforceRowLimit();
}

bool QueueRepository::IsPermanentFailure(const int http_status) {
    return http_status == 400 || http_status == 422;
}

void QueueRepository::EnforceRowLimit() const {
    auto count_stmt = db_.Prepare("SELECT COUNT(*) FROM fountain_event_queue;");
    if (!count_stmt.IsValid() || !count_stmt.Step()) {
        return;
    }
    const auto row_count = static_cast<std::size_t>(count_stmt.ColumnInt64(0));
    if (row_count <= kMaxQueueRows) {
        return;
    }

    std::size_t excess = row_count - kMaxQueueRows;

    const std::vector<std::string> severity_groups = {
        "('debug','info')",
        "('warning')",
        "('error')",
        "('fault')",
    };

    for (const auto &group : severity_groups) {
        if (excess == 0) {
            break;
        }
        auto select = db_.Prepare(
            "SELECT id FROM fountain_event_queue WHERE state IN ('pending','dropped') AND level IN " + group +
            " ORDER BY created_at_ms ASC LIMIT ?;"
        );
        if (!select.IsValid() || !select.BindInt(1, static_cast<int>(excess))) {
            continue;
        }
        std::vector<std::int64_t> ids;
        while (select.Step()) {
            ids.push_back(select.ColumnInt64(0));
        }
        if (ids.empty()) {
            continue;
        }

        auto del = db_.Prepare("DELETE FROM fountain_event_queue WHERE id IN (" + JoinIdsCsv(ids) + ");");
        if (del.IsValid()) {
            del.Step();
        }
        excess = (ids.size() >= excess) ? 0 : (excess - ids.size());
    }
}

}  // namespace fountain

#include <gtest/gtest.h>
#include <sqlite3.h>

#include <nlohmann/json.hpp>

#include "fountain/fountain.h"
#include "test_utils.h"

namespace {

void SeedEvent(const char *name, const char *component = "core") {
    FountainLogField field {};
    field.key = "value";
    field.privacy = FountainLogPrivacyPublic;
    field.type = FountainLogValueInt64;
    field.value.int64_value = 1;
    FountainLogEvent(FountainLogLevelInfo, name, component, &field, 1);
}

}  // namespace

TEST(FountainBatchTests, ClaimsBatchAndMarksRowsUploading) {
    const auto db_path = TestDbPath("fountain_claim");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));
    SeedEvent("e1");
    SeedEvent("e2");
    SeedEvent("e3");

    FountainUploadBatch batch {};
    ASSERT_TRUE(FountainCreateUploadBatch(2, 4096, &batch));
    ASSERT_NE(batch.batch_id, nullptr);
    ASSERT_NE(batch.json_payload, nullptr);
    const auto json = nlohmann::json::parse(batch.json_payload);
    ASSERT_EQ(json["events"].size(), 2);

    sqlite3 *db = nullptr;
    ASSERT_EQ(SQLITE_OK, sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr));
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(
        SQLITE_OK,
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM fountain_event_queue WHERE state='uploading';", -1, &stmt, nullptr)
    );
    ASSERT_EQ(SQLITE_ROW, sqlite3_step(stmt));
    ASSERT_EQ(sqlite3_column_int64(stmt, 0), 2);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    FountainFreeUploadBatch(&batch);
    RemoveFileIfExists(db_path);
}

TEST(FountainBatchTests, MarkSuccessDeletesRows) {
    const auto db_path = TestDbPath("fountain_success");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));
    SeedEvent("e1");

    FountainUploadBatch batch {};
    ASSERT_TRUE(FountainCreateUploadBatch(10, 65536, &batch));
    FountainMarkUploadBatchSucceeded(batch.batch_id);

    sqlite3 *db = nullptr;
    ASSERT_EQ(SQLITE_OK, sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr));
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLITE_OK, sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM fountain_event_queue;", -1, &stmt, nullptr));
    ASSERT_EQ(SQLITE_ROW, sqlite3_step(stmt));
    ASSERT_EQ(sqlite3_column_int64(stmt, 0), 0);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    FountainFreeUploadBatch(&batch);
    RemoveFileIfExists(db_path);
}

TEST(FountainBatchTests, MarkFailureSetsRetryMetadata) {
    const auto db_path = TestDbPath("fountain_retry");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));
    SeedEvent("e1");

    FountainUploadBatch batch {};
    ASSERT_TRUE(FountainCreateUploadBatch(10, 65536, &batch));
    FountainMarkUploadBatchFailed(batch.batch_id, 500, "server unavailable");

    sqlite3 *db = nullptr;
    ASSERT_EQ(SQLITE_OK, sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr));
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(
        SQLITE_OK,
        sqlite3_prepare_v2(
            db,
            "SELECT state, attempt_count, next_attempt_at_ms, last_error FROM fountain_event_queue LIMIT 1;",
            -1,
            &stmt,
            nullptr
        )
    );
    ASSERT_EQ(SQLITE_ROW, sqlite3_step(stmt));
    ASSERT_STREQ(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), "pending");
    ASSERT_GE(sqlite3_column_int(stmt, 1), 1);
    ASSERT_GT(sqlite3_column_int64(stmt, 2), 0);
    ASSERT_STREQ(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)), "server unavailable");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    FountainFreeUploadBatch(&batch);
    RemoveFileIfExists(db_path);
}

TEST(FountainBatchTests, PermanentFailureMarksDropped) {
    const auto db_path = TestDbPath("fountain_drop");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));
    SeedEvent("e1");

    FountainUploadBatch batch {};
    ASSERT_TRUE(FountainCreateUploadBatch(10, 65536, &batch));
    FountainMarkUploadBatchFailed(batch.batch_id, 422, "invalid schema");

    sqlite3 *db = nullptr;
    ASSERT_EQ(SQLITE_OK, sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr));
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(
        SQLITE_OK,
        sqlite3_prepare_v2(db, "SELECT state, last_error FROM fountain_event_queue LIMIT 1;", -1, &stmt, nullptr)
    );
    ASSERT_EQ(SQLITE_ROW, sqlite3_step(stmt));
    ASSERT_STREQ(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), "dropped");
    ASSERT_STREQ(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)), "invalid schema");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    FountainFreeUploadBatch(&batch);
    RemoveFileIfExists(db_path);
}

TEST(FountainBatchTests, RejectsOversizedEventPayload) {
    const auto db_path = TestDbPath("fountain_big");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));
    const FountainAppMetadata app_metadata = {"com.example.app", "1.0", "1", "macOS", "15.5", "arm64"};
    FountainSetAppMetadata(&app_metadata);
    FountainSetInstallID("install");
    FountainSetSessionID("session");

    std::string huge(20 * 1024, 'x');
    FountainLogField field {};
    field.key = "blob";
    field.privacy = FountainLogPrivacyPublic;
    field.type = FountainLogValueString;
    field.value.string_value = huge.c_str();
    FountainLogEvent(FountainLogLevelInfo, "big_event", "test", &field, 1);

    sqlite3 *db = nullptr;
    ASSERT_EQ(SQLITE_OK, sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr));
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLITE_OK, sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM fountain_event_queue;", -1, &stmt, nullptr));
    ASSERT_EQ(SQLITE_ROW, sqlite3_step(stmt));
    ASSERT_EQ(sqlite3_column_int64(stmt, 0), 0);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    RemoveFileIfExists(db_path);
}

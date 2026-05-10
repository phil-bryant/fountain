#include <gtest/gtest.h>
#include <sqlite3.h>

#include <nlohmann/json.hpp>

#include "fountain/fountain.h"
#include "test_utils.h"

TEST(FountainJsonTests, PersistsExpectedEnvelopeShape) {
    const auto db_path = TestDbPath("fountain_json");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));
    FountainSetInstallID("install-xyz");
    FountainSetSessionID("session-abc");
    const FountainAppMetadata app_metadata = {"com.example.app", "1.8.3", "1842", "macOS", "15.5", "arm64"};
    FountainSetAppMetadata(&app_metadata);

    FountainLogField field {};
    field.key = "duration_ms";
    field.privacy = FountainLogPrivacyPublic;
    field.type = FountainLogValueInt64;
    field.value.int64_value = 842;
    FountainLogEvent(FountainLogLevelError, "document_export_failed", "export", &field, 1);

    sqlite3 *db = nullptr;
    ASSERT_EQ(SQLITE_OK, sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr));
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(
        SQLITE_OK,
        sqlite3_prepare_v2(db, "SELECT payload_json FROM fountain_event_queue LIMIT 1;", -1, &stmt, nullptr)
    );
    ASSERT_EQ(SQLITE_ROW, sqlite3_step(stmt));
    const char *json_text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    ASSERT_NE(json_text, nullptr);

    const auto payload = nlohmann::json::parse(json_text);
    ASSERT_EQ(payload["schema_version"], 1);
    ASSERT_EQ(payload["level"], "error");
    ASSERT_EQ(payload["event"], "document_export_failed");
    ASSERT_EQ(payload["component"], "export");
    ASSERT_TRUE(payload.contains("event_id"));
    ASSERT_TRUE(payload.contains("timestamp"));
    ASSERT_EQ(payload["app"]["bundle_id"], "com.example.app");
    ASSERT_EQ(payload["app"]["version"], "1.8.3");
    ASSERT_EQ(payload["app"]["build"], "1842");
    ASSERT_EQ(payload["device"]["os"], "macOS");
    ASSERT_EQ(payload["device"]["os_version"], "15.5");
    ASSERT_EQ(payload["device"]["arch"], "arm64");
    ASSERT_EQ(payload["identity"]["install_id"], "install-xyz");
    ASSERT_EQ(payload["identity"]["session_id"], "session-abc");
    ASSERT_EQ(payload["fields"]["duration_ms"], 842);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    RemoveFileIfExists(db_path);
}

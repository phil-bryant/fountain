#include <gtest/gtest.h>
#include <sqlite3.h>

#include <nlohmann/json.hpp>

#include "fountain/fountain.h"
#include "test_utils.h"

TEST(FountainPrivacyTests, FiltersPrivateAndForbiddenFields) {
    const auto db_path = TestDbPath("fountain_privacy");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));
    FountainSetInstallID("install-1");
    FountainSetSessionID("session-1");
    FountainSetAppMetadata("com.example.app", "1.0.0", "1", "macOS", "15.5", "arm64");

    FountainLogField fields[5] = {};
    fields[0].key = "format";
    fields[0].privacy = FountainLogPrivacyPublic;
    fields[0].type = FountainLogValueString;
    fields[0].value.string_value = "pdf";

    fields[1].key = "error_code";
    fields[1].privacy = FountainLogPrivacyPublic;
    fields[1].type = FountainLogValueString;
    fields[1].value.string_value = "permission_denied";

    fields[2].key = "duration_ms";
    fields[2].privacy = FountainLogPrivacyPublic;
    fields[2].type = FountainLogValueInt64;
    fields[2].value.int64_value = 842;

    fields[3].key = "destination_path";
    fields[3].privacy = FountainLogPrivacyPrivate;
    fields[3].type = FountainLogValueString;
    fields[3].value.string_value = "/Users/person/Desktop/file.pdf";

    fields[4].key = "auth_token";
    fields[4].privacy = FountainLogPrivacyForbidden;
    fields[4].type = FountainLogValueString;
    fields[4].value.string_value = "secret";

    FountainLogEvent(FountainLogLevelError, "document_export_failed", "export", fields, 5);

    sqlite3 *db = nullptr;
    ASSERT_EQ(SQLITE_OK, sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr));
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(
        SQLITE_OK,
        sqlite3_prepare_v2(db, "SELECT payload_json FROM fountain_event_queue LIMIT 1;", -1, &stmt, nullptr)
    );
    ASSERT_EQ(SQLITE_ROW, sqlite3_step(stmt));
    const char *text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    ASSERT_NE(text, nullptr);
    const auto payload = nlohmann::json::parse(text);

    ASSERT_EQ(payload["fields"]["format"], "pdf");
    ASSERT_EQ(payload["fields"]["error_code"], "permission_denied");
    ASSERT_EQ(payload["fields"]["duration_ms"], 842);
    ASSERT_FALSE(payload["fields"].contains("destination_path"));
    ASSERT_FALSE(payload["fields"].contains("auth_token"));

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    RemoveFileIfExists(db_path);
}

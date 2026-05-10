#include <gtest/gtest.h>
#include <sqlite3.h>

#include "fountain/fountain.h"
#include "test_utils.h"

TEST(FountainSqliteTests, InsertsPendingRow) {
    const auto db_path = TestDbPath("fountain_insert");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));
    const FountainAppMetadata app_metadata = {"com.example.app", "1.0", "1", "macOS", "15.5", "arm64"};
    FountainSetAppMetadata(&app_metadata);
    FountainSetInstallID("install");
    FountainSetSessionID("session");

    FountainLogEvent(FountainLogLevelInfo, "startup", "app", nullptr, 0);

    sqlite3 *db = nullptr;
    ASSERT_EQ(SQLITE_OK, sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr));
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(
        SQLITE_OK,
        sqlite3_prepare_v2(
            db,
            "SELECT state, level, event_name, component FROM fountain_event_queue LIMIT 1;",
            -1,
            &stmt,
            nullptr
        )
    );
    ASSERT_EQ(SQLITE_ROW, sqlite3_step(stmt));
    ASSERT_STREQ(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), "pending");
    ASSERT_STREQ(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)), "info");
    ASSERT_STREQ(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)), "startup");
    ASSERT_STREQ(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)), "app");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    RemoveFileIfExists(db_path);
}

#include <gtest/gtest.h>
#include <sqlite3.h>

#include <chrono>
#include <thread>

#include "fountain/fountain.h"
#include "test_utils.h"

namespace {

int64_t CountEventsByName(const std::string &db_path, const std::string &event_name) {
    int64_t count = 0;
    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK) {
        if (sqlite3_prepare_v2(
                db,
                "SELECT COUNT(*) FROM fountain_event_queue WHERE event_name = ?;",
                -1,
                &stmt,
                nullptr
            ) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, event_name.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                count = sqlite3_column_int64(stmt, 0);
            }
        }
    }
    if (stmt != nullptr) sqlite3_finalize(stmt);
    if (db != nullptr) sqlite3_close(db);
    return count;
}

}  // namespace

TEST(FountainHeartbeatTests, EmitsHeartbeatWhenInstallIdMatchesRolloutGate) {
    const auto db_path = TestDbPath("fountain_heartbeat_match");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));
    FountainSetInstallID("install-match");

    FountainHeartbeatConfig config {};
    config.interval_seconds = 1;
    config.event_name = "fountain.heartbeat";
    config.component = "fountain.runtime";
    config.target_install_id = "install-match";
    ASSERT_TRUE(FountainStartHeartbeat(&config));
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    FountainStopHeartbeat();

    EXPECT_GE(CountEventsByName(db_path, "fountain.heartbeat"), 1);
    RemoveFileIfExists(db_path);
}

TEST(FountainHeartbeatTests, SkipsHeartbeatWhenInstallIdDoesNotMatchRolloutGate) {
    const auto db_path = TestDbPath("fountain_heartbeat_mismatch");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));
    FountainSetInstallID("install-a");

    FountainHeartbeatConfig config {};
    config.interval_seconds = 1;
    config.event_name = "fountain.heartbeat";
    config.component = "fountain.runtime";
    config.target_install_id = "install-b";
    ASSERT_TRUE(FountainStartHeartbeat(&config));
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    FountainStopHeartbeat();

    EXPECT_EQ(CountEventsByName(db_path, "fountain.heartbeat"), 0);
    RemoveFileIfExists(db_path);
}

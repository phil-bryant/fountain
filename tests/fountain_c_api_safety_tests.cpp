#include <gtest/gtest.h>

#include "fountain/fountain.h"
#include "test_utils.h"

TEST(FountainCApiSafetyTests, NullInputSafetyDoesNotCrashAndReturnsFalseWhenApplicable) {
    EXPECT_FALSE(FountainConfigure(nullptr));
    FountainSetInstallID(nullptr);
    FountainSetSessionID(nullptr);
    FountainSetAppMetadata(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    FountainLogEvent(FountainLogLevelInfo, nullptr, "component", nullptr, 0);
    FountainLogEvent(FountainLogLevelInfo, "event", nullptr, nullptr, 0);

    EXPECT_FALSE(FountainCreateUploadBatch(10, 1024, nullptr));
    FountainMarkUploadBatchSucceeded(nullptr);
    FountainMarkUploadBatchFailed(nullptr, 500, nullptr);
    FountainFreeUploadBatch(nullptr);
    FountainRunMaintenance();
}

TEST(FountainCApiSafetyTests, CreateUploadBatchNoRowsReturnsFalse) {
    const auto db_path = TestDbPath("fountain_empty_batch");
    RemoveFileIfExists(db_path);
    ASSERT_TRUE(FountainConfigure(db_path.c_str()));

    FountainUploadBatch batch {};
    EXPECT_FALSE(FountainCreateUploadBatch(10, 1024, &batch));
    EXPECT_EQ(batch.batch_id, nullptr);
    EXPECT_EQ(batch.json_payload, nullptr);
    EXPECT_EQ(batch.json_payload_length, 0u);

    RemoveFileIfExists(db_path);
}

#include <iostream>

#include "fountain/fountain.h"

int main() {
    if (!FountainConfigure("/tmp/fountain_example.sqlite3")) {
        std::cerr << "failed to configure Fountain\n";
        return 1;
    }

    FountainSetInstallID("install-id-123");
    FountainSetSessionID("session-id-123");
    FountainSetAppMetadata("com.example.app", "1.0.0", "1", "macOS", "15.5", "arm64");

    FountainLogField fields[2] {};
    fields[0].key = "format";
    fields[0].privacy = FountainLogPrivacyPublic;
    fields[0].type = FountainLogValueString;
    fields[0].value.string_value = "pdf";

    fields[1].key = "destination_path";
    fields[1].privacy = FountainLogPrivacyPrivate;
    fields[1].type = FountainLogValueString;
    fields[1].value.string_value = "/Users/person/Desktop/file.pdf";

    FountainLogEvent(FountainLogLevelError, "document_export_failed", "export", fields, 2);

    FountainUploadBatch batch {};
    if (FountainCreateUploadBatch(100, 512 * 1024, &batch)) {
        std::cout << "batch_id: " << batch.batch_id << "\n";
        std::cout << "payload: " << batch.json_payload << "\n";
        FountainMarkUploadBatchSucceeded(batch.batch_id);
        FountainFreeUploadBatch(&batch);
    }

    FountainRunMaintenance();
    return 0;
}

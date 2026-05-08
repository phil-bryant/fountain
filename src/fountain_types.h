#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "fountain/fountain.h"

namespace fountain {

struct AppMetadata {
    std::string bundle_id;
    std::string app_version;
    std::string build;
    std::string os_name;
    std::string os_version;
    std::string arch;
};

struct IdentityMetadata {
    std::string install_id;
    std::string session_id;
};

struct EventField {
    std::string key;
    FountainLogPrivacy privacy;
    FountainLogValueType type;
    std::string string_value;
    std::int64_t int64_value = 0;
    double double_value = 0.0;
    bool bool_value = false;
};

struct EventInput {
    FountainLogLevel level;
    std::string event_name;
    std::string component;
    std::vector<EventField> fields;
};

struct QueuedEventRecord {
    std::int64_t id = 0;
    std::string payload_json;
};

struct BatchPayload {
    std::string batch_id;
    std::string json_payload;
};

}  // namespace fountain

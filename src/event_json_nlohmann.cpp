#include "event_json.h"

#include <nlohmann/json.hpp>

#include "time_utils.h"

namespace fountain {

std::string LevelToString(const FountainLogLevel level) {
    switch (level) {
        case FountainLogLevelDebug:
            return "debug";
        case FountainLogLevelInfo:
            return "info";
        case FountainLogLevelWarning:
            return "warning";
        case FountainLogLevelError:
            return "error";
        case FountainLogLevelFault:
            return "fault";
    }
    return "info";
}

std::string EventJson::BuildEventEnvelope(
    const EventInput &event,
    const AppMetadata &app,
    const IdentityMetadata &identity,
    const std::string &event_id,
    const std::int64_t timestamp_ms
) const {
    nlohmann::json fields = nlohmann::json::object();
    for (const auto &field : event.fields) {
        if (field.privacy != FountainLogPrivacyPublic) {
            continue;
        }
        switch (field.type) {
            case FountainLogValueString:
                fields[field.key] = field.string_value;
                break;
            case FountainLogValueInt64:
                fields[field.key] = field.int64_value;
                break;
            case FountainLogValueDouble:
                fields[field.key] = field.double_value;
                break;
            case FountainLogValueBool:
                fields[field.key] = field.bool_value;
                break;
        }
    }

    nlohmann::json envelope = {
        {"schema_version", 1},
        {"event_id", event_id},
        {"timestamp", ToIso8601UtcMs(timestamp_ms)},
        {"level", LevelToString(event.level)},
        {"event", event.event_name},
        {"component", event.component},
        {"app",
         {
             {"bundle_id", app.bundle_id},
             {"version", app.app_version},
             {"build", app.build},
         }},
        {"device",
         {
             {"os", app.os_name},
             {"os_version", app.os_version},
             {"arch", app.arch},
         }},
        {"identity",
         {
             {"install_id", identity.install_id},
             {"session_id", identity.session_id},
         }},
        {"fields", fields},
    };

    return envelope.dump();
}

}  // namespace fountain

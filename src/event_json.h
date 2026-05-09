#ifndef FOUNTAIN_SRC_EVENT_JSON_H_
#define FOUNTAIN_SRC_EVENT_JSON_H_

#include <string>

#include "fountain_types.h"

namespace fountain {

class EventJson {
public:
    std::string BuildEventEnvelope(
        const EventInput &event,
        const AppMetadata &app,
        const IdentityMetadata &identity,
        const std::string &event_id,
        std::int64_t timestamp_ms
    ) const;
};

std::string LevelToString(FountainLogLevel level);

}  // namespace fountain
#endif

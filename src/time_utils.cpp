#include "time_utils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fountain {

std::int64_t NowMs() {
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

std::string ToIso8601UtcMs(const std::int64_t unix_ms) {
    const std::time_t t = static_cast<std::time_t>(unix_ms / 1000);
    std::tm tm_utc {};
#if defined(_WIN32)
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    const auto millis = static_cast<int>(unix_ms % 1000);
    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%S") << "." << std::setfill('0') << std::setw(3)
        << millis << "Z";
    return oss.str();
}

}  // namespace fountain

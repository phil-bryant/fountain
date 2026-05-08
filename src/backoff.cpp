#include "backoff.h"

#include <algorithm>
#include <random>

namespace fountain {

std::int64_t ComputeBackoffDelayMs(const int attempt_count) {
    static constexpr std::int64_t kBaseDelayMs = 1000;
    static constexpr std::int64_t kMaxDelayMs = 30LL * 60 * 1000;

    const int clamped_attempt = std::max(1, attempt_count);
    std::int64_t delay = kBaseDelayMs;
    for (int i = 1; i < clamped_attempt; ++i) {
        delay = std::min(kMaxDelayMs, delay * 2);
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    const std::int64_t jitter_span = std::max<std::int64_t>(100, delay / 4);
    std::uniform_int_distribution<std::int64_t> dist(-jitter_span, jitter_span);
    return std::max<std::int64_t>(kBaseDelayMs, delay + dist(gen));
}

}  // namespace fountain

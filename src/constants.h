#pragma once

#include <cstddef>
#include <cstdint>

namespace fountain {

constexpr std::size_t kMaxEventPayloadBytes = 16 * 1024;
constexpr std::size_t kDefaultMaxBatchPayloadBytes = 512 * 1024;
constexpr std::size_t kMaxQueueRows = 50000;
constexpr std::int64_t kMaxEventAgeMs = 7LL * 24 * 60 * 60 * 1000;
constexpr std::int64_t kUploadingStaleTimeoutMs = 15LL * 60 * 1000;

}  // namespace fountain

#ifndef FOUNTAIN_SRC_CONSTANTS_H_
#define FOUNTAIN_SRC_CONSTANTS_H_

#include <cstddef>
#include <cstdint>

namespace fountain {

constexpr std::size_t kMaxEventPayloadBytes = std::size_t{16} * std::size_t{1024};
constexpr std::size_t kDefaultMaxBatchPayloadBytes = std::size_t{512} * std::size_t{1024};
constexpr std::size_t kMaxQueueRows = 50000;
constexpr std::int64_t kMaxEventAgeMs = 7LL * 24 * 60 * 60 * 1000;
constexpr std::int64_t kUploadingStaleTimeoutMs = 15LL * 60 * 1000;

}  // namespace fountain
#endif

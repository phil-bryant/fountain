#ifndef FOUNTAIN_SRC_BACKOFF_H_
#define FOUNTAIN_SRC_BACKOFF_H_

#include <cstdint>

namespace fountain {

std::int64_t ComputeBackoffDelayMs(int attempt_count);

}  // namespace fountain
#endif

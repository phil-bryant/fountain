#pragma once

#include <cstdint>

namespace fountain {

std::int64_t ComputeBackoffDelayMs(int attempt_count);

}  // namespace fountain

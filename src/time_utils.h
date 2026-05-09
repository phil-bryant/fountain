#ifndef FOUNTAIN_SRC_TIME_UTILS_H_
#define FOUNTAIN_SRC_TIME_UTILS_H_

#include <cstdint>
#include <string>

namespace fountain {

std::int64_t NowMs();
std::string ToIso8601UtcMs(std::int64_t unix_ms);

}  // namespace fountain
#endif

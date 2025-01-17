#include "internal/utils.h"
#include <cstring>
#include <chrono>

namespace constellation {

template <typename Duration = std::chrono::milliseconds>
constexpr int64_t get_time_point() {
  static_assert(std::is_same_v<Duration, std::chrono::milliseconds> ||
                    std::is_same_v<Duration, std::chrono::microseconds>,
                "Duration must be milliseconds or microseconds");

  auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<Duration>(now.time_since_epoch()).count();
}

// Backward compatibility wrapper
int64_t get_time_point(const char* unit) {
  if (unit == nullptr || std::strcmp(unit, "ms") == 0) {
    return get_time_point<>();  // default is milliseconds
  } else if (std::strcmp(unit, "us") == 0) {
    return get_time_point<std::chrono::microseconds>();
  }
  throw std::invalid_argument("unit must be 'ms' or 'us'");
}

}  // namespace constellation
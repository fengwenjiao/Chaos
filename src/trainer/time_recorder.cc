#include "time_recorder.h"
#include "internal/utils.h"

namespace constellation {

int64_t TimeRecoder::record() {
  auto temp = get_time_point(unit);
  std::lock_guard<std::mutex> lk(mu_);
  if (t > 0) {
    duration = temp - t;
  }
  t = temp;
  return duration;
}
int64_t TimeRecoder::now() {
  return t;
}
int64_t TimeRecoder::get_duration() const {
  std::lock_guard<std::mutex> lk(mu_);
  return duration;
}
}  // namespace constellation
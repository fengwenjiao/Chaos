#pragma once

#include <memory>
#include <mutex>

namespace constellation {

class TimeRecoder {
 public:
  TimeRecoder(const char* unit) : t(0), duration(0) {
    this->unit = unit;
  }
  int64_t record();
  int64_t now();
  int64_t get_duration() const;

 private:
  int64_t t;
  int64_t duration;
  const char* unit;
  mutable std::mutex mu_;
};
}  // namespace constellation
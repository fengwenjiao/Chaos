#ifndef __TRAINER_DEBUG_UTILS_H__
#define __TRAINER_DEBUG_UTILS_H__

#include <cstdint>

#include "../../include/constellation_trainer.h"

namespace constellation {

struct __ConstelTrainerTest {
  static uint32_t GetTimestamp(const ConstelTrainer& trainer) {
    return trainer.clock_.getLocalTimestamp();
  }
};

namespace test {

static uint32_t GetTimestamp(const ConstelTrainer& trainer) {
  return __ConstelTrainerTest::GetTimestamp(trainer);
}
}  // namespace test
}  // namespace constellation

#endif
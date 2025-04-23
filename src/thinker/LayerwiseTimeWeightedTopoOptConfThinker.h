#pragma once

#ifndef CONS_NETWORK_AWARE
#error "Use this thinker should enable network aware"
#endif

#include "./LayerwiseTimeWeightedConfThinker.h"

namespace constellation {

struct TranTopoExtra : public Extra {
  GlobalTransTopo transtopo;
};

class LayerwiseTimeWeightedTopoOptConfThinker
    : public LayerwiseTimeWeightedConfThinker {
 public:
  std::shared_ptr<Extra> obtainExtra(ConstelController* controller) override;

 private:
  virtual GlobalModelSyncConf decideModelSyncConf(
      const StrategyRequest& req) override;
};

}  // namespace constellation
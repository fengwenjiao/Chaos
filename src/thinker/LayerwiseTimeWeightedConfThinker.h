#pragma once

#ifndef CONS_NETWORK_AWARE
#error "Use this thinker should enable network aware"
#endif

#include "./SimpleThinker.h"

namespace constellation {

class LayerwiseTimeWeightedConfThinker : public ConstelSimpleThinker {
 private:
  virtual GlobalModelSyncConf decideModelSyncConf(
      const StrategyRequest& req) override;
};

}  // namespace constellation
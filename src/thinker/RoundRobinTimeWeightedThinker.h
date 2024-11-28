#pragma once

#include "./SimpleThinker.h"

namespace constellation {

class RoundRobinTimeWeightedThinker : public ConstelSimpleThinker {
 private:
  virtual GlobalModelSyncConf deciedModelSyncConf(const StrategyRequest& req) override;
};

}  // namespace constellation
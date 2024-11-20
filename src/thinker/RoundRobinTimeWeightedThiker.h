#pragma once

#include "./SimpleThinker.h"

namespace constellation {

class RoundRobinTimeWeightedThiker : public ConstelSimpleThinker {
 private:
  virtual GlobalModelSyncConf deciedModelSyncConf(const StrategyRequest& req) override;
}

}  // namespace constellation
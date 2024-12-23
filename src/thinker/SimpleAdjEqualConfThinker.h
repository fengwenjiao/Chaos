#pragma once
#include "./SimpleThinker.h"

namespace constellation {
class SimpleAdjEqualConfThinker : public ConstelSimpleThinker {
 private:
  virtual GlobalModelSyncConf decideModelSyncConf(const StrategyRequest& req) override;
};
}  // namespace constellation

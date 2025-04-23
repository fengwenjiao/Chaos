#pragma once

#include "SimpleThinker.h"

namespace constellation {
class SinglePointConfThinker : public ConstelSimpleThinker {
 private:
  virtual GlobalModelSyncConf decideModelSyncConf(
      const StrategyRequest& req) override;
};

}  // namespace constellation
#pragma once

#include "SimpleThinker.h"

namespace constellation {
class FAPTEqualConfThinker : public ConstelSimpleThinker {
 private:
  virtual GlobalModelSyncConf deciedModelSyncConf(const StrategyRequest& req) override;
};

}  // namespace constellation
#pragma once

#include "constellation_thinker.h"

namespace constellation {

class ConstelSimpleThinker : public ConstelThinker {
 protected:
  /**
   * \brief Transform the ModelLoadAssignment to GlobalModelSyncConf
   */
  virtual GlobalModelSyncConf ModelSycnConfTransform(int target_id,
                                                     ModelLoadAssignment& model_load_assignment);

 private:
  virtual StrategyBlock GenerateStrategyImpl(const StrategyRequest& req) override;

  virtual GlobalTransTopo decideNewTransTopo(const StrategyRequest& req);

  virtual void setRankAndNum(GlobalTransTopo& transtopo);

  virtual GlobalModelSyncConf decideModelSyncConf(const StrategyRequest& req);
};

}  // namespace constellation
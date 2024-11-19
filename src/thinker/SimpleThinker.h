#pragma once

#include "constellation_thinker.h"

namespace constellation {

class ConstelTransTopoThinker : public ConstelThinker {
 public:
 protected:
  /**
   * \brief Transform the ModelLoadAssignment to GlobalModelSyncConf
   */
  virtual GlobalModelSyncConf ModelSycnConfTransform(int target_id,
                                                     ModelLoadAssignment& model_load_assignment);

 private:
  virtual StrategyBlock GenerateStrategyImpl(const StrategyRequest& req) override;

  virtual GlobalTransTopo decideNewTransTopo(const AdjacencyList& overlay, int);

  void deciedModelSyncConf(const AdjacencyList& overlay, ModelSycnConf& model_sync_conf);
};

}  // namespace constellation
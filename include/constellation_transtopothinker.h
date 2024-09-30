#ifndef _CONSTELLATION_TRANSTOPOTHINKER_H_
#define _CONSTELLATION_TRANSTOPOTHINKER_H_

#include "constellation_commons.h"

#include <memory>

namespace constellation {

struct ModelLoadAssignment {
  using PathLoadMap =
      std::unordered_map<TransPath, float, TransPath::PathHash, TransPath::PathEqual>;

  bool assignLoad(const TransPath& path, const float& load) {
    if (load_assignment.find(path) != load_assignment.end()) {
      return false;
    }
    load_assignment[path] = load;
    return true;
  }
  PathLoadMap load_assignment;
};

struct Extra {
  virtual ~Extra() = default;
};

struct StrategyRequest {
  enum class StrategyReqType : uint8_t {
    kTopoUpdateOnly,
    kTopoAndModelSyncConfUpdate,
  };
  StrategyReqType type;
  std::vector<int> targets;
  AdjacencyList overlay;
  std::shared_ptr<Extra> extra;
};

struct StrategyBlock {
  GlobalTransTopo global_topo_;
  ModelLoadAssignment model_load_assignment_;
};

class ConstelThinker {
 public:
  const StrategyBlock& GenerateStrategy(const StrategyRequest& req);
  virtual ~ConstelThinker() = default;

 protected:
  void checkStrategy(const StrategyRequest& req, const StrategyBlock& strategy_block);

  StrategyBlock strategy_block_;
  GlobalTransTopo& global_topo_ = strategy_block_.global_topo_;
  ModelLoadAssignment& model_load_assignment_ = strategy_block_.model_load_assignment_;

 private:
  virtual StrategyBlock GenerateStrategyImpl(const StrategyRequest& req) = 0;
};

class ConstelTransTopoThinker : public ConstelThinker {
 public:
  const GlobalTransTopo& SendOverlay(const AdjacencyList& overlay, ModelSycnConf& model_sync_conf);

 private:
  StrategyBlock GenerateStrategyImpl(const StrategyRequest& req) override;

  GlobalTransTopo decideNewTransTopo(const AdjacencyList& overlay, int);

  void deciedModelSyncConf(const AdjacencyList& overlay, ModelSycnConf& model_sync_conf);
};

}  // namespace constellation
#endif  // _CONSTELLATION_TRANSTOPOTHINKER_H_
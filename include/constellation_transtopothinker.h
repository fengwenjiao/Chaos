#ifndef _CONSTELLATION_TRANSTOPOTHINKER_H_
#define _CONSTELLATION_TRANSTOPOTHINKER_H_

#include "constellation_commons.h"

#include <memory>
#include <algorithm>

namespace constellation {

struct ModelLoadAssignment {
  bool assignLoad(const TransPath& path, const float& load) {
    if (std::find(paths.begin(), paths.end(), path) != paths.end()) {
      return false;
    }
    paths.emplace_back(path);
    loads.emplace_back(load);
    return true;
  }
  void normalize() {
    float sum = 0;
    for (const auto& load : loads) {
      sum += load;
    }
    for (auto& load : loads) {
      load /= sum;
    }
  }
  /* @brief group the paths and loads by the first node in paths.
    * Change the order of paths and loads, which make the paths and loads 
    * that have the same first node in paths are together.  */
  void groupByFirstNode() {
    std::vector<TransPath> new_paths;
    std::vector<float> new_loads;
    std::unordered_map<int, std::vector<size_t>> indices;
    for (size_t i = 0; i < paths.size(); i++) {
      indices[paths[i].path[0]].emplace_back(i);
    }
    for (const auto& index : indices) {
      for (const auto& i : index.second) {
        new_paths.emplace_back(paths[i]);
        new_loads.emplace_back(loads[i]);
      }
    }
    paths = std::move(new_paths);
    loads = std::move(new_loads);
  }
  const std::vector<int>& getPath(size_t i) const {
    return paths[i].path;
  }
  std::vector<TransPath> paths;
  std::vector<float> loads;
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
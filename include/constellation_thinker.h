#ifndef _CONSTELLATION_TRANSTOPOTHINKER_H_
#define _CONSTELLATION_TRANSTOPOTHINKER_H_

#include "constellation_commons.h"

namespace constellation {

class ReadyoverlayInfo;

struct ModelLoadAssignment {
  bool assignLoad(const TransPath& path, const float& load);
  void normalize();

  /* @brief group the paths and loads by the first node in paths.
   * Change the order of paths and loads, which make the paths and loads
   * that have the same first node in paths are together.  */
  void groupByFirstNode();

  inline const std::vector<int>& getPath(size_t i) const {
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
  std::unique_ptr<ReadyoverlayInfo> overlay;
  std::shared_ptr<Extra> extra;
};

struct StrategyBlock {
  GlobalTransTopo global_topo_;
  GlobalModelSyncConf global_model_sync_conf_;
};

class ConstelThinker {
 public:
  void setParamsDist(int key, uint64_t value);
  uint64_t getParamsSize(int key) const;
  uint64_t getParamsTotal() const;

  const StrategyBlock& GenerateStrategy(const StrategyRequest& req);
  virtual ~ConstelThinker() = default;

 protected:
  virtual void checkStrategy(const StrategyRequest& req, const StrategyBlock& strategy_block);

  StrategyBlock strategy_block_;
  GlobalTransTopo& glb_topo_ = strategy_block_.global_topo_;
  GlobalModelSyncConf& glb_conf_ = strategy_block_.global_model_sync_conf_;

 private:
  virtual StrategyBlock GenerateStrategyImpl(const StrategyRequest& req) = 0;

  std::unordered_map<int, uint64_t> model_params_dist_;
  uint64_t model_params_total_;
};

}  // namespace constellation
#endif  // _CONSTELLATION_TRANSTOPOTHINKER_H_
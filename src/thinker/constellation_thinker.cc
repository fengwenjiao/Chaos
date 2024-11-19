#include "constellation_thinker.h"

#include "../overlay/node_overlay_manager.h"
#include "../algorithm/basic.h"

namespace constellation {

bool ModelLoadAssignment::assignLoad(const TransPath& path, const float& load) {
  if (std::find(paths.begin(), paths.end(), path) != paths.end()) {
    return false;
  }
  paths.emplace_back(path);
  loads.emplace_back(load);
  return true;
}

void ModelLoadAssignment::normalize() {
  float sum = 0;
  for (const auto& load : loads) {
    sum += load;
  }
  for (auto& load : loads) {
    load /= sum;
  }
}

void ModelLoadAssignment::groupByFirstNode() {
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

void ConstelThinker::checkStrategy(const StrategyRequest& req,
                                   const StrategyBlock& strategy_block) {
  const auto& transtopo = strategy_block.global_topo_;
  // const auto& model_load_assignment = strategy_block.model_load_assignment_;
  // // TODO: check the strategy
  // const int& target = req.targets[0];
  // const auto& overlay = req.overlay->GetReadyOverlay();
  // for (size_t i = 0; i < model_load_assignment.paths.size(); i++) {
  //   const auto& path = model_load_assignment.getPath(i);
  //   // check the source and target
  //   CHECK_GT(path.size(), 1);
  //   CHECK(overlay.find(*path.cbegin()) != overlay.end());
  //   CHECK_EQ(*path.crbegin(), target);
  //   // check the node is unique
  //   CHECK(algorithm::helper::checkUnique(path));
  // }
}

const StrategyBlock& ConstelThinker::GenerateStrategy(const StrategyRequest& req) {
  auto strategy_block = this->GenerateStrategyImpl(req);
  checkStrategy(req, strategy_block);
  this->strategy_block_ = std::move(strategy_block);
  return this->strategy_block_;
}

void ConstelThinker::setParamsDist(int key, uint64_t value) {
  model_params_dist_.emplace(key, value);
  model_params_total_ += value;
}
uint64_t ConstelThinker::getParamsSize(int key) const {
  return model_params_dist_.count(key) ? model_params_dist_.at(key) : 0;
}
uint64_t ConstelThinker::getParamsTotal() const {
  return model_params_total_;
}

}  // namespace constellation
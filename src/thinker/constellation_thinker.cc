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
  const auto& model_load_assignment = strategy_block.global_model_sync_conf_;
  // check the strategy
  const int& target = req.targets[0];
  const auto& overlay = req.overlay->GetReadyOverlay();
  // check the transtopo
  // check the overlay and transtopo are corresponding
  if (!algorithm::helper::areElementsUniqueAndCorresponding(overlay, transtopo)) {
    throw TranstopoInvalidError("Overlay and Transtopo are not corresponding");
  }
  int num = transtopo.size();      // the number of nodes in transtopo
  int root_id = 0;                 // the root node id
  std::vector<int> ranks(num, 0);  // the rank of each node

  for (const auto& [id, topo] : transtopo) {
    if (topo.getType() != NodeTransTopo::Type::kLeaf) {
      // check all children are in overlay
      for (const auto& child : topo.getChildren()) {
        if (overlay.find(child) == overlay.end()) {
          throw NodeNotFoundError("Child node not found in overlay", child);
        }
      }
    }
    if (topo.getType() != NodeTransTopo::Type::kRoot) {
      // for non-root
      int parent = topo.getParent();
      // check parent is in overlay
      if (overlay.find(parent) == overlay.end()) {
        throw NodeNotFoundError("Parent node not found in overlay", parent);
      }
      // check parent and id are connected
      if (std::find(overlay.at(parent).begin(), overlay.at(parent).end(), id) ==
          overlay.at(parent).end()) {
        throw NodeNotConnectedError("Two nodes in Transtopo are not connected", parent, id);
      }
      // check the node in parent's children
      if (!transtopo.at(parent).has_child(id)) {
        throw TransTopoNotConsistentError(std::to_string(parent) + " and " + std::to_string(id));
      }
    } else {
      // for root
      if (root_id) {
        throw TranstopoInvalidError("More than one root node " + std::to_string(root_id) + " and " +
                                    std::to_string(id));
      }
      root_id = id;
    }
    // check rank and num is set correctly
    if (topo.num_trainers != num) {
      throw TranstopoInvalidError("The number of trainers is not set correctly. The node " +
                                  std::to_string(id) + " has " + std::to_string(topo.num_trainers) +
                                  " trainers but expected " + std::to_string(num));
    }
    if (topo.rank >= num || topo.rank < 0) {
      throw TranstopoInvalidError("The rank is not set correctly. The node " + std::to_string(id) +
                                  " has rank " + std::to_string(topo.rank) +
                                  " but expected less than " + std::to_string(num));
    }
    if (ranks[topo.rank]) {
      throw TranstopoInvalidError("The rank is not unique. Node " + std::to_string(id) +
                                  " has rank " + std::to_string(topo.rank) + " but Node " +
                                  std::to_string(ranks[topo.rank]) + " has the same rank");
    }
  }

  // check the model load assignment
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
  if (model_params_dist_.count(key)) {
    return;
  }
  model_params_dist_.emplace(key, value);
  model_params_total_ += value;
}
uint64_t ConstelThinker::getParamsSize(int key) const {
  const auto it = this->model_params_dist_.find(key);
  return (it != model_params_dist_.end()) ? it->second : 0;
}
uint64_t ConstelThinker::getParamsTotal() const {
  return model_params_total_;
}

}  // namespace constellation
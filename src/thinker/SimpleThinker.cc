#include "./SimpleThinker.h"
#include "../overlay/node_overlay_manager.h"
#include "../algorithm/basic.h"

#include <functional>
#include <random>

namespace constellation {

void dfs(const AdjacencyList& graph,
         int current,
         int target,
         std::vector<int>& path,
         std::unordered_set<int>& visited,
         std::vector<TransPath>& allPaths) {
  path.push_back(current);
  visited.insert(current);

  if (current == target) {
    allPaths.emplace_back(path);
  } else {
    if (graph.find(current) != graph.end()) {
      for (const auto& neighbor : graph.at(current)) {
        if (visited.find(neighbor) == visited.end()) {
          dfs(graph, neighbor, target, path, visited, allPaths);
          break;
        }
      }
    }
  }

  path.pop_back();
  visited.erase(current);
};

std::vector<TransPath> generatePathsToTarget(const AdjacencyList& graph,
                                             int target,
                                             int maxPaths = 10) {
  std::vector<TransPath> allPaths;
  std::vector<TransPath> result;

  for (const auto& [node, _] : graph) {
    if (node == target)
      continue;
    std::vector<int> path;
    std::unordered_set<int> visited;
    dfs(graph, node, target, path, visited, allPaths);

    for (const auto& p : allPaths) {
      if (p.path.back() == target) {
        result.emplace_back(p);
        if (result.size() >= maxPaths) {
          return result;
        }
      }
    }
    allPaths.clear();
  }
  return result;
}

std::vector<TransPath> selectRandomPaths(const std::vector<TransPath>& paths,
                                         size_t numberOfPaths) {
  std::vector<TransPath> selectedPaths;

  if (paths.empty()) {
    return selectedPaths;
  }

  if (numberOfPaths >= paths.size()) {
    return paths;
  }

  std::random_device rd;
  std::mt19937 gen(rd());

  std::vector<size_t> indices(paths.size());
  for (size_t i = 0; i < indices.size(); ++i) {
    indices[i] = i;
  }
  std::shuffle(indices.begin(), indices.end(), gen);

  for (size_t i = 0; i < numberOfPaths; ++i) {
    selectedPaths.emplace_back(paths[indices[i]]);
  }

  return selectedPaths;
};

GlobalTransTopo ConstelTransTopoThinker::decideNewTransTopo(const AdjacencyList& overlay, int) {
  return algorithm::basic::random_choose_method(overlay);
}

void ConstelTransTopoThinker::deciedModelSyncConf(const AdjacencyList& overlay,
                                                  ModelSycnConf& model_sync_conf) {}

GlobalModelSyncConf ConstelTransTopoThinker::ModelSycnConfTransform(
    int target_id,
    ModelLoadAssignment& model_load_assignment) {
  GlobalModelSyncConf global_model_sync_conf;
  auto model_params_total_ = getParamsTotal();
  model_load_assignment.normalize();
  model_load_assignment.groupByFirstNode();
  int key = 0;
  uint64_t tot_len = 0;
  uint64_t cur_len = 0;
  int full_key_begin = -1;
  int full_key_end = -1;
  uint64_t piece;
  for (size_t i = 0; i < model_load_assignment.paths.size(); i++) {
    const auto& path = model_load_assignment.getPath(i);
    const int& node = path[0];
    const float& load = model_load_assignment.loads[i];
    uint64_t slice_len = (i < model_load_assignment.paths.size() - 1) ?
                             std::ceil(load * model_params_total_) :
                             model_params_total_ - tot_len;
    auto& model_sc_ = global_model_sync_conf[node];
    model_sc_.target_node_id.push_back(target_id);
    model_sc_.paths.emplace_back(path);
    model_sc_.kvslices.emplace_back();
    if (cur_len > 0) {
      piece = std::min(slice_len, getParamsSize(key) - cur_len);
      model_sc_.kvslices.back().emplace_back(key, cur_len, piece);
      cur_len += piece;
      cur_len %= getParamsSize(key);
      ;
      tot_len += piece;
      slice_len -= piece;
      if (cur_len == 0 && getParamsSize(++key) == 0) {
        CHECK_EQ(slice_len, 0);
        break;
      }
    }
    full_key_begin = key;
    auto size_ = getParamsSize(key);
    while (size_ > 0 && slice_len >= size_) {
      slice_len -= size_;
      tot_len += size_;
      size_ = getParamsSize(++key);
    }
    full_key_end = key;
    if (full_key_end > full_key_begin) {
      model_sc_.kvslices.back().emplace_back(full_key_begin, full_key_end);
    }
    if (slice_len > 0) {
      model_sc_.kvslices.back().emplace_back(key, 0, slice_len);
      cur_len = slice_len;
      tot_len += slice_len;
    }
  }

  return global_model_sync_conf;
}

// const GlobalTransTopo& ConstelTransTopoThinker::SendOverlay(const AdjacencyList& overlay,
//                                                             ModelSycnConf& model_sync_conf) {
//   GlobalTransTopo transtopo;
//   if (overlay.size() == 1) {
//     NodeTransTopo topo;
//     topo.setoRoot();
//     int node_id = overlay.begin()->first;
//     transtopo[node_id] = topo;
//   } else {
//     transtopo = decideNewTransTopo(overlay, 1);
//   }
//   this->global_topo_ = std::move(transtopo);
//   return this->global_topo_;
// }

StrategyBlock ConstelTransTopoThinker::GenerateStrategyImpl(const StrategyRequest& req) {
  using StrategyReqType = StrategyRequest::StrategyReqType;

  auto& overlay = req.overlay->GetReadyOverlay();
  auto& targets = req.targets;

  StrategyBlock strategy_block;
  auto& global_topo = strategy_block.global_topo_;
  auto& global_sync_conf = strategy_block.global_model_sync_conf_;

  ModelLoadAssignment model_load_assignment;
  std::vector<TransPath> allPaths;

  switch (req.type) {
    case StrategyReqType::kTopoAndModelSyncConfUpdate: {
      // TODO
      CHECK_EQ(targets.size(), 1);
      const auto target = targets[0];
      const auto it = overlay.find(target);
      CHECK(it != overlay.end());

      allPaths = generatePathsToTarget(overlay, target, 5);
      auto paths = selectRandomPaths(allPaths, 2);
      for (const auto& path : paths) {
        model_load_assignment.assignLoad(path, 1.0);
      }
      global_sync_conf = ModelSycnConfTransform(target, model_load_assignment);
    }
    case StrategyReqType::kTopoUpdateOnly: {
      GlobalTransTopo transtopo;
      if (overlay.size() == 1) {
        NodeTransTopo topo;
        topo.setoRoot();
        int node_id = overlay.begin()->first;
        transtopo[node_id] = topo;
      } else {
        transtopo = decideNewTransTopo(overlay, 1);
      }
      global_topo = std::move(transtopo);
      break;
    }

    default:
      break;
  }
  return strategy_block;
}

}  // namespace constellation
#include "ConstelTransTopoThinker.h"
#include "../overlay/node_overlay_manager.h"
#include "../algorithm/basic.h"

#include <functional>
#include <random>

namespace constellation {

GlobalTransTopo ConstelTransTopoThinker::decideNewTransTopo(const AdjacencyList& overlay, int) {
  return algorithm::basic::random_choose_method(overlay);
}

void ConstelTransTopoThinker::deciedModelSyncConf(const AdjacencyList& overlay,
                                                  ModelSycnConf& model_sync_conf) {}

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
  auto& model_load_assignment = strategy_block.model_load_assignment_;

  std::vector<TransPath> allPaths;
  std::function<void(const AdjacencyList&,
                     int,
                     int,
                     std::vector<int>&,
                     std::unordered_set<int>&,
                     std::vector<TransPath>& allPaths)>
      dfs;
  dfs = [&dfs](const AdjacencyList& graph,
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
  auto generatePathsToTarget =
      [&dfs](const AdjacencyList& graph, int target, int maxPaths = 10) -> std::vector<TransPath> {
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
  };
  auto selectRandomPaths = [](const std::vector<TransPath>& paths,
                              size_t numberOfPaths) -> std::vector<TransPath> {
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
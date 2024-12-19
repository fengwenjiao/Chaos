#include "./SinglePointConfThinker.h"
#include "../overlay/node_overlay_manager.h"
#include "../algorithm/basic.h"

namespace constellation {

GlobalModelSyncConf SinglePointConfThinker::decideModelSyncConf(const StrategyRequest& req) {
  auto& overlay = req.overlay->GetReadyOverlay();
  auto& targets = req.targets;
  const auto target = targets[0];

  ModelLoadAssignment model_load_assignment;
  std::vector<TransPath> allPaths;
  using namespace algorithm;
  allPaths = basic::random_choose_paths(overlay, target, 10);
  auto paths = helper::randomChoose(allPaths, 1);
  model_load_assignment.assignLoad(paths[0], 1.0);
  // auto node1 = overlay.begin()->first;
  // auto node2 = overlay.begin()->second[0];
  // int node;
  // if (node1 == target) {
  //   node = node2;
  // }else{
  //   node = node1;
  // }
  // model_load_assignment.assignLoad({node,target}, 1.0);

  return ModelSycnConfTransform(target, model_load_assignment);
}

}  // namespace constellation
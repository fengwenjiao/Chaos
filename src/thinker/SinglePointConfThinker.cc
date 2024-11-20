#include "./SinglePointConfThinker.h"
#include "../overlay/node_overlay_manager.h"
#include "../algorithm/basic.h"

namespace constellation {

GlobalModelSyncConf SinglePointConfThinker::deciedModelSyncConf(const StrategyRequest& req) {
  auto& overlay = req.overlay->GetReadyOverlay();
  auto& targets = req.targets;
  const auto target = targets[0];

  ModelLoadAssignment model_load_assignment;
  std::vector<TransPath> allPaths;
  using namespace algorithm;
  allPaths = basic::random_choose_paths(overlay, target, 5);
  auto paths = helper::randomChoose(allPaths, 1);
  for (const auto& path : paths) {
    model_load_assignment.assignLoad(path, 1.0);
  }
  return ModelSycnConfTransform(target, model_load_assignment);
}

}  // namespace constellation
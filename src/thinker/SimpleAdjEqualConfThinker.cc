#include "SimpleAdjEqualConfThinker.h"
#include "../overlay/node_overlay_manager.h"

namespace constellation {

GlobalModelSyncConf SimpleAdjEqualConfThinker::decideModelSyncConf(const StrategyRequest& req) {
  auto& overlay = req.overlay->GetReadyOverlay();
  auto& targets = req.targets;
  const auto target = targets[0];

  ModelLoadAssignment model_load_assignment;
  auto& neighbors = overlay.at(target);
  for (const auto& neighbor : neighbors) {
    TransPath path{neighbor, target};
    model_load_assignment.assignLoad(path, 1);
  }
  return ModelSycnConfTransform(target, model_load_assignment);
}

}  // namespace constellation

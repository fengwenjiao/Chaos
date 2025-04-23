#ifdef CONS_NETWORK_AWARE

#include "./SimpleAdjTimeWeightedConfThinker.h"
#include "../overlay/network_aware/network_aware.h"

namespace constellation {
GlobalModelSyncConf SimpleAdjTimeWeightedConfThinker::decideModelSyncConf(
    const StrategyRequest& req) {
  auto* overlay_info =
      dynamic_cast<aware::NetAWoverlayInfo*>(req.overlay.get());
  if (overlay_info == nullptr) {
    throw std::runtime_error(
        "FAPTEqualConfThinker only support NetworkAwareOverlay. Please enable "
        "network aware.");
  }
  auto& overlay = overlay_info->GetReadyOverlay();
  auto& targets = req.targets;
  const auto target = targets[0];

  ModelLoadAssignment model_load_assignment;
  auto& neighbors = overlay.at(target);
  for (const auto& neighbor : neighbors) {
    TransPath path{neighbor, target};
    float bw = overlay_info->get_edge_property(topo::Edge{neighbor, target});
    model_load_assignment.assignLoad(path, bw);
  }
  return ModelSycnConfTransform(target, model_load_assignment);
}

}  // namespace constellation

#endif
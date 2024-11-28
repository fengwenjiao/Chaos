#ifdef CONS_NETWORK_AWARE

#include "SimpleEqualConfThinker.h"
#include "../overlay/network_aware/network_aware.h"

namespace constellation {

GlobalModelSyncConf SimpleEqualConfThinker::deciedModelSyncConf(const StrategyRequest& req) {
  auto* overlay_info = dynamic_cast<aware::NetAWoverlayInfo*>(req.overlay.get());
  if (overlay_info == nullptr) {
    throw std::runtime_error(
        "SimpleEqualConfThinker only support NetworkAwareOverlay. Please enable network aware.");
  }
  auto& overlay = overlay_info->GetReadyOverlay();
  auto& targets = req.targets;
  const auto target = targets[0];

  ModelLoadAssignment model_load_assignment;
  auto& neighbors = overlay.at(target);
  for (const auto& neighbor : neighbors) {
    TransPath path{neighbor, target};
    auto bw = overlay_info->get_edge_property(topo::Edge{neighbor, target});
    LOG(INFO) << "Assign path: " << path.debug_string() << " with bandwidth: " << bw;
    model_load_assignment.assignLoad(path, bw);
  }
  return ModelSycnConfTransform(target, model_load_assignment);
}

}  // namespace constellation
#endif
#ifdef CONS_NETWORK_AWARE

#include "./FAPTEqualConfThinker.h"
#include "../algorithm/basic.h"

#include "../overlay/network_aware/network_aware.h"

namespace constellation {

GlobalModelSyncConf FAPTEqualConfThinker::decideModelSyncConf(const StrategyRequest& req) {
  auto* overlay_info = dynamic_cast<aware::NetAWoverlayInfo*>(req.overlay.get());
  if (overlay_info == nullptr) {
    throw std::runtime_error(
        "FAPTEqualConfThinker only support NetworkAwareOverlay. Please enable network aware.");
  }
  auto& overlay = overlay_info->GetReadyOverlay();
  auto& targets = req.targets;
  const auto target = targets[0];

  ModelLoadAssignment model_load_assignment;

  AdjacencyListT<float> weights;
  for (const auto& [src, neighbors] : overlay) {
    for (size_t i = 0; i < neighbors.size(); i++) {
      weights[src][neighbors[i]] =
          1 / overlay_info->get_edge_property(topo::Edge{src, neighbors[i]});
    }
  }
  using namespace constellation::algorithm::basic;
  auto paths = dijsktra_paths(overlay, weights, target);
  for (const auto& path : paths) {
    model_load_assignment.assignLoad(path, 1.0);
  }
  return ModelSycnConfTransform(target, model_load_assignment);
}

}  // namespace constellation

#endif
#ifdef CONS_NETWORK_AWARE

#include "./LayerwiseTimeWeightedConfThinker.h"
#include "../overlay/network_aware/network_aware.h"
#include "../algorithm/algorithm_helper.h"

namespace constellation {

std::vector<KVSlice> slice(int key,
                           uint64_t key_total,
                           std::vector<float> weights) {
  std::vector<KVSlice> slices(weights.size());
  float sum = 0;
  for (size_t i = 0; i < weights.size(); i++) {
    sum += weights[i];
  }
  using algorithm::helper::spilitRange;
  auto rngs = spilitRange(
      static_cast<uint64_t>(0), key_total, weights.size(), weights.data());
  for (size_t i = 0; i < weights.size(); i++) {
    slices[i] = KVSlice(key, rngs[i].first, rngs[i].second - rngs[i].first);
  }
  return slices;
}

GlobalModelSyncConf LayerwiseTimeWeightedConfThinker::decideModelSyncConf(
    const StrategyRequest& req) {
  auto* overlay_info =
      dynamic_cast<aware::NetAWoverlayInfo*>(req.overlay.get());
  if (overlay_info == nullptr) {
    throw std::runtime_error(
        "LayerwiseTimeWeightedConfThinker only support NetworkAwareOverlay. "
        "Please enable network "
        "aware.");
  }
  auto& overlay = overlay_info->GetReadyOverlay();
  auto& targets = req.targets;
  const auto target = targets[0];
  GlobalModelSyncConf ret;
  auto& neighbors = overlay.at(target);
  std::vector<float> bws;
  bws.reserve(neighbors.size());
  for (auto& neighbor : neighbors) {
    ret[neighbor].target_node_id.emplace_back(target);
    ret[neighbor].paths.emplace_back(std::vector<int>{neighbor, target});
    ret[neighbor].kvslices.emplace_back();
    float bw = overlay_info->get_edge_property(topo::Edge{neighbor, target});
    if (bw <= 0) {
      LOG(WARNING) << "path: " << neighbor << " -> " << target
                   << " has invalid bandwidth: " << bw << ". Set to 1.";
      bw = 1;
    }
    bws.emplace_back(bw);
  }
  int key = 0;
  while (true) {
    auto size = getParamsSize(key);
    if (size == 0) {
      break;
    }
    auto slices = slice(key, size, bws);
    for (size_t i = 0; i < neighbors.size(); i++) {
      auto& kvslice = ret[neighbors[i]].kvslices.back();
      kvslice.emplace_back(slices[i]);
    }
    key++;
  }
  return ret;
}

}  // namespace constellation

#endif
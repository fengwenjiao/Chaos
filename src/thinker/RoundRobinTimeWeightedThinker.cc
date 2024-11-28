#ifdef CONS_NETWORK_AWARE

#include "RoundRobinTimeWeightedThinker.h"
#include "../overlay/network_aware/network_aware.h"

namespace constellation {

template <typename T>
size_t chooseMinIndex(const std::vector<T>& vec) {
  size_t min_index = 0;
  for (size_t i = 1; i < vec.size(); i++) {
    if (vec[i] < vec[min_index]) {
      min_index = i;
    }
  }
  return min_index;
}

GlobalModelSyncConf RoundRobinTimeWeightedThinker::deciedModelSyncConf(const StrategyRequest& req) {
  auto* overlay_info = dynamic_cast<aware::NetAWoverlayInfo*>(req.overlay.get());
  if (overlay_info == nullptr) {
    throw std::runtime_error(
        "RoundRobinTimeWeightedThinker only support NetworkAwareOverlay. Please enable network "
        "aware.");
  }
  auto& overlay = overlay_info->GetReadyOverlay();
  auto& targets = req.targets;
  const auto target = targets[0];

  GlobalModelSyncConf ret;
  auto& neighbors = overlay.at(target);
  std::vector<float> bws;
  std::vector<uint64_t> loads;
  bws.reserve(neighbors.size());
  loads.reserve(neighbors.size());
  for (auto& neighbor : neighbors) {
    ret[neighbor].target_node_id.emplace_back(neighbor);
    ret[neighbor].paths.emplace_back(std::vector<int>{neighbor, target});
    ret[neighbor].kvslices.emplace_back();
    float bw = overlay_info->get_edge_property(topo::Edge{neighbor, target});
    if (bw <= 0) {
      LOG(WARNING) << "path: " << neighbor << " -> " << target << " has invalid bandwidth: " << bw
                   << ". Set to 1.";
      bw = 1;
    }
    bws.emplace_back(bw);
    loads.emplace_back(-1 * bw);
  }
  int key = 0;
  while (true) {
    auto size = getParamsSize(key);
    if (size == 0) {
      break;
    }
    auto index = chooseMinIndex(loads);
    auto& kvslice = ret[neighbors[index]].kvslices.back();
    kvslice.emplace_back(key, ++key);
    // update bws
    if (loads[index] < 0) {
      loads[index] = size / bws[index];
    } else {
      loads[index] += size / bws[index];
    }
  }
  return ret;
}

}  // namespace constellation

#endif
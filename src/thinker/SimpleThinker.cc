#include "./SimpleThinker.h"
#include "../overlay/node_overlay_manager.h"
#include "../algorithm/basic.h"

#include <functional>
#include <random>

namespace constellation {

GlobalTransTopo ConstelSimpleThinker::decideNewTransTopo(const StrategyRequest& req) {
  auto& overlay = req.overlay->GetReadyOverlay();
  return algorithm::basic::random_choose_tree(overlay);
}

GlobalModelSyncConf ConstelSimpleThinker::deciedModelSyncConf(const StrategyRequest& req) {
  auto& overlay = req.overlay->GetReadyOverlay();
  auto& targets = req.targets;

  const auto target = targets[0];
  ModelLoadAssignment model_load_assignment;
  std::vector<TransPath> allPaths;
  using namespace algorithm;
  allPaths = basic::random_choose_paths(overlay, target, 5);
  auto paths = helper::randomChoose(allPaths, 2);
  for (const auto& path : paths) {
    model_load_assignment.assignLoad(path, 1.0);
  }
  return ModelSycnConfTransform(target, model_load_assignment);
}

GlobalModelSyncConf ConstelSimpleThinker::ModelSycnConfTransform(
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

StrategyBlock ConstelSimpleThinker::GenerateStrategyImpl(const StrategyRequest& req) {
  using StrategyReqType = StrategyRequest::StrategyReqType;

  auto& overlay = req.overlay->GetReadyOverlay();
  auto& targets = req.targets;

  StrategyBlock strategy_block;
  auto& global_topo = strategy_block.global_topo_;
  auto& global_sync_conf = strategy_block.global_model_sync_conf_;

  switch (req.type) {
    case StrategyReqType::kTopoAndModelSyncConfUpdate: {
      global_sync_conf = deciedModelSyncConf(req);
    }
    case StrategyReqType::kTopoUpdateOnly: {
      if (overlay.size() == 1) {
        GlobalTransTopo transtopo;
        NodeTransTopo topo;
        topo.setoRoot();
        int node_id = overlay.begin()->first;
        transtopo.emplace(node_id, topo);
        global_topo = std::move(transtopo);
      } else {
        CHECK_EQ(targets.size(), 1);
        const auto it = overlay.find(targets[0]);
        CHECK(it != overlay.end());
        global_topo = decideNewTransTopo(req);
      }
      break;
    }

    default:
      break;
  }
  return strategy_block;
}

}  // namespace constellation
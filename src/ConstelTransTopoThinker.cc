#include "constellation_transtopothinker.h"

#include "algorithm/basic.h"

namespace constellation {

void ConstelThinker::checkStrategy(const StrategyRequest& req,const StrategyBlock& strategy_block) {
  const auto &transtopo = global_topo_;
  const auto &model_load_assignment = model_load_assignment_;
  // TODO: check the strategy
  const int & target = req.targets[0];
  const auto & overlay = req.overlay;
  for (const auto& [path, load] : model_load_assignment.load_assignment) {
    // check the source and target
    CHECK_GT(path.path.size(), 1);
    CHECK(overlay.find(*path.path.cbegin()) != overlay.end());
    CHECK_EQ(*path.path.crbegin(), target);
    // check the node is unique
    CHECK(algorithm::helper::checkUnique(path.path));
  }
}

const StrategyBlock& ConstelThinker::GenerateStrategy(const StrategyRequest& req){
  auto strategy_block = GenerateStrategyImpl(req);
  checkStrategy(req,strategy_block);
  this->strategy_block_ = std::move(strategy_block);
  return this->strategy_block_;
}

GlobalTransTopo ConstelTransTopoThinker::decideNewTransTopo(const AdjacencyList& overlay, int) {
  return algorithm::basic::random_choose_method(overlay);
}

void ConstelTransTopoThinker::deciedModelSyncConf(const AdjacencyList& overlay,
                                                  ModelSycnConf& model_sync_conf) {}

const GlobalTransTopo& ConstelTransTopoThinker::SendOverlay(const AdjacencyList& overlay,
                                                            ModelSycnConf& model_sync_conf) {
  GlobalTransTopo transtopo;
  if (overlay.size() == 1) {
    NodeTransTopo topo;
    topo.setoRoot();
    int node_id = overlay.begin()->first;
    transtopo[node_id] = topo;
  } else {
    transtopo = decideNewTransTopo(overlay, 1);
  }
  this->global_topo_ = std::move(transtopo);
  return this->global_topo_;
}

StrategyBlock ConstelTransTopoThinker::GenerateStrategyImpl(const StrategyRequest& req) {
  using StrategyReqType = StrategyRequest::StrategyReqType;

  const auto& overlay = req.overlay;
  const auto& targets = req.targets;

  StrategyBlock strategy_block;
  auto &global_topo = strategy_block.global_topo_;
  auto &model_load_assignment = strategy_block.model_load_assignment_;

  switch (req.type) {
    case StrategyReqType::kTopoAndModelSyncConfUpdate: {
      //TODO
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
    }

    default:
      break;
  }

  return strategy_block;
}

}  // namespace constellation
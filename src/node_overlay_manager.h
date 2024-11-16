#pragma once
#include "topo_graph.hpp"
#include "constellation_commons.h"

#include <memory>

namespace constellation {

class ReadyoverlayInfo {
 public:
  ReadyoverlayInfo() = default;
  explicit ReadyoverlayInfo(AdjacencyList overlay) : overlay_(overlay) {}
  virtual ~ReadyoverlayInfo() = default;
  inline const AdjacencyList& GetReadyOverlay() {
    return overlay_;
  }

 private:
  AdjacencyList overlay_;
};

class ReadyNodeOverlayManager {
 public:
  ReadyNodeOverlayManager() : is_asycn_add_(false), is_first_reach_init_num_{false} {}
  virtual bool HandleNodeReady(int node_id);
  inline bool ShouldGetNewTransTopo() const {
    return is_asycn_add_;
  }
  inline bool isFristReachInitNum() const {
    return is_first_reach_init_num_;
  }
  virtual std::unique_ptr<ReadyoverlayInfo> GetReadyOverlay();
  virtual ~ReadyNodeOverlayManager() = default;

 private:
  topo::TopoGraph<topo::Edge<int> > ready_nodes_;

  bool is_asycn_add_;  // 0: sync join stage, 1: async join stage
  bool is_first_reach_init_num_;
};

}  // namespace constellation
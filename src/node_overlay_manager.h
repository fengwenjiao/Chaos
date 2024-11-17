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

 protected:
  AdjacencyList overlay_;
};

class NodeManagerBase {
 public:
  NodeManagerBase() : is_asycn_add_(false), is_first_reach_init_num_{false} {}
  inline bool ShouldGetNewTransTopo() const {
    return is_asycn_add_;
  }
  inline bool isFristReachInitNum() const {
    return is_first_reach_init_num_;
  }
  virtual bool HandleNodeReady(int node_id) = 0;
  virtual std::unique_ptr<ReadyoverlayInfo> GetReadyOverlay() = 0;
  virtual ~NodeManagerBase() = default;

 protected:
  bool is_asycn_add_;  // 0: sync join stage, 1: async join stage
  bool is_first_reach_init_num_;
};

class ReadyNodeOverlayManager : public NodeManagerBase {
 public:
  ReadyNodeOverlayManager() : NodeManagerBase() {}
  virtual bool HandleNodeReady(int node_id) override;

  virtual std::unique_ptr<ReadyoverlayInfo> GetReadyOverlay() override;

 private:
  topo::TopoGraph<topo::Edge<int> > ready_nodes_;
};

}  // namespace constellation
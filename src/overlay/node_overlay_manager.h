#pragma once
#include "constellation_commons.h"

#include "topo_graph.hpp"

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
  virtual std::string debug_string() const {
    std::string str;
    str += "{";
    for (const auto& edge : overlay_) {
      str += std::to_string(edge.first) + " -> [";
      for (const auto& dst : edge.second) {
        str += std::to_string(dst) + " ";
      }
      str += "] ";
    }
    str += "}";
    return str;
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
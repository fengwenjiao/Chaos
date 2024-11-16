#pragma once
#include "topo_graph.hpp"
#include "clusterRM/smq.h"
#include "node_overlay_manager.h"

namespace constellation {
namespace aware {

struct WorkerNode {
  int id;
  struct GpuInfo {
    std::string gpu_name;
    int gpu_id;
    float gpu_memory;
    float gpu_utilization;
  };
  std::vector<GpuInfo> gpu_info;

  std::vector<std::string> cpu_model_name;
  float memory;
  float cpu_utilization;
  float memory_utilization;

  // hash function
  struct Hash {
    std::size_t operator()(const WorkerNode& node) const {
      return std::hash<int>()(node.id);
    }
  };
};

struct LinkProperty {
  float bandwidth;
};

using AwareEdge = topo::Edge<int, LinkProperty>;

class NetworkAwareNodeManager : public ReadyNodeOverlayManager {
 public:
  virtual bool HandleNodeReady(int node_id) override {
    return true;
  }

 private:
  topo::TopoGraph<AwareEdge> ready_nodes_;
};

}  // namespace aware
}  // namespace constellation

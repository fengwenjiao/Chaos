#pragma once

#include "ps/ps.h"

#include "../topo_graph.hpp"
#include "clusterRM/smq.h"
#include "../node_overlay_manager.h"

#include <thread>

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

class NetAWoverlayInfo : public ReadyoverlayInfo {
 public:
  NetAWoverlayInfo() = default;
  explicit NetAWoverlayInfo(AdjacencyList overlay) : ReadyoverlayInfo(overlay) {}
  inline float& get_edge_property(const topo::Edge<int>& edge) {
    return edge_property_[edge];
  }

 private:
  std::unordered_map<topo::Edge<int>, float, topo::Edge<int>::Hash> edge_property_;
};

class NetworkAwareNodeManager : public ReadyNodeOverlayManager {
 public:
  NetworkAwareNodeManager(std::unique_ptr<moniter::Smq>& test_server) : ReadyNodeOverlayManager() {
    test_server_ = test_server.get();
    test_server_thread_.reset(new std::thread([this] { test_server_->start_server(); }));
  }
  virtual ~NetworkAwareNodeManager() override {
    test_server_->stop();
    test_server_thread_->join();
  }

  virtual std::unique_ptr<ReadyoverlayInfo> GetReadyOverlay() override {
    using namespace moniter;
    auto overlay = ReadyNodeOverlayManager::GetReadyOverlay();
    auto overlayinfo = std::make_unique<NetAWoverlayInfo>(overlay->GetReadyOverlay());
    auto& infos = test_server_->gather_info(kSignalStatic + kSignalDynamic + kSignalBandwidth);
    for (auto& info : infos) {
      auto res = test_server_->convert_to_meta(info);
      auto id = res.id;
      auto& bandwith = res.network_info.bandwidth;
      for (const auto& [ip, bw] : bandwith) {
        auto& target_ids = ps::Postoffice::Get()->GetIp2Nodes().at(ip);
        for (auto target_id : target_ids) {
          overlayinfo->get_edge_property(topo::Edge{id, target_id}) = bw;
        }
      }
    }
    return overlayinfo;
  }

 private:
  moniter::Smq* test_server_;
  std::unique_ptr<std::thread> test_server_thread_;
};

}  // namespace aware
}  // namespace constellation

#ifndef _CONSTELLATION_CONTROLLER_H_
#define _CONSTELLATION_CONTROLLER_H_

#include <functional>
#include "./constellation_commons.h"
#include "./constellation_transtopothinker.h"
#include "internal/topo_graph.hpp"
#include <ps/ps.h>

namespace constellation {

class ReadyNodeOverlayManager {
 public:
  ReadyNodeOverlayManager(): is_asycn_add_(false) {}
  bool HandleNodeReady(int node_id) {
    auto& connected_nodes = ps::Postoffice::Get()->GetOverlayNeighbour(node_id);
    if (!ready_nodes_.AddNode(node_id)) {
      // the node is already in the ready_nodes_
      return false;
    }
    bool is_add_edge = false;
    for (auto& node : connected_nodes) {
      if (ready_nodes_.HasNode(node)) {
        // the nerghbour is ready, then add edge
        if (ready_nodes_.AddEdge(node_id, node)) {
          is_add_edge = true;
        }
      }
    }
    if(!is_add_edge && ready_nodes_.NumNodes() >=2) {
      LOG(WARNING) << "Node " << node_id << " is ready, but no edge is added";
    }
    // check if node number is enough
    if (ready_nodes_.NumNodes() == ps::Postoffice::Get()->init_num_trainers()) {
      is_asycn_add_ = true;
    }
    return true;
  }
  bool ShouldGetNewTransTopo() {
    return isAsyncJoinStage() ;
  }
  bool isAsyncJoinStage() {
    return is_asycn_add_;
  }

  //TODO: GetReadyOverlay() is debug version, should return the string of overlay
  AdjacencyList GetReadyOverlayStr() {
    auto& edges = ready_nodes_.GetEdges();
    AdjacencyList overlay;
    for (auto& edge : edges) {
      overlay[edge.src].push_back(edge.dst);
      overlay[edge.dst].push_back(edge.src);
    }
    if(overlay.empty()){
      auto& nodes = ready_nodes_.GetNodes();
      CHECK_EQ(nodes.size(), 1);
      auto& node = *nodes.begin();
      overlay[node] = std::vector<int>();
    }
    return overlay;
  }


 private:
  TopoGraph<int> ready_nodes_;
  bool is_asycn_add_ ;  // 0: sync join stage, 1: async join stage
};

class ConstelController {
 public:
  explicit ConstelController() {
    ps::StartAsync(0, "ConstelController\0");
    using namespace std::placeholders;
    ps_scheduler_ = new ps::Controller(0);
    ps_scheduler_->set_request_handle(std::bind(&ConstelController::RequestHandle, this, _1, _2));
    ps_scheduler_->set_response_handle(std::bind(&ConstelController::ResponseHandle, this, _1, _2));

    thinker_ = new ConstelTransTopoThinker();
  }
  ~ConstelController() {
    ps::Finalize(0, false);
    delete ps_scheduler_;
    delete thinker_;
  }

 private:
  /**
   * \brief Controller handle for all received request
   */
  void RequestHandle(const ps::SimpleData& recved, ps::SimpleApp* app);
  /**
   * \brief Controller handle for all received response
   */
  void ResponseHandle(const ps::SimpleData& recved, ps::SimpleApp* app);
  /**
   * \brief Controller handle for signal from scheduler
   */
  void SchedulerSignalHandle(const ps::SimpleData& recved, ps::SimpleApp* app);
  /**
   * \brief Controller return a proper future timsestamp
   */
  uint32_t GetFutureTimtestamp();
  /**
   * \brief send message to all trainers
   */
  void SendToALLTrainers(int head, const std::string& body);
  /**
   * \brief send message to some trainer
   */
  void SendToTrainer(int head, const std::string& body, int recv_id);
  /**
   * \brief serialze timestamp and transtopo into a string
   */
  std::string SerializeTransTopo(int timestamp, const std::pair<int, std::vector<int>>& data);
  /**
   * \brief deserialze  a string into timestamp and transtopo
   */
  bool DeserializeTransTopo(const std::string& serialized, int& timestamp, std::pair<int, std::vector<int>>& data);

  ReadyNodeOverlayManager node_manager_;

  ScaleClock clock_;
  ps::Controller* ps_scheduler_;
  ConstelTransTopoThinker * thinker_;

};

}  // namespace constellation
#endif  // _CONSTELLATION_CONTROLLER_H_
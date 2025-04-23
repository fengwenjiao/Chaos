#include "node_overlay_manager.h"
#include "ps/ps.h"

namespace constellation {

bool ReadyNodeOverlayManager::HandleNodeReady(int node_id) {
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
  if (!is_add_edge && ready_nodes_.NumNodes() >= 2) {
    LOG(WARNING) << "Node " << node_id << " is ready, but no edge is added";
  }
  if (is_asycn_add_) {
    is_first_reach_init_num_ = false;
    return true;
  }
  // check if node number is enough
  if (ready_nodes_.NumNodes() == ps::Postoffice::Get()->init_num_trainers()) {
    is_asycn_add_ = true;
    // first reach init num
    is_first_reach_init_num_ = true;
  }
  return true;
}

// TODO: GetReadyOverlay() is debug version, should return the string of overlay
std::unique_ptr<ReadyoverlayInfo> ReadyNodeOverlayManager::GetReadyOverlay() {
  auto& edges = ready_nodes_.GetEdges();
  AdjacencyList overlay;
  for (auto& edge : edges) {
    overlay[edge.src].push_back(edge.dst);
    overlay[edge.dst].push_back(edge.src);
  }
  if (overlay.empty()) {
    auto& nodes = ready_nodes_.GetNodes();
    CHECK_EQ(nodes.size(), 1);
    auto& node = *nodes.begin();
    overlay[node] = std::vector<int>();
  }
  return std::make_unique<ReadyoverlayInfo>(overlay);
}
}  // namespace constellation
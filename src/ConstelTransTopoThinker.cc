#include "constellation_transtopothinker.h"

namespace constellation {

GlobalTransTopo ConstelTransTopoThinker::decideNewTransTopo(AdjacencyList& overlay) {
  std::unordered_map<int, NodeTransTopo> transtopo;
  if (overlay.empty()) {
    return transtopo;
  }
  std::queue<int> q;
  std::unordered_map<int, bool> visited;

  int root = overlay.begin()->first;
  q.push(root);
  visited[root] = true;
  transtopo[root] = NodeTransTopo();

  while (!q.empty()) {
    int current = q.front();
    q.pop();
    for (int neighbor : overlay[current]) {
      if (!visited[neighbor]) {
        visited[neighbor] = true;
        q.push(neighbor);
        transtopo[current].addChildren(neighbor);
        transtopo[neighbor].setParent(current);
      }
    }
  }
  return transtopo;
}

GlobalTransTopo ConstelTransTopoThinker::decideNewTransTopo(AdjacencyList& overlay,int) {
  std::unordered_map<int, NodeTransTopo> transtopo;
  if (overlay.empty()) {
    return transtopo;
  }
  int prvs=-1;
  for(const auto& it:overlay){
    auto id = it.first;
    if(prvs!=-1){
      transtopo[id].setParent(prvs);
      transtopo[prvs].addChildren(id);
    }
    prvs = id;
  }

  return transtopo;
}


GlobalTransTopo ConstelTransTopoThinker::SendOverlay(AdjacencyList& overlay) {
  if (overlay.size() == 1) {
    GlobalTransTopo transtopo;
    NodeTransTopo topo;
    topo.setoRoot();
    int node_id = overlay.begin()->first;
    transtopo[node_id] = topo;
    return transtopo;
  }
  return decideNewTransTopo(overlay,1);
}

}  // namespace constellation
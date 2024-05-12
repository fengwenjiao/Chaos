#ifndef _CONSTELLATION_TRANSTOPOTHINKER_H_
#define _CONSTELLATION_TRANSTOPOTHINKER_H_

#include <queue>
#include "./constellation_commons.h"

namespace constellation {

class ConstelTransTopoThinker {
 public:
  GlobalTransTopo decideNewTransTopo(AdjacencyList& overlay) {
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
  }
  // Send the overlay to the thinker
  //TODO:void SendOverlaytoThinker(std::string), now it is debug version
  GlobalTransTopo SendOverlay(AdjacencyList& overlay){
    return decideNewTransTopo(overlay);
  }

 private:
 bool is_local_thinker_;
};

}  // namespace constellation
#endif  // _CONSTELLATION_TRANSTOPOTHINKER_H_
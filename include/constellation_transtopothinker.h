#ifndef _CONSTELLATION_TRANSTOPOTHINKER_H_
#define _CONSTELLATION_TRANSTOPOTHINKER_H_

#include <queue>
#include <unordered_map>
#include "./constellation_commons.h"

namespace constellation {

class ConstelTransTopoThinker {
 public:
  GlobalTransTopo decideNewTransTopo(AdjacencyList& overlay);
  GlobalTransTopo SendOverlay(AdjacencyList& overlay);

 private:
  bool is_local_thinker_;
};

}  // namespace constellation
#endif  // _CONSTELLATION_TRANSTOPOTHINKER_H_
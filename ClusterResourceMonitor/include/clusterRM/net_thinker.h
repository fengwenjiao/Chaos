#ifndef MONITER_NET_THINKER_H
#define MONITER_NET_THINKER_H
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace moniter {
class NetThinker {
 public:
  static std::vector<std::vector<std::pair<int, int>>> edge_coloring(
      const std::unordered_map<int, std::vector<int>>& topo);

 private:
  NetThinker() = delete;
  ~NetThinker() = delete;

  struct Edge {
    int u;
    int v;
    Edge(int a, int b) : u(a), v(b) {
      if (u > v)
        std::swap(u, v);
    }
    bool operator==(const Edge& other) const {
      return u == other.u && v == other.v;
    }
  };

  struct EdgeHash {
    size_t operator()(const Edge& e) const {
      return std::hash<int>()(e.u) ^ std::hash<int>()(e.v);
    }
  };

  static void topo_parser(const std::unordered_map<int, std::vector<int>>& topo,
                          std::unordered_set<Edge, EdgeHash>& edges,
                          std::unordered_set<int>& node_set);
};

}  // namespace moniter
#endif  // MONITER_NET_THINKER_H
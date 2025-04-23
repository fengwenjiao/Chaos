#include "net_thinker.h"
#include <unordered_set>
#include "util.h"

namespace moniter {
std::vector<std::vector<std::pair<int, int>>> NetThinker::edge_coloring(
    const std::unordered_map<int, std::vector<int>>& topo) {
  std::unordered_set<Edge, EdgeHash> edges;
  std::unordered_set<int> node_set;
  topo_parser(topo, edges, node_set);

  for (const auto& node : topo) {
    int u = node.first;
    for (int v : node.second) {
      Edge e(u, v);
      edges.insert(e);
    }
  }

  std::unordered_map<Edge, int, EdgeHash> edge_colors;
  std::unordered_map<int, std::unordered_set<int>> node_colors;

  // coloring
  for (const Edge& e : edges) {
    std::unordered_set<int> used_colors;

    if (node_colors.find(e.u) != node_colors.end()) {
      used_colors.insert(node_colors[e.u].begin(), node_colors[e.u].end());
    }
    if (node_colors.find(e.v) != node_colors.end()) {
      used_colors.insert(node_colors[e.v].begin(), node_colors[e.v].end());
    }

    int color = 1;
    while (used_colors.find(color) != used_colors.end()) {
      color++;
    }

    edge_colors[e] = color;
    node_colors[e.u].insert(color);
    node_colors[e.v].insert(color);
  }

  std::unordered_map<int, std::vector<std::pair<int, int>>> color_groups;
  for (const auto& ec : edge_colors) {
    color_groups[ec.second].push_back(std::make_pair(ec.first.u, ec.first.v));
  }
  std::vector<std::vector<std::pair<int, int>>> result;
  for (const auto& group : color_groups) {
    result.push_back(group.second);
  }
  return result;
}

void NetThinker::topo_parser(
    const std::unordered_map<int, std::vector<int>>& topo,
    std::unordered_set<Edge, EdgeHash>& edges,
    std::unordered_set<int>& node_set) {
  node_set.clear();
  edges.clear();

  for (const auto& node : topo) {
    int u = node.first;

    node_set.insert(u);

    for (int v : node.second) {
      if (v != u) {
        Edge e(u, v);
        edges.insert(e);
      } else {
        LOG_WARNING_("Self loop detected: " + std::to_string(u) +
                     " ,remove this loop");
      }
    }
  }
}

}  // namespace moniter
#pragma once

#include <queue>
#include <functional>
#include <random>

#include "constellation_commons.h"
#include "./algorithm_helper.h"

namespace constellation::algorithm::basic {

GlobalTransTopo dfs_method(const AdjacencyList& overlay) {
  std::unordered_map<int, NodeTransTopo> transtopo;
  if (overlay.empty()) {
    return transtopo;
  }
  std::queue<int> q;
  std::unordered_map<int, bool> visited;

  int root = overlay.cbegin()->first;
  q.push(root);
  visited[root] = true;
  transtopo[root] = NodeTransTopo();

  while (!q.empty()) {
    int current = q.front();
    q.pop();
    for (const auto& neighbor : overlay.at(current)) {
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

GlobalTransTopo random_choose_method(const AdjacencyList& overlay) {
  using Edge = std::pair<int, int>;
  GlobalTransTopo transtopo;
  if (overlay.empty()) {
    return transtopo;
  }

  AdjacencyList mst;
  std::vector<Edge> edges;
  std::unordered_set<int> nodes;

  for (const auto& [node, neighbors] : overlay) {
    nodes.insert(node);
    for (const auto& neighbor : neighbors) {
      if (node < neighbor) {
        edges.emplace_back(node, neighbor);
      }
    }
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(edges.begin(), edges.end(), gen);

  algorithm::helper::UnionFind<int> uf(nodes);

  for (const auto& [u, v] : edges) {
    if (uf.unionSets(u, v)) {
      mst[u].push_back(v);
      mst[v].push_back(u);
    }
  }

  auto buildTree = [](int root, const AdjacencyList& mst, GlobalTransTopo& transtopo) {
    std::unordered_set<int> visited;
    std::function<void(int)> dfs = [&](int node) {
      visited.insert(node);
      for (const int& neighbor : mst.at(node)) {
        if (visited.find(neighbor) == visited.end()) {
          transtopo[neighbor].setParent(node);
          transtopo[node].addChildren(neighbor);
          dfs(neighbor);
        }
      }
    };
    dfs(root);
  };

  int root = *nodes.begin();

  buildTree(root, mst, transtopo);

  bool flag = false;
  for (const auto& [_, topo] : transtopo) {
    if (topo.getType() == NodeTransTopo::Type::kUnset) {
      throw std::runtime_error("Unset node in the transtopo");
    }
    if (flag) {
      if (topo.getType() == NodeTransTopo::Type::kRoot) {
        throw std::runtime_error("More than one root node in the transtopo");
      }
    } else {
      if (topo.getType() == NodeTransTopo::Type::kRoot) {
        flag = true;
      }
    }
  }

  return transtopo;
}

}  // namespace constellation::algorithm::basic
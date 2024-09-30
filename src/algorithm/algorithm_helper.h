#pragma once

#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace constellation::algorithm::helper {

/**
 * @brief A simple union-find data structure
 * @tparam Node The type of the node. Must be hashable and comparable.
 */
template <typename Node>
class UnionFind {
 public:
  /* @brief Constructor
   * @param nodes The set of nodes
   */
  UnionFind(const std::unordered_set<Node>& nodes) : parent_(nodes.size()) {
    for (const auto& node : nodes) {
      parent_[node] = node;
    }
  }

  /* @brief Find the root of the set
   * @param x The node to find
   * @return The root of the set
   */
  const Node& find(const Node& x) {
    if (parent_[x] == x) {
      return x;
    }
    parent_[x] = find(parent_[x]);
    return parent_[x];
  }

  /* @brief Union two sets
   * @param x The first node
   * @param y The second node
   * @return true if the two sets are unioned, false if they are already in the same set
   */
  bool unionSets(const Node& x, const Node& y) {
    const auto& rootX = find(x);
    const auto& rootY = find(y);
    if (rootX != rootY) {
      parent_[rootX] = rootY;
      return true;
    }
    return false;
  }

 private:
  std::unordered_map<Node, Node> parent_;
};

template <typename T>
bool checkUnique(const std::vector<T>& vec) {
  std::unordered_set<T> set;
  for (const auto& item : vec) {
    if (set.find(item) != set.end()) {
      return false;
    }
    set.insert(item);
  }
  return true;
}

}  // namespace constellation::algorithm::helper
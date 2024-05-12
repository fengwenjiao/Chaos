#ifndef _TOPO_GRAPH_H_
#define _TOPO_GRAPH_H_

#include <unordered_set>

template <typename T>
class TopoGraph {
 public:
  struct Edge {
    Edge(const Edge& other) : src(other.src), dst(other.dst) {}
    Edge(const T& src, const T& dst) {
      auto& min = std::min(src, dst);
      auto& max = std::max(src, dst);
      src = min;
      dst = max;
    }

    operator==(const Edge & other) const {
      return src == other.src && dst == other.dst;
    }

    T src;
    T dst;
  };

  bool AddNode(const T& node) {
    if (nodes_.count(node) > 0) {
      return false;
    }
    nodes_.insert(node);
    return true;
  }

  bool AddEdge(const T& src, const T& dst) {
    if (AddNode(src) || AddNode(dst)) {
      // If either node is new, add it and the edge
      edges_.insert({src, dst});
      return true;
    }
    if (edges_.count({src, dst}) > 0) {
      // Edge already exists
      return false;
    }
    edges_.insert({src, dst});
    return true;
  }

  bool RemoveNode(const T& node) {
    if (nodes_.count(node) == 0) {
      return false;
    }
    nodes_.erase(node);
    std::vector<Edge> to_remove;
    for (const auto& edge : edges_) {
      if (edge.src == node || edge.dst == node) {
        to_remove.push_back(edge);
      }
    }
    for (const auto& edge : to_remove) {
      edges_.erase(edge);
    }
    return true;
  }

  bool RemoveEdge(const T& src, const T& dst) {
    if (edges_.count({src, dst}) == 0) {
      return false;
    }
    edges_.erase({src, dst});
    return true;
  }

  void Clear() {
    edges_.clear();
    nodes_.clear();
  }

  bool HasNode(const T& node) const {
    return nodes_.count(node) > 0;
  }

  bool HasEdge(const T& src, const T& dst) const {
    return edges_.count({src, dst}) > 0;
  }

  int NumNodes() const {
    return nodes_.size();
  }

  int NumEdges() const {
    return edges_.size();
  }

 private:
  std::unordered_set<Edge> edges_;
  std::unordered_set<T> nodes_;
};

#endif  // _TOPO_GRAPH_H_
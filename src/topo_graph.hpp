#ifndef _TOPO_GRAPH_H_
#define _TOPO_GRAPH_H_

#include <unordered_set>
#include <vector>
#include <string>
#include <memory>

namespace constellation {

namespace topo {

template <typename E>
struct EdgeProperty {
  E link_property;
};

template <>
struct EdgeProperty<void> {};

template <typename T, typename E = void>
struct Edge : EdgeProperty<E> {
  using Node = T;
  Edge() {}
  Edge(const Edge& other) : src(other.src), dst(other.dst) {}
  Edge(const T& src, const T& dst) : src(src < dst ? src : dst), dst(src < dst ? dst : src) {}
  virtual ~Edge() {}
  bool operator==(const Edge& other) const {
    return this->src == other.src && this->dst == other.dst;
  }

  T src;
  T dst;

  struct Hash {
    std::size_t operator()(const Edge& edge) const {
      return std::hash<T>()(edge.src) ^ std::hash<T>()(edge.dst);
    }
  };
};

template <typename Edge>
class TopoGraph {
 public:
  using T = typename Edge::Node;
  TopoGraph() {}
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
    return edges_.insert(Edge(src, dst)).second;
  }

  bool RemoveNode(const T& node) {
    if (nodes_.count(node) == 0) {
      return false;
    }
    nodes_.erase(node);
    std::vector<Edge> to_remove;
    // may use a map record node->edge to speed up
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
    return edges_.erase(Edge(src, dst)) > 0;
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

  const std::unordered_set<Edge, typename Edge::Hash>& GetEdges() const {
    return edges_;
  }

  const std::unordered_set<T>& GetNodes() const {
    return nodes_;
  }

 private:
  std::unordered_set<Edge, typename Edge::Hash> edges_;
  std::unordered_set<T> nodes_;
};

}  // namespace topo
}  // namespace constellation
#endif  // _TOPO_GRAPH_H_
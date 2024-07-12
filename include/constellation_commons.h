#ifndef _CONSTELLATION_COMMONS_H_
#define _CONSTELLATION_COMMONS_H_

#include <unordered_map>
#include <vector>
#include "ps/base.h"

namespace constellation {

/** @brief Overlay topology*/
using AdjacencyList = std::unordered_map<int, std::vector<int>>;
/** @brief Node transport topology.
 * including member parent and children
 * @param `type`: the `type` of this node, `kRoot` - root node, `kLeaf` - leaf node,
 * `kInner` - inner node and `kUnset` - unset
 * - parent: the parent of this node, only valid for inner and leaf nodes
 * - children: the children of this node
 */
class NodeTransTopo {
 public:
  enum class Type {
    kRoot,
    kLeaf,
    kInner,
    kUnset,
  };

  NodeTransTopo() : type_(Type::kUnset), parent_(0) {}
  NodeTransTopo(const NodeTransTopo& other) {
    type_ = other.type_;
    parent_ = other.parent_;
    children_ = other.children_;
  }
  NodeTransTopo(NodeTransTopo&& other) {
    type_ = other.type_;
    parent_ = other.parent_;
    children_ = std::move(other.children_);
  }
  NodeTransTopo& operator=(const NodeTransTopo& other) {
    if (this == &other) {
      return *this;
    }
    type_ = other.type_;
    parent_ = other.parent_;
    children_ = other.children_;
    return *this;
  }
  NodeTransTopo& operator=(NodeTransTopo&& other) {
    if (this == &other) {
      return *this;
    }
    type_ = other.type_;
    parent_ = other.parent_;
    children_ = std::move(other.children_);
    return *this;
  }

  Type getType() const {
    return type_;
  }

  int getParent() const {
    return parent_;
  }

  const std::vector<int>& getChildren() const {
    return children_;
  }

  bool setoRoot() {
    if (this->parent_ == 0) {
      this->type_ = Type::kRoot;
      return true;
    }
    return false;
  }

  void setParent(int parent) {
    CHECK_GE(parent, ps::kMinTrainerID);
    parent_ = parent;
    type_ = children_.size() > 0 ? Type::kInner : Type::kLeaf;
  }

  void addChildren(int child) {
    CHECK_GE(child, ps::kMinTrainerID);
    children_.push_back(child);
    type_ = type_ == Type::kLeaf ? Type::kInner : Type::kRoot;
  }

  std::string Encode() const {
    std::string str = std::to_string(static_cast<int>(type_)) + " ";
    str += std::to_string(parent_) + " ";
    for (int child : children_) {
      str += std::to_string(child) + " ";
    }
    return str;
  }

  void Decode(const std::string& str) {
    if (str.empty())
      return;
    size_t pos = 0;
    type_ = static_cast<Type>(std::stoi(str, &pos));
    parent_ = std::stoi(str.substr(pos), &pos);
    children_.clear();
    while (pos < str.size()) {
      children_.push_back(std::stoi(str.substr(pos), &pos));
    }
  }

 private:
  Type type_;
  int parent_;
  std::vector<int> children_;
};

/** @brief Global transport topology.
 * including all nodes' transport topology
 */
using GlobalTransTopo = std::unordered_map<int, NodeTransTopo>;

/** @brief Encode the global transport topology to string
 * @param `transtopo`: the global transport topology
 * @param `str`: the string to store the encoded result
 */
static void EncodeGlobalTransTopo(const GlobalTransTopo& transtopo, std::string* str) {
  for (const auto& kv : transtopo) {
    *str += std::to_string(kv.first) + " " + kv.second.Encode() + " ";
  }
}

/** @brief Decode the global transport topology from string
 * @param `str`: the string to store the encoded result
 * @param `transtopo`: the global transport topology
 */
static void DecodeGlobalTransTopo(const std::string& str, GlobalTransTopo* transtopo) {
  transtopo->clear();
  if (str.empty())
    return;
  size_t pos = 0;
  while (pos < str.size()) {
    int node = std::stoi(str.substr(pos), &pos);
    NodeTransTopo n;
    n.Decode(str.substr(pos));
    transtopo->insert({node, n});
  }
}

class ScaleClock {
 public:
  struct Tick {
    Tick() : timestamp(0) {}
    Tick(uint32_t ts, const GlobalTransTopo& topo) : timestamp(ts), transtopo(topo) {}
    Tick(uint32_t ts, GlobalTransTopo&& topo) : timestamp(ts), transtopo(std::move(topo)) {}
    Tick& operator=(const Tick& other) {
      timestamp = other.timestamp;
      transtopo = other.transtopo;
      return *this;
    }
    Tick& operator=(Tick&& other) {
      timestamp = other.timestamp;
      transtopo = std::move(other.transtopo);
      return *this;
    }
    uint32_t timestamp;
    GlobalTransTopo transtopo;

    void Encode(std::string* str) const {
      // timestamp GlobalTransTopo1{target_node_id NodeTransTopo{type parent [children]}}
      *str += std::to_string(timestamp) + " ";
      EncodeGlobalTransTopo(transtopo, str);
    }
    void Decode(const std::string& str) {
      size_t pos = 0;
      timestamp = std::stoi(str, &pos);
      DecodeGlobalTransTopo(str.substr(pos), &transtopo);
    }
  };

  void setAlarm(const Tick& tick) {
    int timestamp = tick.timestamp;
    ticks_[timestamp] = tick;
  }

  void setAlarm(Tick&& tick) {
    int timestamp = tick.timestamp;
    ticks_[timestamp] = std::move(tick);
  }

  void clockTick() {
    local_timestamp_++;
    // TODO:check if there is any alarm
  }

  uint32_t getLocalTimestamp() const {
    return local_timestamp_;
  }

 private:
  uint32_t local_timestamp_ = 0;
  std::unordered_map<uint32_t, Tick> ticks_;
};

static const int SignalBound = 100;

enum class kControllerSignal {
  kAddNodeSignal = 101,
  kNodeReadySignal = 102,
  kUpdateTransTopoAnouce = 103,
};

}  // namespace constellation
#endif  // _CONSTELLATION_COMMONS_H_
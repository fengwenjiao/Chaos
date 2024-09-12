#ifndef _CONSTELLATION_COMMONS_H_
#define _CONSTELLATION_COMMONS_H_

#include "ps/base.h"
#include "./internal/utils.h"
#include "./internal/serilite.hpp"

#include <unordered_map>
#include <vector>

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
struct NodeTransTopo {
  enum class Type {
    kRoot,
    kLeaf,
    kInner,
    kUnset,
  };

  NodeTransTopo() = default;
  NodeTransTopo(const NodeTransTopo& other) = default;
  NodeTransTopo(NodeTransTopo&& other) noexcept = default;
  NodeTransTopo& operator=(const NodeTransTopo& other) = default;
  NodeTransTopo& operator=(NodeTransTopo&& other) noexcept = default;

  const Type& getType() const {
    return type_;
  }

  const int& getParent() const {
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


  Type type_;
  int parent_;
  std::vector<int> children_;
};

/** @brief Global transport topology.
 * including all nodes' transport topology
 */
using GlobalTransTopo = std::unordered_map<int, NodeTransTopo>;

struct ScaleClock {
 public:
  struct Tick {
    Tick() = default;
    Tick(const Tick& other) = default;
    Tick& operator=(const Tick& other) = default;
    Tick& operator=(Tick&& other) noexcept = default;
    Tick(Tick&& other) noexcept = default;
    
    uint32_t timestamp;
    GlobalTransTopo transtopo;
  };

  void removeTick(uint32_t timestamp) {
    auto it = ticks_.find(timestamp);
    if (it != ticks_.end()) {
      ticks_.erase(it);
    }
  }

  void setAlarm(Tick tick) {
    auto timestamp = tick.timestamp;
    ticks_[timestamp] = std::move(tick);
  }

  bool clockTick() {
    local_timestamp_++;

    if (ticks_.find(local_timestamp_) != ticks_.end()) {
      return true;
    }
    return false;
  }

  const uint32_t& getLocalTimestamp() const {
    return local_timestamp_;
  }

  uint32_t local_timestamp_ = 0;
  std::unordered_map<uint32_t, Tick> ticks_;
};

static const int SignalBound = 100;

enum class kControllerSignal {
  kAddNodeSignal = 101,
  kNodeReadySignal = 102,
  kUpdateTransTopoAnouce = 103,
  kUpdateClockSignal = 104,
};

}  // namespace constellation
#endif  // _CONSTELLATION_COMMONS_H_
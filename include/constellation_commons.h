#ifndef _CONSTELLATION_COMMONS_H_
#define _CONSTELLATION_COMMONS_H_

#include "ps/base.h"
#include "ps/range.h"
#include "./internal/utils.h"


#include <unordered_map>
#include <vector>
#include <cstdint>

#define DEFAULT_SPECIAL_MEMBERS(Type) \
  Type() = default; \
  Type(const Type& other) = default; \
  Type(Type&& other) noexcept = default; \
  Type& operator=(const Type& other) = default; \
  Type& operator=(Type&& other) noexcept = default;

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

  DEFAULT_SPECIAL_MEMBERS(NodeTransTopo);

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

struct TransPath{
  std::vector<int> path;

  template <typename... T>
  TransPath(T... args) : path({args...}) {}

  // Hash function
  struct PathHash {
    std::size_t operator()(const TransPath& v) const {
      std::hash<int> hasher;
      std::size_t seed = 0;
      for (int i : v.path) {
        seed ^= hasher(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    }
  };

  // Equal function
  struct PathEqual {
    bool operator()(const TransPath& lhs, const TransPath& rhs) const {
      for (size_t i = 0; i < lhs.path.size(); i++) {
        if (lhs.path[i] != rhs.path[i]) {
          return false;
        }
      }
      return true;
    }
  };
};


/** @brief Model synchronization configuration.
 * including target node id, keys, paths and slices
 * @param `target_node_id`: the target node id
 * @param `keys`: the keys of the model
 * @param `paths`: indicate the node path of the slice
 * @param `slices`: the slices of the model
 */
struct ModelSycnConf{
  DEFAULT_SPECIAL_MEMBERS(ModelSycnConf);
  std::vector<int> target_node_id;
  std::vector<int> keys;
  std::vector<std::vector<int>> paths;
  std::vector<ps::Range> slices;
};

/** @brief Global model synchronization configuration.
 * including all nodes' model synchronization configuration
 */
using GlobalModelSyncConf = std::unordered_map<int, ModelSycnConf>;

struct ScaleClock {
 public:
  struct Tick {
    DEFAULT_SPECIAL_MEMBERS(Tick);
    
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
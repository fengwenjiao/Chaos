#ifndef _CONSTELLATION_COMMONS_H_
#define _CONSTELLATION_COMMONS_H_

#include "ps/base.h"
#include "ps/range.h"
#include "./internal/utils.h"

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <deque>
#include <mutex>

#define DEFAULT_SPECIAL_MEMBERS(Type)           \
  Type() = default;                             \
  Type(const Type& other) = default;            \
  Type(Type&& other) noexcept = default;        \
  Type& operator=(const Type& other) = default; \
  Type& operator=(Type&& other) noexcept = default;

namespace constellation {

template <typename T>
bool std_isin(const T& val, const std::vector<T>& vec) {
  return std::find(vec.begin(), vec.end(), val) != vec.end();
}

template <typename T, typename V>
bool std_isin(const T& val, const std::unordered_map<T, V>& map) {
  return map.find(val) != map.end();
}

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

  std::string debug_string() const {
    std::string s = "{";
    s += "'type': " + std::to_string(static_cast<int>(type_)) + ",";
    s += "'parent': " + std::to_string(parent_) + ",";
    s += "'children': [";
    for (size_t i = 0; i < children_.size(); i++) {
      const char* c = i == children_.size() - 1 ? "" : ",";
      s += std::to_string(children_[i]) + c;
    }
    s += "]}";
    return s;
  }

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
  void update_type() {
    if (parent_ == 0) {
      type_ = Type::kRoot;
    } else {
      type_ = children_.size() > 0 ? Type::kInner : Type::kLeaf;
    }
  }

  void setParent(int parent) {
    CHECK_GE(parent, ps::kMinTrainerID);
    parent_ = parent;
    update_type();
  }

  void addChildren(int child) {
    CHECK_GE(child, ps::kMinTrainerID);
    children_.push_back(child);
    update_type();
  }

  Type type_;
  int parent_;
  std::vector<int> children_;
};

/** @brief Global transport topology.
 * including all nodes' transport topology
 */
using GlobalTransTopo = std::unordered_map<int, NodeTransTopo>;

struct TransPath {
  std::vector<int> path;

  TransPath() = default;
  TransPath(std::initializer_list<int> list) : path(list) {}
  TransPath(std::vector<int> path) : path(std::move(path)) {}

  // Hash function
  struct PathHash {
    std::size_t operator()(const TransPath& v) const {
      std::hash<int> hasher;
      std::size_t seed = 0;
      for (auto& i : v.path) {
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

  bool operator==(const TransPath& rhs) const {
    if (path.size() != rhs.path.size()) {
      return false;
    }
    for (size_t i = 0; i < path.size(); i++) {
      if (path[i] != rhs.path[i]) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const TransPath& rhs) const {
    return !(*this == rhs);
  }
  bool operator<(const TransPath& rhs) const {
    return std::tie(path) < std::tie(rhs.path);
  }
};

struct ReadySignalBody {
  DEFAULT_SPECIAL_MEMBERS(ReadySignalBody);
  int id;
  bool need_sycn_model;
  std::vector<int> keys;
  std::vector<uint64_t> lens;
};

struct KVSlice {
  DEFAULT_SPECIAL_MEMBERS(KVSlice);
  int key_begin, key_end;
  uint64_t slice = 0;
  uint64_t slice_len = 0;
  KVSlice(const int& key_begin, const int& key_end) : key_begin(key_begin), key_end(key_end) {}
  KVSlice(const int& key, const uint64_t& slice, const uint64_t& slice_len)
      : key_begin(key), key_end(key), slice(slice), slice_len(slice_len) {}
  void inc_range() {
    if (is_full()) {
      key_end++;
    }
  }
  bool is_full() const {
    return key_end > key_begin;
  }
  std::string debug_string() {
    std::string s;
    s += "keys: " + std::to_string(key_begin) + "-" + std::to_string(key_end) + " ";
    s += "slice: " + std::to_string(slice) + " slice len:" + std::to_string(slice_len);
    return s;
  }
};

/** @brief Model synchronization configuration.
 * including target node id, keys, paths and slices
 * @param `target_node_id`: the target node id
 * @param `keys`: the keys of the model
 * @param `paths`: indicate the node path of the slice
 * @param `slices`: the slices of the model
 */
struct ModelSycnConf {
  DEFAULT_SPECIAL_MEMBERS(ModelSycnConf);

  std::vector<int> target_node_id;

  std::vector<std::vector<KVSlice>> kvslices;
  std::vector<std::vector<int>> paths;

  void Clear() {
    target_node_id.clear();
    kvslices.clear();
    paths.clear();
  }

  std::string debug_string() {
    std::string s;
    s += "target node id: ";
    for (auto& i : target_node_id) {
      s += std::to_string(i) + " ";
    }
    s += "{ ";
    for (size_t i = 0; i < kvslices.size(); i++) {
      s += "path: ";
      for (auto& j : paths[i]) {
        s += std::to_string(j) + " ";
      }
      s += "kvslices: ";
      for (auto& j : kvslices[i]) {
        s += j.debug_string() + " ";
      }
    }
    s += "}";
    return s;
  }
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
    ModelSycnConf model_sync_conf;

    std::string debug_string() {
      std::string s;
      s += "timestamp: " + std::to_string(timestamp) + " ";
      s += "transtopo: ";
      for (auto& [id, topo] : transtopo) {
        s += std::to_string(id) + ": " + topo.debug_string() + " ";
      }
      s += "\nmodel_sync_conf: " + model_sync_conf.debug_string();
      return s;
    }
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
    // TODO： to be improved
    mu_.lock();
    local_timestamp_++;

    if (ticks_.find(local_timestamp_) != ticks_.end()) {
      return true;
    }
    return false;
  }
  void unlock() {
    mu_.unlock();
  }

  const uint32_t& getLocalTimestamp() const {
    std::lock_guard<std::mutex> lk(mu_);
    return local_timestamp_;
  }

  uint32_t local_timestamp_ = 0;
  mutable std::mutex mu_;  // protect the local_timestamp_
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
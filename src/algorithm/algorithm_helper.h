#pragma once

#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <type_traits>
#include <random>

namespace constellation::algorithm::helper {

namespace details {
// Check if T has an iterator and a value_type member type.
template <typename T, typename = void>
struct has_iter_value : std::false_type {};

template <typename T>
struct has_iter_value<
    T,
    std::void_t<typename T::iterator, typename T::const_iterator, typename T::value_type>>
    : std::true_type {};

// Alias for has_iter_value<T>::value.
template <typename T>
constexpr bool has_iter_value_v = has_iter_value<T>::value;

// Check if T is iterable.
template <typename T, typename = void>
struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<
    T,
    std::void_t<decltype(std::begin(std::declval<T>()) == std::end(std::declval<T>()))>>
    : std::bool_constant<has_iter_value_v<T>> {};

// Alias for is_iterable<T>::value.
template <typename T>
constexpr bool is_iterable_v = is_iterable<T>::value;

}  // namespace details

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

template <typename T,
          typename std::enable_if_t<details::is_iterable_v<T>, int> = 0,
          typename V = typename std::decay_t<decltype(*std::declval<const T&>().begin())>>
std::vector<V> randomChoose(const T& container, size_t num) {
  size_t size = std::min(container.size(), num);
  if (size == 0) {
    return {};
  } else if (size == container.size()) {
    return std::vector<V>(container.begin(), container.end());
  }
  std::vector<V> result;
  result.reserve(size);

  std::random_device rd;
  std::mt19937 gen(rd());

  std::vector<size_t> indices(container.size());
  for (size_t i = 0; i < indices.size(); ++i) {
    indices[i] = i;
  }
  std::shuffle(indices.begin(), indices.end(), gen);

  for (size_t i = 0; i < size; ++i) {
    result.emplace_back(*(container.begin() + indices[i]));
  }
  return result;
}

template <typename T>
std::vector<T> randomChoose(const std::initializer_list<T>& container, size_t num) {
  return randomChoose<std::initializer_list<T>>(container, num);
}

}  // namespace constellation::algorithm::helper
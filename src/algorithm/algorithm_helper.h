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

// Helper trait to check if a type is a specialization of std::unordered_map
template <typename T>
struct is_unordered_map : std::false_type {};

template <typename K, typename V, typename... Args>
struct is_unordered_map<std::unordered_map<K, V, Args...>> : std::true_type {};

template <typename T>
constexpr bool is_unordered_map_v = is_unordered_map<T>::value;

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

// Extract keys from unordered_map or return the vector itself
template <typename Container>
auto extract_elements(const Container& container) ->
    typename std::enable_if<!details::is_unordered_map_v<Container>, const Container&>::type {
  return container;
}

template <typename Container>
auto extract_elements(const Container& container) ->
    typename std::enable_if<details::is_unordered_map_v<Container>,
                            std::vector<typename Container::key_type>>::type {
  std::vector<typename Container::key_type> keys;
  keys.reserve(container.size());
  for (const auto& [key, _] : container) {
    keys.emplace_back(key);
  }
  return keys;
}

// Template function to check if two containers have unique and corresponding elements
template <typename Container1, typename Container2>
bool areElementsUniqueAndCorresponding(const Container1& cont1, const Container2& cont2) {
  // Extract elements (keys if unordered_map)
  auto elements1 = extract_elements(cont1);
  auto elements2 = extract_elements(cont2);

  // If elements are vectors, make copies for sorting
  std::vector<typename std::decay<decltype(elements1[0])>::type> sorted1;
  std::vector<typename std::decay<decltype(elements2[0])>::type> sorted2;

  if constexpr (details::is_unordered_map_v<Container1>) {
    sorted1 = std::move(elements1);
  } else {
    sorted1.assign(elements1.begin(), elements1.end());
  }
  if constexpr (details::is_unordered_map_v<Container2>) {
    sorted2 = std::move(elements2);
  } else {
    sorted2.assign(elements2.begin(), elements2.end());
  }
  // Check size
  if (sorted1.size() != sorted2.size()) {
    return false;
  }
  // Check uniqueness by sorting and removing duplicates
  std::sort(sorted1.begin(), sorted1.end());
  if (!checkUnique(sorted1)) {
    return false;  // cont1 has duplicate elements
  }

  std::sort(sorted2.begin(), sorted2.end());
  if (!checkUnique(sorted2)) {
    return false;  // cont2 has duplicate elements
  }

  // Check if elements are corresponding
  return std::equal(sorted1.begin(), sorted1.end(), sorted2.begin());
}

}  // namespace constellation::algorithm::helper
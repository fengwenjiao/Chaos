#ifndef CONSTELLATION_TEST_DEBUG_UTILS_H
#define CONSTELLATION_TEST_DEBUG_UTILS_H

#include <iostream>
#include <iomanip>
#include <vector>
#include <iterator>

#include "../include/internal/CArray.h"
#include "../include/constellation_commons.h"

namespace constellation {
namespace test {
inline unsigned long generateCArraySummary(const char* input, size_t size) {
  std::string str(input, size);

  std::hash<std::string> hasher;
  unsigned long hash = hasher(str);

  return hash;
}

class PRINT_CARRAY {
 public:
  static std::ostream& print(const CArray& arr, std::ostream& os = std::cout) {
    auto type = static_cast<ConstelDataType>(arr.dtype);
    switch (type) {
      case ConstelDataType::CONSTEL_FLOAT32:
        printImpl<float>(arr, os);
        break;
    }
    return os;
  }

 private:
  template <class T>
  static void printImpl(const CArray& arr, std::ostream& os) {
    const T* ptr = reinterpret_cast<const T*>(arr.data());
    int ele_num = arr.size() / sizeof(T);
    os << "CArray: {" << static_cast<const void*>(ptr)
       << " Abstract: " << generateCArraySummary(arr.data(), arr.size())
       << "}\n[ ";
    os << std::fixed << std::setprecision(5);
    // std::copy(ptr, ptr + ele_num, std::ostream_iterator<T>(os, " "));
    os << "]";
  }
};
inline std::ostream& operator<<(std::ostream& os, const CArray& arr) {
  return PRINT_CARRAY::print(arr, os);
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
  os << "Vector of " << typeid(T).name() << "{ Address: " << &vec
     << " Size:" << vec.size() << " }";
  os << "[ ";
  for (const auto& val : vec) {
    os << val << " ";
  }
  os << "]";
  return os;
}

template <>
inline std::ostream& operator<<(std::ostream& os,
                                const std::vector<CArray>& vec) {
  os << "Vector of CArray" << "{ Address: " << &vec << " Size:" << vec.size()
     << " }\n";
  os << "---------------------------------------------------\n";
  int i = 0;
  for (const auto& arr : vec) {
    os << "(" << i++ << ") ";
    os << arr << "\n";
  }
  os << "---------------------------------------------------\n";
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const NodeTransTopo& topo) {
  using NodeType = NodeTransTopo::Type;
  auto type_map = std::unordered_map<NodeType, std::string>{
      {NodeType::kUnset, "Unset"},
      {NodeType::kInner, "Inner"},
      {NodeType::kLeaf, "Leaf"},
      {NodeType::kRoot, "Root"},
  };
  os << "{NodeType: " << type_map[topo.getType()] << ", Parent: ["
     << topo.getParent() << "] ,Children: [" << topo.getChildren() << "]}";

  return os;
}

}  // namespace test
}  // namespace constellation

#endif  // CONSTELLATION_TEST_DEBUG_UTILS_H

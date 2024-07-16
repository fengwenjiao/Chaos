#ifndef CONSTELLATION_TEST_DEBUG_UTILS_H
#define CONSTELLATION_TEST_DEBUG_UTILS_H

#include <iostream>
#include <vector>
#include <iterator>

#include "../include/internal/CArray.h"

namespace constellation {
namespace test {
unsigned long generateCArraySummary(const char* input) {
  // 将 char* 转换为 std::string
  std::string str(input);

  // 创建一个 std::hash 对象并用它来计算字符串的哈希值
  std::hash<std::string> hasher;
  unsigned long hash = hasher(str);

  // 返回哈希值作为摘要
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
       << " Abstract: " << generateCArraySummary(arr.data()) << "}\n[ ";
    std::copy(ptr, ptr + ele_num, std::ostream_iterator<T>(os, " "));
    os << "]";
  }
};
std::ostream& operator<<(std::ostream& os, const CArray& arr) {
  return PRINT_CARRAY::print(arr, os);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
  os << "Vector of " << typeid(T).name() << "{ Address: " << &vec << " Size:" << vec.size()
     << " }\n";
  os << "[ ";
  for (const auto& val : vec) {
    os << val << " ";
  }
  os << "]";
  return os;
}

template <>
std::ostream& operator<<(std::ostream& os, const std::vector<CArray>& vec) {
  os << "Vector of CArray"
     << "{ Address: " << &vec << " Size:" << vec.size() << " }\n";
  os << "---------------------------------------------------\n";
  int i = 0;
  for (const auto& arr : vec) {
    os << "(" << i++ << ") ";
    os << arr << "\n";
  }
  os << "---------------------------------------------------\n";
  return os;
}

}  // namespace test
}  // namespace constellation

#endif  // CONSTELLATION_TEST_DEBUG_UTILS_H

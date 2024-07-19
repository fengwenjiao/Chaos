#ifndef CONSTELLATION_UTILS_H
#define CONSTELLATION_UTILS_H

#include <string>
#include <stdexcept>

namespace constellation {
//std::string findValue(const std::string& str,
//                      const std::string& key,
//                      std::string bracket = "") noexcept {
//  // test: 123
//  auto start = str.find(key) + key.size();
//  // skip : and space
//  start = str.find_first_not_of(": ", start);
//  if (start == std::string::npos) {
//    return "";
//  }
//  if(!bracket.empty()){
//    if (bracket.size() != 2) return "";
//    if(str[start] != bracket[0]){
//      return "";
//    }
//    start++;
//    auto end = str.find_first_of(bracket[1], start);
//    if (end == std::string::npos) {
//      return "";
//    }
//    return str.substr(start, end - start);
//
//  }else{
//    //find the first , or } ]
//    auto end = str.find_first_of(",}] ", start);
//    end = end == std::string::npos ?str.size():end;
//    return str.substr(start, end - start);
//  }
//
//}
}  // namespace constellation

#endif  // CONSTELLATION_UTILS_H

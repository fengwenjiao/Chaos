#ifndef MONITER_Util_H_
#define MONITER_Util_H_
#include <cstdlib>
#include <iostream>
#include <cstring>

#define DEFAULT_SPECIAL_MEMBERS(Type)           \
  Type() = default;                             \
  Type(const Type& other) = default;            \
  Type(Type&& other) noexcept = default;        \
  Type& operator=(const Type& other) = default; \
  Type& operator=(Type&& other) noexcept = default;

#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
enum LogLevel {
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_NONE
};
extern LogLevel currentLogLevel;

#define LOG_INFO_(msg)                                                         \
  do {                                                                         \
    if (currentLogLevel <= LOG_LEVEL_INFO) {                                   \
      std::cout << "[INFO][" << __FILENAME__ << ":" << __LINE__ << "]:" << msg \
                << std::endl;                                                  \
    }                                                                          \
  } while (0)

#define LOG_WARNING_(msg)                                          \
  do {                                                             \
    if (currentLogLevel <= LOG_LEVEL_WARNING) {                    \
      std::cout << "[WARNING][" << __FILENAME__ << ":" << __LINE__ \
                << "]:" << msg << std::endl;                       \
    }                                                              \
  } while (0)

#define LOG_ERROR_(msg)                                                      \
  do {                                                                       \
    if (currentLogLevel <= LOG_LEVEL_ERROR) {                                \
      std::cout << "[ERROR][" << __FILENAME__ << ":" << __LINE__ << "] "     \
                << msg;                                                      \
      if (errno != 0) {                                                      \
        std::cout << ", errno: " << errno << " (" << strerror(errno) << ")"; \
        errno = 0;                                                           \
      }                                                                      \
      std::cout << std::endl;                                                \
      std::cout.flush();                                                     \
      throw std::runtime_error("smq::error");                                \
    }                                                                        \
  } while (0)

namespace moniter {

class Util {
 public:
  /**
   *\brief open file and return a content
   *\param filepathPath Path to the file to be open, like "/proc/cpuinfo"
   *\return file content
   */
  static std::string open_file(const std::string& filePath);
  /**
   *\brief execute command in shell and return the result
   *\param cmd Command to be executed
   *\return result of exeution
   */
  static std::string exec(const std::string& cmd);
  /**
   *\brief find the value corresponding to key in text, like" Max   : 20 % "
   *start from pos
   *\param text Text to find value in
   *\param key Key word
   *\param pos search value from pos
   *\return value corresponding to the key
   */
  static std::string find_value(const std::string& text,
                                const std::string& key,
                                size_t pos = 0);
  /**
   *\brief remove the unit in a string ,like "1234.2 MB"->"1234.2"
   *\param text Text to remove unit in.
   */
  static std::string remove_unit(const std::string& text);
};
}  // namespace moniter

#endif  // MONITER_Util_H_
#pragma once

#include <exception>
#include <string>

namespace constellation {

// strategy check exception
class StrategyCheckException : public std::exception {
 public:
  explicit StrategyCheckException(const std::string& msg) : msg_(msg) {}
  const char* what() const noexcept override {
    return msg_.c_str();
  }

 protected:
  std::string msg_;
};

class TranstopoInvalidError : public StrategyCheckException {
 public:
  explicit TranstopoInvalidError(const std::string& msg)
      : StrategyCheckException("Transtopo is invalid: " + msg) {}
};

class TransTopoNotConsistentError : public TranstopoInvalidError {
 public:
  explicit TransTopoNotConsistentError(const std::string& msg)
      : TranstopoInvalidError("Transtopo is not consistent. " + msg) {}
};

// class NodeNumberError : public TranstopoInvalidError {
//  public:
//   explicit NodeNumberError(const std::string& msg, size_t num, size_t expected)
//       : TranstopoInvalidError(msg) {
//     msg_ += ". The number of nodes is " + std::to_string(num) + " but expected " +
//             std::to_string(expected);
//   }
// };

class NodeNotFoundError : public TranstopoInvalidError {
 public:
  explicit NodeNotFoundError(const std::string& msg, int id) : TranstopoInvalidError(msg) {
    msg_ += ". The node with id " + std::to_string(id) + " is not found";
  }
};

class NodeNotConnectedError : public TranstopoInvalidError {
 public:
  explicit NodeNotConnectedError(const std::string& msg, int id1, int id2)
      : TranstopoInvalidError(msg) {
    msg_ += ". The node with id " + std::to_string(id1) + " and " + std::to_string(id2) +
            " are not connected";
  }
};

}  // namespace constellation
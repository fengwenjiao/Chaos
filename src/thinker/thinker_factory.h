#pragma once

#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <cstring>

namespace constellation {

class ThinkerFactory {
 public:
  using ThinkerCreator = std::function<ConstelThinker*()>;

  // Creates a Thinker instance based on the given name
  static ConstelThinker* CreateThinker(const char* name);

 private:
  // Retrieves the Thinker map
  static const std::unordered_map<std::string, ThinkerCreator>& GetThinkerMap();

  // Initializes the Thinker map
  static std::unordered_map<std::string, ThinkerCreator> InitializeThinkerMap();
};

}  // namespace constellation

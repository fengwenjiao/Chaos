#include "constellation_thinker.h"
#include "thinker_factory.h"
#include "./FAPTEqualConfThinker.h"
#include "./FAPTTimeWeightedConfThinker.h"
#include "./RoundRobinTimeWeightedThinker.h"
#include "./SimpleEqualConfThinker.h"
#include "./SimpleThinker.h"
#include "./SinglePointConfThinker.h"

namespace constellation {

// Creates a Thinker instance based on the given name
ConstelThinker* ThinkerFactory::CreateThinker(const char* name) {
  const auto& it = GetThinkerMap().find(name);
  if (it != GetThinkerMap().end()) {
    return it->second();  // Calls the corresponding creation function
  } else {
    throw std::runtime_error("Unknown thinker name: " + std::string(name));
  }
}

// Retrieves the Thinker map
const std::unordered_map<std::string, ThinkerFactory::ThinkerCreator>&
ThinkerFactory::GetThinkerMap() {
  static const std::unordered_map<std::string, ThinkerCreator> thinker_map = InitializeThinkerMap();
  return thinker_map;
}

// Initializes the Thinker map
std::unordered_map<std::string, ThinkerFactory::ThinkerCreator>
ThinkerFactory::InitializeThinkerMap() {
  return {
      {"SimpleThinker", []() { return new ConstelSimpleThinker(); }},
      {"SimpleEqualConfThinker", []() { return new SimpleEqualConfThinker(); }},
      {"SinglePointConfThinker", []() { return new SinglePointConfThinker(); }},
      {"FAPTEqualConfThinker", []() { return new FAPTEqualConfThinker(); }},
      {"FAPTTimeWeightedConfThinker", []() { return new FAPTTimeWeightedConfThinker(); }},
      {"RoundRobinTimeWeightedThinker", []() { return new RoundRobinTimeWeightedThinker(); }},
  };
}

}  // namespace constellation

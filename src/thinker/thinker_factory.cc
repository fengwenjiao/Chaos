#include "constellation_thinker.h"
#include "thinker_factory.h"

#include "./SimpleThinker.h"
#include "./SinglePointConfThinker.h"

#ifdef CONS_NETWORK_AWARE
#include "./RoundRobinTimeWeightedThinker.h"
#include "./SimpleEqualConfThinker.h"
#include "./FAPTEqualConfThinker.h"
#include "./FAPTTimeWeightedConfThinker.h"
#endif

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
      {"ContelSimpleThinker", []() { return new ConstelSimpleThinker(); }},
      {"SinglePointConfThinker", []() { return new SinglePointConfThinker(); }},
#ifdef CONS_NETWORK_AWARE
      {"SimpleEqualConfThinker", []() { return new SimpleEqualConfThinker(); }},
      {"FAPTEqualConfThinker", []() { return new FAPTEqualConfThinker(); }},
      {"FAPTTimeWeightedConfThinker", []() { return new FAPTTimeWeightedConfThinker(); }},
      {"RoundRobinTimeWeightedThinker", []() { return new RoundRobinTimeWeightedThinker(); }},
#endif
  };
}

}  // namespace constellation

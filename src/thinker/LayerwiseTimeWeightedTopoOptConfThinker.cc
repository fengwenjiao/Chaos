#ifdef CONS_NETWORK_AWARE

#include "./LayerwiseTimeWeightedTopoOptConfThinker.h"
#include "constellation_controller.h"
#include "LayerwiseTimeWeightedTopoOptConfThinker.h"

namespace constellation {
std::shared_ptr<Extra> LayerwiseTimeWeightedTopoOptConfThinker::obtainExtra(
    ConstelController* controller) {
  auto extra = std::make_shared<TranTopoExtra>();
  extra->transtopo = controller->GetGlobalTransTopo();
  return extra;
}

GlobalModelSyncConf
LayerwiseTimeWeightedTopoOptConfThinker::decideModelSyncConf(
    const StrategyRequest& req) {
  // TODO: implement this function
}
}  // namespace constellation

#endif
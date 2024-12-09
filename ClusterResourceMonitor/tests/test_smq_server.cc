#include "smq.h"
#include "base.h"
#include "util.h"
using namespace moniter;

int main() {
  moniter::Smq test("192.168.1.16", 60000);
  test.start_server(0);
  std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  // std::vector<std::string> infos;
  // infos = test.gather_info(kSignalStatic+kSignalDynamic+kSignalBandwidth);
  // LOG("recved info:");
  // for(auto info : infos){
  //     LOG(info);
  // }
  test.set_topo({{1, {2, 3, 4}}, {2, {1, 4}}, {3, {1}}});
  std::unordered_map<int, SmqMeta> infos;
  infos = test.gather_info(kSignalStatic);
  infos = test.gather_info(kSignalDynamic);
  // LOG_WARNING_("-----");
  // for(auto& info : infos){
  //     LOG_WARNING_(info.first);
  //     LOG_WARNING_(nlohmann::json(info.second).dump(4));
  // }
  infos = test.gather_info(kSignalNetwork);
  // LOG_WARNING_("-----");
  // for(auto& info : infos){
  //     LOG_WARNING_(info.first);
  //     LOG_WARNING_(nlohmann::json(info.second).dump(4));
  // }
  infos = test.gather_info(kSignalStatic + kSignalDynamic + kSignalNetwork);
  // LOG_WARNING_("-----");
  // for(auto& info : infos){
  //     LOG_WARNING_(info.first);
  //     LOG_WARNING_(nlohmann::json(info.second).dump(4));
  // }
  infos = test.gather_info(kSignalNetwork);
  // LOG_WARNING_("-----");
  // for(auto& info : infos){
  //     LOG_WARNING_(info.first);
  //     LOG_WARNING_(nlohmann::json(info.second).dump(4));
  // }
  // std::this_thread::sleep_for(std::chrono::milliseconds(20000));
  test.stop_smq();
  return 0;
}

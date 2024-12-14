#include "smq.h"
#include "base.h"
#include "util.h"

#include <iostream>

using namespace moniter;

void print_debug(const std::unordered_map<int, SmqMeta>& infos) {
  std::cout << "Number of entries: " << infos.size() << std::endl;
  for (auto& [id, info] : infos) {
    std::cout << "id: " << id << std::endl;
    std::cout << "Static Info:" << std::endl;
    std::cout << "  CPU Models Size: " << info.static_info.cpu_models.size() << std::endl;
    for (auto& cpu : info.static_info.cpu_models) {
      std::cout << "    CPU Physical ID: " << cpu.physical_id << ", Model Name: " << cpu.model_name << std::endl;
    }
    std::cout << "  GPU Models Size: " << info.static_info.gpu_models.size() << std::endl;
    for (auto& gpu : info.static_info.gpu_models) {
      std::cout << "    GPU Minor Number: " << gpu.minor_number << ", Model Name: " << gpu.model_name 
                << ", Total Memory: " << gpu.gpu_mem_total << std::endl;
    }
    std::cout << "  Total RAM: " << info.static_info.total_ram << std::endl;

    std::cout << "Dynamic Info:" << std::endl;
    std::cout << "  CPU Usage: " << info.dynamic_info.cpu_usage << std::endl;
    std::cout << "  Available RAM: " << info.dynamic_info.available_ram << std::endl;
    for (auto& gpu : info.dynamic_info.gpu_usage) {
      std::cout << "    GPU Minor Number: " << gpu.minor_number << ", GPU Utilization: " << gpu.gpu_util 
                << ", Memory Utilization: " << gpu.mem_util << std::endl;
    }

    std::cout << "Network Info:" << std::endl;
    std::cout << "  Bandwidth Size: " << info.network_info.bandwidth.size() << std::endl;
    for (auto& [id, bandwidth] : info.network_info.bandwidth) {
      std::cout << "    ID: " << id << ", Bandwidth: " << bandwidth << std::endl;
    }
  }
}

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
  print_debug(infos);
  infos = test.gather_info(kSignalDynamic);
  // LOG_WARNING_("-----");
  print_debug(infos);
  infos = test.gather_info(kSignalNetwork);
  // LOG_WARNING_("-----");
  // for(auto& info : infos){
  //     LOG_WARNING_(info.first);
  //     LOG_WARNING_(nlohmann::json(info.second).dump(4));
  // }
  print_debug(infos);
  infos = test.gather_info(kSignalStatic + kSignalDynamic + kSignalNetwork);
  // LOG_WARNING_("-----");
  // for(auto& info : infos){
  //     LOG_WARNING_(info.first);
  //     LOG_WARNING_(nlohmann::json(info.second).dump(4));
  // }
  print_debug(infos);
  infos = test.gather_info(kSignalNetwork);
  // LOG_WARNING_("-----");
  // for(auto& info : infos){
  //     LOG_WARNING_(info.first);
  //     LOG_WARNING_(nlohmann::json(info.second).dump(4));
  // }
  // std::this_thread::sleep_for(std::chrono::milliseconds(20000));
  print_debug(infos);
  test.stop_smq();
  return 0;
}

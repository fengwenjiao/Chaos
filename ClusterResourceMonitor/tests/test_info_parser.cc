#include "info_parser.h"
#include <iostream>
#include "util.h"
using namespace moniter;

nlohmann::json get_static_info() {
  nlohmann::json static_info;

  nlohmann::json cpu_json = moniter::StaticInfo::get_cpu_info();
  nlohmann::json gpu_json = moniter::StaticInfo::get_gpu_info();
  nlohmann::json ram_total_json = moniter::StaticInfo::get_totalram();

  static_info["cpu"] = cpu_json;
  static_info["gpu"] = gpu_json;
  static_info["total_ram"] = ram_total_json;

  return static_info;
}

nlohmann::json get_network_info() {
  nlohmann::json network_info = {{1, 1.2}, {2, 2.3}};

  return network_info;
}

nlohmann::json get_dynamic_info() {
  nlohmann::json dynamic_info;
  nlohmann::json cpu_usage_json = moniter::DynamicInfo::Get().get_cpu_usage();
  nlohmann::json gpu_usage_json = moniter::DynamicInfo::Get().get_gpu_usage();
  nlohmann::json ram_available_json = moniter::DynamicInfo::Get().get_available_ram();

  dynamic_info["cpu_usage"] = cpu_usage_json;
  dynamic_info["gpu_usage"] = gpu_usage_json;
  dynamic_info["available_ram"] = ram_available_json;
  return dynamic_info;
}

int main() {
  nlohmann::json info;
  info["id"] = 1;
  info["dynamic_info"] = get_dynamic_info();
  // info["static_info"] = get_static_info();
  info["network_info"] = get_network_info();

  std::string str = info.dump(4);
  // std::cout<<str<<std::endl;
  LOG_INFO_("info: ");
  auto meta = moniter::convert_to_meta(str);
  LOG_INFO_("info: ");
  std::cout << "ID: " << meta.id << std::endl;

  std::cout << "Dynamic Info:" << std::endl;
  std::cout << "  Available RAM: " << meta.dynamic_info.available_ram << " GB" << std::endl;
  std::cout << "  CPU Usage: " << meta.dynamic_info.cpu_usage << " %" << std::endl;
  std::cout << "  GPU Usage:" << std::endl;
  for (const auto& gpu : meta.dynamic_info.gpu_usage) {
    std::cout << "    Minor Number: " << gpu.minor_number << std::endl;
    std::cout << "    GPU Utilization: " << gpu.gpu_util << " %" << std::endl;
    std::cout << "    Memory Utilization: " << gpu.mem_util << " %" << std::endl;
  }

  std::cout << "Static Info:" << std::endl;
  std::cout << "  CPU Models:" << std::endl;
  for (const auto& cpu : meta.static_info.cpu_models) {
    std::cout << "    Physical ID: " << cpu.physical_id << std::endl;
    std::cout << "    Model Name: " << cpu.model_name << std::endl;
  }
  std::cout << "  GPU Models:" << std::endl;
  for (const auto& gpu : meta.static_info.gpu_models) {
    std::cout << "    Minor Number: " << gpu.minor_number << std::endl;
    std::cout << "    Model Name: " << gpu.model_name << std::endl;
    std::cout << "    Total Memory: " << gpu.gpu_mem_total << " GB" << std::endl;
  }
  std::cout << "  Total RAM: " << meta.static_info.total_ram << " GB" << std::endl;

  std::cout << "Network Info:" << std::endl;
  for (const auto& [interface, bandwidth] : meta.network_info.bandwidth) {
    std::cout << "  Target ID: " << interface << ", Bandwidth: " << bandwidth << " Mbps"
              << std::endl;
  }

  nlohmann::json signal;
  signal["ksignal"] = 1;
  signal["test_targets"] = {{1, {"192.168.1.1", 8080}}, {2, {"192.168.1.2", 9090}}};
  std::string signal_str = signal.dump(0);
  auto signal_meta = moniter::parse_signal(signal_str);
  std::cout << "kSignal: " << signal_meta.ksignal << std::endl;
  for (const auto& [target, test] : signal_meta.test_targets) {
    std::cout << "  Target ID: " << target << ", ip: " << test.first << ",port: " << test.second
              << std::endl;
  }
  return 0;
}

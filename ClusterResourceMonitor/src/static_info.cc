#include "static_info.h"
#include <map>
#include <cstring>
#include "util.h"
namespace moniter {
float StaticInfo::get_totalram() {
  std::string proc_meminfo = Util::open_file("/proc/meminfo");
  std::string total_ram_str = Util::find_value(proc_meminfo, "MemTotal");
  std::string total_ram_str_nounit = Util::remove_unit(total_ram_str);
  if (total_ram_str_nounit.empty()) {
    LOG_WARNING_("get total ram failed");
    return -1;
  }
  unsigned long total_mem_ul = std::stoul(total_ram_str_nounit);
  float total_mem_flt_GB = total_mem_ul * float(1) / 1024 / 1024;
  return total_mem_flt_GB;
}

std::vector<StaticInfo::cpu> StaticInfo::get_cpu_info() {
  std::string proc_cpuinfo = Util::open_file("/proc/cpuinfo");
  size_t pos = 0;
  int physical_id;
  std::string model_name;
  std::map<int, std::string> physical_to_model;
  std::map<std::string, int> cpu_count;
  std::vector<StaticInfo::cpu> cpu_info;

  pos = proc_cpuinfo.find("processor");
  while (pos != std::string::npos) {
    std::string id = Util::find_value(proc_cpuinfo, "physical id", pos);
    physical_id = std::stoi(Util::find_value(proc_cpuinfo, "physical id", pos));
    model_name = Util::find_value(proc_cpuinfo, "model name", pos);
    pos = proc_cpuinfo.find("processor", pos + std::strlen("processor"));
    physical_to_model[physical_id] = model_name;
  }
  for (const auto& entry : physical_to_model) {
    cpu_info.emplace_back(StaticInfo::cpu{entry.first, entry.second});
  }
  return cpu_info;
}

std::vector<StaticInfo::gpu> StaticInfo::get_gpu_info() {
  std::string shell_output = Util::exec("nvidia-smi -q");
  size_t pos = shell_output.find("Driver Version");
  if (pos == std::string::npos) {
    LOG_WARNING_("nvidia-smi execution failed");
    return std::vector<StaticInfo::gpu>();
  }

  int attached_gpus = std::stoi(Util::find_value(shell_output, "Attached GPUs"));

  std::vector<StaticInfo::gpu> gpu_info;
  int minor_number;
  std::string model_name;
  float gpu_mem_total;
  for (int i = 0; i < attached_gpus; ++i) {
    pos = shell_output.find("Product Name", pos);
    model_name = Util::find_value(shell_output, "Product Name", pos);
    minor_number = std::stoi(Util::find_value(shell_output, "Minor Number", pos));
    pos = shell_output.find("FB Memory Usage", pos);
    std::string gpu_mem_total_str = Util::find_value(shell_output, "Total", pos);
    gpu_mem_total = float(stoi(Util::remove_unit(gpu_mem_total_str))) / 1024;
    gpu_info.emplace_back(StaticInfo::gpu{minor_number, model_name, gpu_mem_total});
  }
  return gpu_info;
}

int StaticInfo::get_attached_gpus() {
  std::string shell_output = Util::exec(" nvidia-smi -q -d PIDS");
  size_t pos = shell_output.find("Driver Version");
  if (pos == std::string::npos) {
    LOG_WARNING_("nvidia-smi execution failed");
    return 0;
  }
  int attached_gpus = std::stoi(Util::find_value(shell_output, "Attached GPUs"));
  return attached_gpus;
}

}  // namespace moniter

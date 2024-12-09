#include "dynamic_info.h"
#include <sstream>
#include <unistd.h>
#include "util.h"

namespace moniter {
struct DynamicInfo::cpu_times_stat DynamicInfo::get_cpu_times() {
  std::string proc_stat = Util::open_file("/proc/stat");
  std::string proc_stat_first_line = proc_stat.substr(0, proc_stat.find("/n"));
  std::string times = proc_stat_first_line.substr(proc_stat_first_line.find(" "));
  std::istringstream iss(times);
  unsigned long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
  iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
  unsigned long total_time = user + nice + system + idle + iowait + irq + softirq + steal;
  struct cpu_times_stat cpu_times(total_time, idle);
  return cpu_times;
};

float DynamicInfo::get_cpu_usage() {
  // TODO: improve accuracy
  struct DynamicInfo::cpu_times_stat cpu_stat_1 = get_cpu_times();
  usleep(500);
  struct DynamicInfo::cpu_times_stat cpu_stat_2 = get_cpu_times();
  unsigned long total_diff = cpu_stat_2.total_time - cpu_stat_1.total_time;
  unsigned long idle_diff = cpu_stat_2.idle_time - cpu_stat_1.idle_time;
  float cpu_usage = 100.0 * (total_diff - idle_diff) / total_diff;
  return cpu_usage;
}

std::vector<DynamicInfo::gpu_usage> DynamicInfo::get_gpu_usage() {
  if (attached_gpus_ == -1) {
    attached_gpus_ = StaticInfo::get_attached_gpus();
  }
  if (attached_gpus_ == 0) {
    LOG_WARNING_("Try to get gpu usagem, but no gpu attached");
    return std::vector<DynamicInfo::gpu_usage>();
  }

  std::vector<DynamicInfo::gpu_usage> gpu_usage_;
  for (int i = 0; i < StaticInfo::get_attached_gpus(); ++i) {
    std::string cmd = std::string("nvidia-smi -i ") + std::to_string(i) + " -q -d UTILIZATION";
    std::string shell_output = Util::exec(cmd);
    std::string gpu_util_str = Util::find_value(shell_output, "Gpu");
    float gpu_util = std::stof(Util::remove_unit(gpu_util_str));
    std::string mem_util_str = Util::find_value(shell_output, "Memory");
    float mem_util = std::stof(Util::remove_unit(mem_util_str));
    gpu_usage_.emplace_back(i, gpu_util, mem_util);
  }
  return gpu_usage_;
}

float DynamicInfo::get_available_ram() {
  std::string proc_meminfo = Util::open_file("/proc/meminfo");
  std::string avail_mem_str = Util::find_value(proc_meminfo, "MemAvailable");
  unsigned long avail_mem_ul = std::stoul(Util::remove_unit(avail_mem_str));
  float avail_mem_flt_GB = float(avail_mem_ul) / 1024 / 1024;
  return avail_mem_flt_GB;
}

}  // namespace moniter

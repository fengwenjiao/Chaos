#ifndef MONITER_SMQ_META_H_
#define MONITER_SMQ_META_H_
#include <vector>
#include <unordered_map>
#include "static_info.h"
#include "dynamic_info.h"
#include "util.h"
namespace moniter {
struct DynamicInfoMeta {
  float available_ram;
  float cpu_usage;
  std::vector<DynamicInfo::gpu_usage> gpu_usage;
};

struct StaticInfoMeta {
  std::vector<StaticInfo::cpu> cpu_models;
  std::vector<StaticInfo::gpu> gpu_models;
  float total_ram;
};

struct NetworkInfoMeta {
  std::unordered_map<int, float> bandwidth;

  NetworkInfoMeta& operator+=(const NetworkInfoMeta& other) {
    for (const auto& pair : other.bandwidth) {
      if (this->bandwidth.find(pair.first) != this->bandwidth.end()) {
        LOG_WARNING_("Warning: Key " << pair.first << " already exists. Original value: "
                                     << this->bandwidth[pair.first]
                                     << ", Adding value: " << pair.second);
      }
      this->bandwidth[pair.first] += pair.second;
    }

    return *this;
  }
};

struct SmqMeta {
  int id;
  DynamicInfoMeta dynamic_info;
  StaticInfoMeta static_info;
  NetworkInfoMeta network_info;
};

struct SignalMeta {
  int ksignal;
  int id;
  std::string ip;
  int iperf_port;
  SmqMeta smq_meta;
  std::unordered_map<int, std::pair<std::string, int>> test_targets;
};

}  // namespace moniter
#endif  // MONITER_SMQ_META_H_
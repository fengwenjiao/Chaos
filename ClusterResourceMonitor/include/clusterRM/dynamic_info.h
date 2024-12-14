#ifndef MONITER_DYNAMIC_INFO_H_
#define MONITER_DYNAMIC_INFO_H_
#include "static_info.h"

namespace moniter {

class DynamicInfo {
 public:
  /**
   *\brief get the instance of DynamicInfo
   */
  static DynamicInfo& Get() {
    static DynamicInfo dynamic_info;
    return dynamic_info;
  }

  struct gpu_usage {
    DEFAULT_SPECIAL_MEMBERS(gpu_usage);
    int minor_number;
    float gpu_util;
    float mem_util;
    gpu_usage(int minor_number, float gpu_util, float mem_util)
        : minor_number(minor_number), gpu_util(gpu_util), mem_util(mem_util) {}
  };

  /**
   *\brief get the gpu usage information
   *\return gpu id, gpu utility, gpu memory utility
   */
  std::vector<DynamicInfo::gpu_usage> get_gpu_usage();
  /**
   *\brief get the average cpu utility
   */
  float get_cpu_usage();
  /**
   *\brief get available ram
   *\return available ram size (GB)
   */
  float get_available_ram();

 private:
  struct cpu_times_stat {
    unsigned long total_time;
    unsigned long idle_time;
    cpu_times_stat(unsigned long total_time, unsigned long idle_time)
        : total_time(total_time), idle_time(idle_time) {}
  };

  /** \brief constructor, initialize with */
  DynamicInfo() {}
  /** \brief empty deconstrcutor */
  ~DynamicInfo() {}
  /** \brief get current time cpu stat */
  struct cpu_times_stat get_cpu_times();

  int attached_gpus_ = -1;
};
}  // namespace moniter

#endif  // MONITER_DYNAMIC_INFO_H_

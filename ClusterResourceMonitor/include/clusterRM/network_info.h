#ifndef MONITER_BANDWIDTH_INFO_H_
#define MONITER_BANDWIDTH_INFO_H_
#include <string>
#include <atomic>

#include "../../3rdparty/iperf-cmake/iperf/src/iperf_api.h"

namespace moniter {

class BandwidthInfo {
 public:
  BandwidthInfo() = default;
  ~BandwidthInfo() = default;
  void start_bandwidth_test_server();
  void stop_bandwidth_test_server();
  float test_bandwidth(const std::string& ip, const int port);
  int get_server_port();
  inline bool is_iperf_server_ready() {
    return iperf_server_ready.load();
  }

 private:
  std::atomic<bool> iperf_server_ready{false};
  int iperf_port_ = -1;
  int get_available_port();
  struct iperf_test* create_iperf_client_test(const std::string& ip, const int port);
};
}  // namespace moniter
#endif  // MONITER_BANDWIDTH_INFO_H_
#include "network_info.h"
#include "util.h"
#include <thread>
#include <unistd.h>
#include <chrono>
#include <vector>
using namespace moniter;

int main(int argc, char* argv[]) {
  std::vector<std::thread> threads;
  BandwidthInfo band;
  std::thread iperf_server_thread(
      [&band]() { band.start_bandwidth_test_server(); });
  sleep(2);
  int server_port = band.get_server_port();
  for (int i = 0; i < 10; i++) {
    usleep(200);  // 休眠 200 微秒
    threads.emplace_back([&band, &server_port]() {
      band.test_bandwidth("192.168.1.16", server_port);
    });
  }
  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  band.stop_bandwidth_test_server();
  iperf_server_thread.join();
  return 0;
}
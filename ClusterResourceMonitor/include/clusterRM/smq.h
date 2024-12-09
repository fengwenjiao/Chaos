#ifndef SMQ_H_
#define SMQ_H_
#include <thread>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <sys/epoll.h>
#include <functional>
#include "smq_meta.h"
#include "network_info.h"

namespace moniter {
class Smq {
 public:
  Smq(const char* ip, int port = 12093) {
    server_ip_ = std::string(ip);
    server_port_ = port;
  }
  Smq(const std::string& ip, int port = 12093) {
    server_ip_ = ip;
    server_port_ = port;
  }
  ~Smq() {}
  /**
   * @brief start the smq server thread
   */
  inline void start_server(int global_id) {
    is_server_ = true;
    smq_thread_ =
        std::unique_ptr<std::thread>(new std::thread(std::bind(&Smq::server, this, global_id)));
  }

  /**
   * @brief start the smq client thread
   */
  inline void start_client(int global_id) {
    is_server_ = false;
    smq_thread_ =
        std::unique_ptr<std::thread>(new std::thread(std::bind(&Smq::client, this, global_id)));
  }

  std::unordered_map<int, SmqMeta> gather_info(int ksignal);

  inline void stop_smq() {
    while (!smq_ready_.load())
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    smq_ready_.store(false);
    smq_thread_->join();
  };

  void set_topo(std::unordered_map<int, std::vector<int>> topo);

 private:
  std::unique_ptr<std::thread> smq_thread_;
  std::string server_ip_;
  BandwidthInfo band_;
  int server_port_;
  std::vector<std::vector<std::pair<int, int>>> band_test_groups;
  // socket fd -> client id
  std::unordered_map<int, int> socket_ids_;
  std::unordered_map<int, int> id_sockets_;
  bool is_server_;
  bool is_topo_ready_;
  int id_ = -1;
  std::unordered_map<int, std::vector<int>> topo_;
  std::unordered_map<int, std::pair<std::string, int>> client_iperf_servers_;
  std::atomic<bool> smq_ready_;

  std::mutex tracker_mu_;
  std::condition_variable tracker_cond_;
  std::pair<int, int> tracker_;
  std::unordered_map<int, SmqMeta> recved_info_;
  void server(int global_id);
  void client(int global_id);
  inline void set_id(int global_id) {
    id_ = global_id;
  };
  std::string get_info_with_signal(SignalMeta signal);
  void client_register(int client_fd);
  void server_data_handler(std::vector<char> recv_data);

  int CreateSocket();
  void SetNonBlocking(int sock_fd);
  int InitializeEpoll(int target_fd, struct epoll_event& ev, uint32_t events);
  std::string resolve_hostname(const std::string& server_ip);
  int ReadData(int socket, char* buffer, std::vector<char>& recv_data);
  std::string get_ip_from_socket(int socket);
};
}  // namespace moniter
#endif  // SMQ_H_
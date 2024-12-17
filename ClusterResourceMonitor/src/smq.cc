#include "smq.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <unordered_set>
#include "util.h"
#include "net_thinker.h"  // Ensure NetThinker is included
#include "../../src/utils/serilite.hpp"
#include "base.h"
#include "network_utils.h"

#define MAX_EVENTS  50
#define BUFFER_SIZE 1024

namespace moniter {
void Smq::server(int global_id) {
  set_id(global_id);
  char buffer[BUFFER_SIZE] = {0};

  int server_fd = CreateSocket();

  struct epoll_event ev, events[MAX_EVENTS];

  int epoll_fd = InitializeEpoll(server_fd, ev, EPOLLIN);

  smq_ready_.store(true);

  while (true) {
    int num_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, 100);
    if (!smq_ready_.load())
      break;

    if (num_fds == -1) {
      LOG_ERROR_("epoll_wait() failed");
    }

    for (int n = 0; n < num_fds; ++n) {
      if (events[n].data.fd == server_fd) {
        int new_socket = accept(server_fd, nullptr, nullptr);
        if (new_socket == -1) {
          LOG_ERROR_("Accept new socket failed");
        }

        SetNonBlocking(new_socket);

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = new_socket;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
          LOG_ERROR_("epoll_ctl: new_socket error");
        }

        socket_ids_[new_socket] = -1;
        LOG_INFO_("New connection established");
      } else {
        int client_socket = events[n].data.fd;
        std::vector<char> recv_data;
        int ret = ReadData(client_socket, buffer, recv_data);
        if (ret == 0) {
          LOG_INFO_("Client disconnected");
          socket_ids_.erase(client_socket);
          for (auto& id_socket : id_sockets_) {
            if (id_socket.second == client_socket) {
              id_sockets_.erase(id_socket.first);
              break;
            }
          }
        } else {
          // SignalMeta signal = parse_signal(std::string(recv_data.begin(), recv_data.end()));
          SignalMeta signal;
          try {
            std::string recv_str(recv_data.begin(), recv_data.end());
            constellation::serilite::deserialize(recv_str, signal);
          } catch (const std::exception& e) {
            LOG_WARNING_("Parse signal failed: " << e.what());
            continue;
          }
          if (signal.ksignal == kSignalRegister) {
            if (socket_ids_[client_socket] != -1) {
              LOG_WARNING_("Socket already registered");
              continue;
            }
            if (id_sockets_.find(signal.id) != id_sockets_.end()) {
              LOG_WARNING_("Client id already registered");
              continue;
            }
            if (signal.id == id_) {
              LOG_WARNING_("Client id conflict with server id");
              continue;
            }
            if (signal.ip.empty()) {
              throw std::runtime_error("Client ip is empty");
            }
            client_iperf_servers_[signal.id] = std::make_pair(signal.ip, signal.iperf_port);
            id_sockets_[signal.id] = client_socket;
            socket_ids_[client_socket] = signal.id;
            LOG_INFO_("Client registered: fd:" << client_socket << " id:" << signal.id
                                               << " ip: " << signal.ip
                                               << " iperf port: " << signal.iperf_port);
          } else {
            server_data_handler(recv_data);
            // LOG_INFO_("msg received: " << std::string(nlohmann::json(signal).dump()));
          }
        }
      }
    }
  }

  close(epoll_fd);
  close(server_fd);

  LOG_INFO_("Smq server finalized ");
  return;
}

void Smq::client(int global_id) {
  set_id(global_id);
  char buffer[BUFFER_SIZE] = {0};

  int client_fd = CreateSocket();

  std::thread iperf_server_thread([this] { band_.start_bandwidth_test_server(); });

  while (!band_.is_iperf_server_ready())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  client_register(client_fd);

  smq_ready_.store(true);
  while (smq_ready_.load()) {
    std::vector<char> recv_data;
    int ret = ReadData(client_fd, buffer, recv_data);
    if (ret == 0) {
      LOG_WARNING_("Server disconnected");
      break;
    }
    // SignalMeta signal = parse_signal(std::string(recv_data.begin(), recv_data.end()));
    SignalMeta signal;
    try {
      std::string recv_str(recv_data.begin(), recv_data.end());
      constellation::serilite::deserialize(recv_str, signal);
    } catch (const std::exception& e) {
      LOG_WARNING_("Parse signal failed: " << e.what());
      continue;
    }
    // LOG_INFO_(signal.id << " -> " << id_
    //                     << ":data:" << std::string(recv_data.begin(), recv_data.end()));
    std::string infos = get_info_with_signal(signal);
    send(client_fd, infos.c_str(), infos.length(), 0);
    // LOG_INFO_(id_ << " -> " << signal.id << ":data:" << infos);
  }

  band_.stop_bandwidth_test_server();
  iperf_server_thread.join();
  close(client_fd);

  LOG_INFO_("Smq client finalized ");
  return;
}

std::unordered_map<int, SmqMeta> Smq::gather_info(int ksignal) {
  if ((ksignal & kSignalStatic) || (ksignal & kSignalDynamic) || (ksignal & kSignalNetwork)) {
  } else {
    LOG_WARNING_("Invalid gather info signal");
    return {};
  }
  if (socket_ids_.size() == 0) {
    LOG_WARNING_("No connected client");
    return {};
  }

  recved_info_.clear();

  SignalMeta gather_info_signal;
  gather_info_signal.id = id_;

  int signal = 0;
  if (ksignal & kSignalStatic)
    signal += kSignalStatic;
  if (ksignal & kSignalDynamic)
    signal += kSignalDynamic;

  if (signal != 0) {
    gather_info_signal.ksignal = signal;

    // TODO
    std::string gather_info_msg =
        constellation::serilite::serialize(gather_info_signal).as_string();
    for (auto& client_socket : id_sockets_) {
      send(client_socket.second, gather_info_msg.c_str(), gather_info_msg.length(), 0);
      // LOG_INFO_(id_ << " -> " << client_socket.first << ":signal:" << gather_info_msg);
    }

    std::unique_lock<std::mutex> lk(tracker_mu_);

    tracker_ = std::make_pair(socket_ids_.size(), 0);

    tracker_cond_.wait(lk, [this] { return tracker_.first == tracker_.second; });
  }

  if (ksignal & kSignalNetwork) {
    int band_test_rounds = band_test_groups.size();
    if (is_topo_ready_ == false) {
      LOG_WARNING_("topo is not ready, unale to gather network info");
    } else if (band_test_rounds == 0) {
      LOG_WARNING_("No link to test, unable to gather network info");
    } else {
      gather_info_signal.ksignal = kSignalNetwork;
      for (auto& band_test : band_test_groups) {
        int num = 0;
        for (auto& sigle_test : band_test) {
          gather_info_signal.test_targets.clear();
          int client_id = sigle_test.first;
          int target_id = sigle_test.second;
          if ((id_sockets_.find(client_id) == id_sockets_.end())) {
            LOG_WARNING_("Client:" << client_id << " not registered");
            continue;
          } else if (id_sockets_.find(target_id) == id_sockets_.end()) {
            LOG_WARNING_("Client:" << target_id << " not registered");
            continue;
          }
          int client_socket = id_sockets_[client_id];
          gather_info_signal.test_targets[target_id] = client_iperf_servers_[target_id];
          // TODO:
          //  std::string gather_info_msg = nlohmann::json(gather_info_signal).dump();
          std::string gather_info_msg =
              constellation::serilite::serialize(gather_info_signal).as_string();
          send(client_socket, gather_info_msg.c_str(), gather_info_msg.length(), 0);
          num++;
          // LOG_INFO_(id_ << " -> " << client_id << ":signal:" << gather_info_msg);
        }
        std::unique_lock<std::mutex> lk(tracker_mu_);
        tracker_ = std::make_pair(num, 0);
        tracker_cond_.wait(lk, [this] { return tracker_.first == tracker_.second; });
      }
    }
  }
  return recved_info_;
}

int Smq::CreateSocket() {
  int sock_fd;
  int opt = 1;
  struct sockaddr_in addr;

  server_ip_ = resolve_hostname(server_ip_);
  if (server_ip_.empty()) {
    LOG_ERROR_("Invalid Smq server ip");
  }
  // LOG_INFO_("Smq server ip: "+ server_ip_);

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());
  addr.sin_port = htons(server_port_);

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    LOG_ERROR_("Socket creation failed");
  }

  if (is_server_) {
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
      LOG_ERROR_("setsockopt() failed");
    }

    if (bind(sock_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
      LOG_ERROR_("bind() failed");
    }

    if (listen(sock_fd, 10) < 0) {
      LOG_ERROR_("listen() failed");
    }
    SetNonBlocking(sock_fd);
    LOG_INFO_("Server is listening on port " << server_port_);

  } else {
    if (connect(sock_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
      LOG_ERROR_("Connect to server failed");
    }

    LOG_INFO_("Client connected to server at " << server_ip_ << ":" << server_port_);
  }

  return sock_fd;
}

void Smq::SetNonBlocking(int socket) {
  int flags = fcntl(socket, F_GETFL, 0);
  if (flags == -1) {
    LOG_ERROR_("fcntl(F_GETFL) failed, socket: " << socket << ", errno: " << errno);
  }
  if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0) {
    LOG_ERROR_("fcntl(F_SETFL) failed to set nonblocking, socket: " << socket
                                                                    << ", errno: " << errno);
  }
}

std::string Smq::resolve_hostname(const std::string& server_ip) {
  // 检查 server_ip 是否是有效的 IP 地址
  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, server_ip.c_str(), &(sa.sin_addr));
  if (result == 1) {
    // server_ip 是有效的 IPv4 地址
    return server_ip;
  }

  // 尝试解析 server_ip 作为域名
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;      // IPv4 或 IPv6
  hints.ai_socktype = SOCK_STREAM;  // TCP

  int status = getaddrinfo(server_ip.c_str(), NULL, &hints, &res);
  if (status != 0) {
    std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
    return "";
  }

  // 遍历解析结果，找到第一个有效的 IP 地址
  for (struct addrinfo* p = res; p != NULL; p = p->ai_next) {
    void* addr;
    char ipstr[INET6_ADDRSTRLEN];

    if (p->ai_family == AF_INET) {  // IPv4
      struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
      addr = &(ipv4->sin_addr);
    } else {  // IPv6
      struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
      addr = &(ipv6->sin6_addr);
    }

    inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
    freeaddrinfo(res);
    return std::string(ipstr);
  }

  freeaddrinfo(res);
  return "";
}

int Smq::InitializeEpoll(int target_fd, struct epoll_event& ev, uint32_t events) {
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    LOG_ERROR_("epoll_create1 failed");
  }

  ev.events = events;
  ev.data.fd = target_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, target_fd, &ev) == -1) {
    close(epoll_fd);
    LOG_ERROR_("epoll_ctl: add target_fd failed");
  }

  return epoll_fd;
}

int Smq::ReadData(int socket, char* buffer, std::vector<char>& recv_data) {
  int valread = read(socket, buffer, BUFFER_SIZE);
  if (valread == 0)
    return 0;
  else if (valread > 0) {
    recv_data.insert(recv_data.end(), buffer, buffer + valread);
    // BUG: client can only read once, need to read until no data
    if (is_server_) {
      while ((valread = read(socket, buffer, BUFFER_SIZE)) > 0) {
        LOG_INFO_("Smq reading data...");
        recv_data.insert(recv_data.end(), buffer, buffer + valread);
        if (valread < BUFFER_SIZE) {
          break;
        }
      }
    }
    memset(buffer, 0, BUFFER_SIZE);
    return recv_data.size();
  } else if (valread == -1) {
    LOG_ERROR_("Read data error");
  }
  LOG_WARNING_("Undefined Behavior");

  return -1;
}

void Smq::client_register(int client_fd) {
  int iperf_port = band_.get_server_port();
  SignalMeta register_signal;
  register_signal.ksignal = 0;
  register_signal.id = id_;
  std::string ip, interface;
  ps::GetAvailableInterfaceAndIP(&interface, &ip);
  if (ip.empty()) {
    throw std::runtime_error("Failed to get ip address");
  }
  register_signal.ip = ip;
  register_signal.iperf_port = iperf_port;
  // std::string register_msg = nlohmann::json(register_signal).dump(0);
  std::string register_msg = constellation::serilite::serialize(register_signal).as_string();
  int ret = send(client_fd, register_msg.c_str(), register_msg.length(), 0);
  if (ret == -1) {
    LOG_ERROR_("Send register message failed");
  }
}

std::string Smq::get_info_with_signal(SignalMeta signal) {
  int ksignal = signal.ksignal;
  std::unordered_map<int, std::pair<std::string, int>> test_targets = signal.test_targets;

  SmqMeta smq_meta;
  smq_meta.id = id_;
  if (ksignal & kSignalStatic) {
    smq_meta.static_info.cpu_models = StaticInfo::get_cpu_info();
    smq_meta.static_info.gpu_models = StaticInfo::get_gpu_info();
    smq_meta.static_info.total_ram = StaticInfo::get_totalram();
  }
  if (ksignal & kSignalDynamic) {
    smq_meta.dynamic_info.cpu_usage = DynamicInfo::Get().get_cpu_usage();
    smq_meta.dynamic_info.gpu_usage = DynamicInfo::Get().get_gpu_usage();
    smq_meta.dynamic_info.available_ram = DynamicInfo::Get().get_available_ram();
  }
  if (ksignal & kSignalNetwork) {
    for (const auto& [id, ip_port] : test_targets) {
      smq_meta.network_info.bandwidth[id] = band_.test_bandwidth(ip_port.first, ip_port.second);
    }
  }

  SignalMeta smq_signal;
  smq_signal.ksignal = ksignal;
  smq_signal.id = id_;
  smq_signal.smq_meta = smq_meta;
  // std::string json_string = nlohmann::json(smq_signal).dump();
  std::string json_string = constellation::serilite::serialize(smq_signal).as_string();
  return json_string;
}

std::string Smq::get_ip_from_socket(int client_socket) {
  struct sockaddr_in peer_addr;
  socklen_t peer_addr_len = sizeof(peer_addr);

  if (getpeername(client_socket, (struct sockaddr*)&peer_addr, &peer_addr_len) == -1) {
    LOG_ERROR_("Error getting peer name: ");
    return "";
  }

  char ip_str[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &peer_addr.sin_addr, ip_str, sizeof(ip_str)) == nullptr) {
    LOG_ERROR_("Error converting IP address: ");
    return "";
  }

  return std::string(ip_str);
}

void Smq::set_topo(std::unordered_map<int, std::vector<int>> topo) {
  if (is_server_) {
    topo_ = topo;
    band_test_groups = NetThinker::edge_coloring(topo);
    is_topo_ready_ = true;
  } else {
    LOG_WARNING_("Client can not set topo");
  }
}

void Smq::server_data_handler(std::vector<char> recv_data) {
  // SignalMeta data = parse_signal(std::string(recv_data.begin(), recv_data.end()));
  SignalMeta data;
  try {
    std::string recv_str(recv_data.begin(), recv_data.end());
    constellation::serilite::deserialize(recv_str, data);
  } catch (const std::exception& e) {
    LOG_WARNING_("Parse signal failed: " << e.what());
    return;
  }
  if (data.id == -1) {
    LOG_WARNING_("Invalid data id");
    return;
  } else if (id_sockets_.find(data.id) == id_sockets_.end()) {
    LOG_WARNING_("Client id not registered");
    return;
  }
  if (recved_info_.find(data.id) == recved_info_.end()) {
    recved_info_[data.id] = data.smq_meta;
  } else {
    if (data.ksignal & kSignalNetwork) {
      recved_info_[data.id].network_info += data.smq_meta.network_info;
    } else {
      LOG_WARNING_("Duplicated data");
      return;
    }
  }
  std::unique_lock<std::mutex> lk(tracker_mu_);
  tracker_.second++;
  tracker_cond_.notify_one();
}

}  // namespace moniter

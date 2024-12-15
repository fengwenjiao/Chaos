#include "network_info.h"
#include "util.h"
#include <thread>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>

#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>

namespace moniter {

namespace iperf {
static jmp_buf sigend_jmp_buf;

static void __attribute__((noreturn)) sigend_handler(int sig) {
  longjmp(sigend_jmp_buf, 1);
}
}  // namespace iperf

void BandwidthInfo::start_bandwidth_test_server() {
  if (iperf_server_ready.load()) {
    LOG_WARNING_("iperf server already started");
    return;
  }
  iperf_port_ = get_available_port();
  LOG_INFO_("iperf server start at port " << iperf_port_);
  iperf_server_ready.store(true);

  pid_t pid = fork();

  if (pid < 0) {
    throw std::runtime_error("fork failed");
  } else if (pid == 0) {
    // child process
    // start iperf server, from iperf/src/main.c
    ::iperf_test* test;
    test = iperf_new_test();
    if (test == NULL) {
      LOG_ERROR_("iperf new test failed");
      return;
    }
    iperf_defaults(test);
    iperf_set_test_role(test, 's');
    iperf_set_test_server_port(test, iperf_port_);
    // iperf_set_test_one_off(test, 1);
    iperf_set_verbose(test, 0);
    iperf_set_test_omit(test, 0);
    iperf_set_test_logfile(test, "/dev/null");

    /* Termination signals. */
    iperf_catch_sigend(iperf::sigend_handler);
    if (setjmp(iperf::sigend_jmp_buf))
      iperf_got_sigend(test);

    /* Ignore SIGPIPE to simplify error handling */
    signal(SIGPIPE, SIG_IGN);

    if (iperf_create_pidfile(test) < 0) {
      i_errno = IEPIDFILE;
      iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
    }
    for (;;) {
      int rc;
      rc = iperf_run_server(test);
      test->server_last_run_rc = rc;
      if (rc < 0) {
        iperf_err(test, "error - %s", iperf_strerror(i_errno));
        // if (test->json_output) {
        //   if (iperf_json_finish(test) < 0)
        //     return;
        // }
        iflush(test);

        if (rc < -1) {
          iperf_errexit(test, "exiting");
        }
      }
      iperf_reset_test(test);
      if (iperf_get_test_one_off(test) && rc != 2) {
        /* Authentication failure doesn't count for 1-off test */
        if (rc < 0 && i_errno == IEAUTHTEST) {
          continue;
        }
        break;
      }
    }
    iperf_delete_pidfile(test);

    // reset signal handler
    iperf_catch_sigend(SIG_DFL);
    signal(SIGPIPE, SIG_DFL);

  } else {
    // parent process
    pid_ = static_cast<int>(pid);
  }
}

void BandwidthInfo::stop_bandwidth_test_server() {
  if (!iperf_server_ready.load()) {
    LOG_WARNING_("iperf server not started");
    return;
  }
  iperf_server_ready.store(false);
  if (pid_) {
    kill(pid_, SIGTERM);
  }
  waitpid(pid_, NULL, 0);
}

float BandwidthInfo::test_bandwidth(const std::string& ip, const int port) {
  struct iperf_test* test = create_iperf_client_test(ip, port);

  int max_retry = 10;
  int count = 0;
  while (iperf_run_client(test) < 0) {
    iperf_free_test(test);
    test = create_iperf_client_test(ip, port);
    if (count >= max_retry) {
      LOG_WARNING_("Failed to test bandwidth " << iperf_strerror(i_errno) << "");
      iperf_free_test(test);
      return -1;
    }
    count++;

    LOG_WARNING_("Error when iperf test with: " << ip << ":" << port << "   "
                                                << iperf_strerror(i_errno) << ", retrying...");
    sleep(2);
  }
  char* json_output = iperf_get_test_json_output_string(test);
  if (json_output == NULL) {
    LOG_WARNING_("failed to get JSON output");
    iperf_free_test(test);
    return -1;
  }
  std::string json_str(json_output);

  size_t pos = json_str.find("sum_received");
  std::string bandwidth = moniter::Util::find_value(json_str, "bits_per_second", pos);
  float band = std::stof(bandwidth);
  LOG_INFO_("test with: " << ip << " port:" << port << ", bandwidth " << band / (1000 * 1000)
                          << "Mbps");
  iperf_free_test(test);
  return band;
}

int BandwidthInfo::get_available_port() {
  struct sockaddr_in addr;
  addr.sin_port = htons(0);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (0 != bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) {
    LOG_ERROR_("bind port " << addr.sin_port << " failed");
    return 0;
  }

  socklen_t addr_len = sizeof(addr);

  if (0 != getsockname(sock, (struct sockaddr*)&addr, &addr_len)) {
    LOG_ERROR_("getsockname failed");
    return 0;
  }

  int ret_port = ntohs(addr.sin_port);
  close(sock);
  return ret_port;
}

int BandwidthInfo::get_server_port() {
  if (iperf_server_ready.load()) {
    return iperf_port_;
  } else {
    LOG_WARNING_("iperf server not started");
    return -1;
  }
}

struct iperf_test* BandwidthInfo::create_iperf_client_test(const std::string& ip, const int port) {
  struct iperf_test* test;

  test = iperf_new_test();
  if (test == NULL) {
    LOG_ERROR_("Failed to create iperf client test\n");
  }

  // configure iperf
  iperf_defaults(test);
  iperf_set_test_role(test, 'c');
  iperf_set_test_server_hostname(test, const_cast<char*>(ip.c_str()));
  iperf_set_test_server_port(test, port);
  iperf_set_test_duration(test, 1);
  iperf_set_test_json_output(test, 1);
  iperf_set_verbose(test, 0);
  iperf_set_test_zerocopy(test, 1);
  iperf_set_test_logfile(test, "/dev/null");

  // using iperf_set_test_rate() may cause iperf test unable to finish
  if ((ip == std::string("127.0.0.1")) && (port == iperf_port_)) {
    // unsigned int local_test_bandwidth_limit = 1280000000;
    // iperf_set_test_rate(test, local_test_bandwidth_limit);
    LOG_INFO_("iperf finalize test");
  }

  return test;
}
}  // namespace moniter
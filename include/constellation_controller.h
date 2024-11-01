#ifndef _CONSTELLATION_CONTROLLER_H_
#define _CONSTELLATION_CONTROLLER_H_

#include <functional>

#include "./constellation_transtopothinker.h"
#include "internal/topo_graph.hpp"
#include "ps/ps.h"

namespace constellation {

class ReadyNodeOverlayManager {
 public:
  ReadyNodeOverlayManager() : is_asycn_add_(false), is_first_reach_init_num_{false} {}
  bool HandleNodeReady(int node_id);
  inline bool ShouldGetNewTransTopo() const {
    return is_asycn_add_;
  }
  inline bool isFristReachInitNum() const {
    return is_first_reach_init_num_;
  }
  AdjacencyList GetReadyOverlayStr();

 private:
  TopoGraph<int> ready_nodes_;
  bool is_asycn_add_;  // 0: sync join stage, 1: async join stage
  bool is_first_reach_init_num_;
};

class ConstelController {
 public:
  explicit ConstelController(ConstelThinker* thinker = nullptr) {
    ps::StartAsync(0, "ConstelController\0");
    using namespace std::placeholders;
    ps_scheduler_ = new ps::Controller(0);
    ps_scheduler_->set_request_handle(std::bind(&ConstelController::RequestHandle, this, _1, _2));
    ps_scheduler_->set_response_handle(std::bind(&ConstelController::ResponseHandle, this, _1, _2));

    if (thinker == nullptr) {
      thinker_ = new ConstelTransTopoThinker();
    } else {
      thinker_ = thinker;
    }
  }

  ~ConstelController() {
    ps::Finalize(0, false);
    delete ps_scheduler_;
    delete thinker_;
  }

  void run() {
    if (thinker_ == nullptr) {
      LOG(INFO) << "Thinker is not set, use default thinker";
      thinker_ = new ConstelTransTopoThinker();
    }
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1000));
    }
  }

 private:
  /**
   * \brief Transform the ModelLoadAssignment to GlobalModelSyncConf
   */
  GlobalModelSyncConf ModelSycnConfTransform(int target_id,
                                             ModelLoadAssignment model_load_assignment);
  /**
   * \brief Controller handle for all received request
   */
  void RequestHandle(const ps::SimpleData& recved, ps::SimpleApp* app);
  /**
   * \brief Controller handle for all received response
   */
  void ResponseHandle(const ps::SimpleData& recved, ps::SimpleApp* app);
  /**
   * \brief Controller handle for signal from scheduler
   */
  void SchedulerSignalHandle(const ps::SimpleData& recved, ps::SimpleApp* app);
  /**
   * \brief Controller return a proper future timsestamp
   */
  uint32_t GetFutureTimtestamp();
  /**
   * \brief send message to all trainers
   */
  void SendToALLTrainers(int head, const std::string& body);
  /**
   * \brief send message to some trainer
   */
  void SendToTrainer(int head, const std::string& body, int recv_id);

  std::unordered_map<int, uint64_t> model_params_dist_;
  uint64_t model_params_total_ = 0;

  ReadyNodeOverlayManager node_manager_;

  ScaleClock clock_;

  ps::Controller* ps_scheduler_;

  ConstelThinker* thinker_;

  bool is_sycn_add_finished_ = false;
};

}  // namespace constellation
#endif  // _CONSTELLATION_CONTROLLER_H_
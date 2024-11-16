#ifndef _CONSTELLATION_CONTROLLER_H_
#define _CONSTELLATION_CONTROLLER_H_

#include <functional>

#include "./constellation_transtopothinker.h"
#include "ps/ps.h"

namespace moniter {
class Smq;
}  // namespace moniter

namespace constellation {

class ReadyNodeOverlayManager;

class ConstelController {
 public:
  explicit ConstelController(ConstelThinker* thinker = nullptr);

  ~ConstelController();

  void run();

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

  std::shared_ptr<ReadyNodeOverlayManager> node_manager_;

  ScaleClock clock_;

  ps::Controller* ps_scheduler_;

  ConstelThinker* thinker_;

  bool is_sycn_add_finished_ = false;

#ifdef CONS_NETWORK_AWARE
  moniter::Smq* test_server_;
#endif
};

}  // namespace constellation
#endif  // _CONSTELLATION_CONTROLLER_H_
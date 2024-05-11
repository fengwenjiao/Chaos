#ifndef _CONSTELLATION_CONTROLLER_H_
#define _CONSTELLATION_CONTROLLER_H_

#include <functional>

#include <ps/ps.h>

namespace constellation {
class ConstelController {
 public:
  explicit ConstelController() {
    using namespace std::placeholders;
    ps_scheduler_ = new ps::KVServer<char>(0);
    static_cast<ps::SimpleApp*>(ps_scheduler_)
        ->set_request_handle(std::bind(&ConstelController::RequestHandle, this, _1, _2));
    static_cast<ps::SimpleApp*>(ps_scheduler_)
        ->set_response_handle(std::bind(&ConstelController::ResponseHandle, this, _1, _2));
  }
  ~ConstelController(){
    delete ps_scheduler_;
  }

 private:
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
   * \brief Controller send topo to strategy layer and get new transtopo
   */
  std::unordered_map<:int,std::pair<int,std::vector<int>>> SetTranstopo(std::unordered_map<int,std::pair<int,std::vector<int>>>);
  /**
   * \brief Controller return a proper future timestamp
   */
  int GetFutureTimtestamp();
  /**
   * \brief send message to all trainers
   */
  void SendToALLTrainers();
  
  int timestamp_ = 0;
  int future_timestamp_ = 0;
  ps::KVServer<char>* ps_scheduler_;
  set<int> ready_nodes_;//zzh: set stores the node in ready
  int addnode_stage_ = 0//zzh: 0 denotes sync add,1 denotes async add
};

}  // namespace constellation
#endif  // _CONSTELLATION_CONTROLLER_H_
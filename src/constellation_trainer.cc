#include "constellation_trainer.h"

namespace constellation {

void ConstelTrainer::Init(int key, const CArray<char>* val) {
  // send ready signal to controller
  int head = static_cast<int>(kControllerSignal::kNodeReadySignal);
  int my_id = ps::Postoffice::Get()->GetMyID();
  std::string body = std::to_string(my_id);
  trainer_->Wait(trainer_->Request(head, body, ps::kScheduler));
  // wait until receive the transtopo from controller
  while (!is_ctx_ready_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  // TODO :model init
}

void ConstelTrainer::Push(int key, const CArray<char>& val) {}

void ConstelTrainer::Pull(int key, CArray<char>* val) {}

void ConstelTrainer::RequestHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {
  int sender = recved.sender;
  kControllerSignal signal = static_cast<kControllerSignal>(recved.head);
  const std::string& body = recved.body;
  switch (signal) {
    // case : kControllerSignal::xxxx:
    //     break;
    case kControllerSignal::kUpdateTransTopoAnouce: {
      // recv new transtopo, set it to clock
      ScaleClock::Tick tick;
      tick.Decode(body);
      auto fut_timestamp = tick.timestamp;
      auto& transtopo = tick.transtopo;
      if (fut_timestamp == 0) {
        // sycn add stage, update right now
        int my_id = ps::Postoffice::Get()->GetMyID();
        auto it = transtopo.find(my_id);
        CHECK(it != transtopo.end()) << "Node " << my_id << " is not in the transtopo";
        // update Postoffice transtopo
        auto& local_transtopo = it->second;
        ps::Postoffice::Get()->UpdateLocalTrans(local_transtopo.getParent(),
                                                local_transtopo.getChildren());
        this->is_scale_ = false;
        // notify main thread
        this->is_ctx_ready_.store(true);
      } else {
        // async add stage, set alarm
        clock_.setAlarm(std::move(tick));
      }
      break;
    }
    default:
      LOG(FATAL) << "Unkown signal ";
      break;
  };
  app->Response(recved);
}

void ConstelTrainer::DataHandle(const ps::KVMeta& req_meta,
                                const ps::KVPairs<char>& req_data,
                                ps::KVTrainer<char>* trainer) {
  // pass
}

}  // namespace constellation
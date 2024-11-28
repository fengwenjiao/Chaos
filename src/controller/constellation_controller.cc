#include "constellation_controller.h"
#include "dmlc/logging.h"

#include "../utils/serilite.hpp"
#include "../overlay/topo_graph.hpp"
#include "../overlay/node_overlay_manager.h"
#include "../thinker/SimpleThinker.h"
#include "../thinker/thinker_factory.h"
#if CONS_NETWORK_AWARE
#include "clusterRM/smq.h"
#include "../overlay/network_aware/network_aware.h"
#endif

#include <cmath>
#include <algorithm>

namespace constellation {
ConstelController::ConstelController() {
  ps::StartAsync(0, "ConstelController\0");
  using namespace std::placeholders;
  ps_scheduler_ = new ps::Controller(0);
  ps_scheduler_->set_request_handle(std::bind(&ConstelController::RequestHandle, this, _1, _2));
  ps_scheduler_->set_response_handle(std::bind(&ConstelController::ResponseHandle, this, _1, _2));
  thinker_ = nullptr;

  // create node manager
#ifdef CONS_NETWORK_AWARE
  test_server_ = std::make_unique<moniter::Smq>();
  node_manager_ = std::make_shared<aware::NetworkAwareNodeManager>(test_server_);
#else
  node_manager_ = std::make_shared<ReadyNodeOverlayManager>();
#endif
}

ConstelController::ConstelController(ConstelThinker* thinker) : ConstelController() {
  setThinker(thinker);
}
ConstelController::ConstelController(const char* thinker_name) : ConstelController() {
  setThinker(thinker_name);
}
void ConstelController::setThinker(const char* thinker_name) {
  auto* thinker = ThinkerFactory::CreateThinker(thinker_name);
  setThinker(thinker);
}

ConstelController::~ConstelController() {
  ps::Finalize(0, false);
  delete ps_scheduler_;
  delete thinker_;
}

void ConstelController::run() {
  if (thinker_ == nullptr) {
    LOG(INFO) << "Thinker is not set, use default thinker";
    setThinker("ContelSimpleThinker");
  }
  CHECK(thinker_ != nullptr) << "Thinker is failed to create";
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1000));
  }
}

void ConstelController::RequestHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {
  kControllerSignal signal = static_cast<kControllerSignal>(recved.head);
  const std::string& body = recved.body;
  switch (signal) {
    case kControllerSignal::kNodeReadySignal: {
      ReadySignalBody ready_signal_body;
      serilite::deserialize(body, ready_signal_body);
      int ready_node_id = ready_signal_body.id;
      // TODO: handle keys and lens
      if (!node_manager_->HandleNodeReady(ready_node_id) ||
          !node_manager_->ShouldGetNewTransTopo()) {
        // duplicated node ready signal
        break;
      }
      if (ready_signal_body.need_sycn_model) {
        CHECK(ready_signal_body.keys.size() > 0) << "keys size should be greater than 0";
        int priv = ready_signal_body.keys[0];
        for (int i = 0; i < ready_signal_body.keys.size(); i++) {
          const auto key = ready_signal_body.keys[i];
          const auto& len = ready_signal_body.lens[i];
          if (i != 0) {
            CHECK_EQ(key, priv + 1)
                << "keys should be continuous" << "priv: " << priv << " key: " << key;
            priv = key;
          }
          if (thinker_->getParamsSize(key) == 0) {
            thinker_->setParamsDist(key, len);
          } else if (thinker_->getParamsSize(key) != len) {
            LOG(WARNING) << "Key: " << key << " has different length: " << len
                         << " from previous length: " << thinker_->getParamsSize(key);
          }
        }
      }
      // get new transtopo
      // TODO: now it is just a simple version, need to be improved
      // TODO: if need remote thinker, here should just send, can not get the result immediately
      auto overlay = node_manager_->GetReadyOverlay();
      ModelSycnConf model_sync_conf;
      model_sync_conf.target_node_id = {ready_node_id};
      auto req = StrategyRequest{StrategyRequest::StrategyReqType::kTopoAndModelSyncConfUpdate,
                                 {ready_node_id},
                                 std::move(overlay),
                                 nullptr};
      if (!ready_signal_body.need_sycn_model || node_manager_->isFristReachInitNum()) {
        req.type = StrategyRequest::StrategyReqType::kTopoUpdateOnly;
      }
      auto strategy_block = thinker_->GenerateStrategy(req);
      auto& transtopo = strategy_block.global_topo_;
      auto& global_model_sync_conf = strategy_block.global_model_sync_conf_;

      // Decide a new future timestamp
      uint32_t future_timestamp = GetFutureTimtestamp();

      std::unordered_map<int, std::string> data;
      ScaleClock::Tick tick;
      tick.timestamp = future_timestamp;
      GlobalTransTopo topo;
      for (const auto& [id, related_topo] : transtopo) {
        // send to all trainers in the topo
        // TODO: may need to notify the trainer that is not in the topo
        topo.clear();
        topo[id] = related_topo;  // only one node in the topo
        tick.transtopo = std::move(topo);
        if (global_model_sync_conf.find(id) != global_model_sync_conf.end()) {
          auto& model_sync_conf = global_model_sync_conf[id];
          tick.model_sync_conf = std::move(model_sync_conf);
        }
        auto serialized_tick = serilite::serialize(tick);
        auto str = serialized_tick.as_string();
        data.emplace(std::make_pair(id, str));
        PS_VLOG(2) << "Send new tick to node: " << id << " with tick: " << tick.debug_string();
      }
      int head = static_cast<int>(kControllerSignal::kUpdateTransTopoAnouce);
      // send to all trainers and wait for response, should not wait!(dead lock)
      app->Request(head, data);
      // set alarm for the new future timestamp
      tick.transtopo = transtopo;
      tick.model_sync_conf = {};
      clock_.setAlarm(std::move(tick));
      break;
    }
    case kControllerSignal::kUpdateClockSignal: {
      // body: node_id, timestamp
      auto it = body.find(",");
      int node_id = std::stoi(body.substr(0, it));
      uint32_t timestamp = std::stoi(body.substr(it + 1));

      this->clock_.clockTick();
      clock_.unlock();
      if (timestamp != this->clock_.getLocalTimestamp()) {
        LOG(WARNING) << "Controller received timestamp: " << timestamp
                     << " but local timestamp is: " << this->clock_.getLocalTimestamp();
        // this->clock_.local_timestamp_ = timestamp;
      }
      break;
    }

    default:
      LOG(WARNING) << "Controller received unknown signal from scheduler";
      break;
  }
  app->Response(recved);
}

void ConstelController::ResponseHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {}

void ConstelController::SchedulerSignalHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {}

uint32_t ConstelController::GetFutureTimtestamp() {
  if (!is_sycn_add_finished_) {
    is_sycn_add_finished_ = true;
    return 0;
  }
  uint32_t future_timestamp = clock_.getLocalTimestamp();
  return future_timestamp + 3;
}

}  // namespace constellation
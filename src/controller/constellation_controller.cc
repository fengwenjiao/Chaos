#include "constellation_controller.h"
#include "dmlc/logging.h"

#include "../utils/serilite.hpp"
#include "../overlay/topo_graph.hpp"
#include "../overlay/node_overlay_manager.h"
#include "../thinker/SimpleThinker.h"
#include "./threadsafe_queue.hpp"
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
  ps_scheduler_->set_request_handle(
      std::bind(&ConstelController::RequestHandle, this, _1, _2));
  ps_scheduler_->set_response_handle(
      std::bind(&ConstelController::ResponseHandle, this, _1, _2));
  thinker_ = nullptr;

  batch_t_window_ = std::make_shared<WindowedBuffer<int64_t>>(5);
  rtt_window_ = std::make_shared<WindowedBuffer<int64_t>>(5);

  // create node manager
#ifdef CONS_NETWORK_AWARE
  test_server_ =
      std::make_unique<moniter::Smq>(ps::Postoffice::Get()->getSchedulerHost());
  node_manager_ =
      std::make_shared<aware::NetworkAwareNodeManager>(test_server_);
#else
  node_manager_ = std::make_shared<ReadyNodeOverlayManager>();
#endif
}

ConstelController::ConstelController(ConstelThinker* thinker)
    : ConstelController() {
  setThinker(thinker);
}
ConstelController::ConstelController(const char* thinker_name)
    : ConstelController() {
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

  main_task_queue_ = std::make_shared<ThreadSafeQueue<MainTask>>();
  while (true) {
    auto task = main_task_queue_->pop();
    if (task) {
      task();
    } else {
      break;
    }
  }
}

void ConstelController::RequestHandle(const ps::SimpleData& recved,
                                      ps::SimpleApp* app) {
  kControllerSignal signal = static_cast<kControllerSignal>(recved.head);
  const std::string& body = recved.body;
  std::string response_body;
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
        CHECK(ready_signal_body.keys.size() > 0)
            << "keys size should be greater than 0";
        int priv = ready_signal_body.keys[0];
        for (int i = 0; i < ready_signal_body.keys.size(); i++) {
          const auto key = ready_signal_body.keys[i];
          const auto& len = ready_signal_body.lens[i];
          if (i != 0) {
            CHECK_EQ(key, priv + 1) << "keys should be continuous"
                                    << "priv: " << priv << " key: " << key;
            priv = key;
          }
          if (thinker_->getParamsSize(key) == 0) {
            thinker_->setParamsDist(key, len);
          } else if (thinker_->getParamsSize(key) != len) {
            LOG(WARNING) << "Key: " << key << " has different length: " << len
                         << " from previous length: "
                         << thinker_->getParamsSize(key);
          }
        }
      }
      PS_VLOG(2) << "model total size: " << thinker_->getParamsTotal();
      // get new transtopo
      // TODO: now it is just a simple version, need to be improved
      // TODO: if need remote thinker, here should just send, can not get the
      // result immediately
      auto overlay = node_manager_->GetReadyOverlay();
      PS_VLOG(2) << "Get new overlay: " << overlay->debug_string();
      ModelSycnConf model_sync_conf;
      model_sync_conf.target_node_id = {ready_node_id};
      auto extra = thinker_->obtainExtra(this);
      auto req = StrategyRequest{
          StrategyRequest::StrategyReqType::kTopoAndModelSyncConfUpdate,
          {ready_node_id},
          std::move(overlay),
          extra};
      if (!ready_signal_body.need_sycn_model ||
          node_manager_->isFristReachInitNum()) {
        req.type = StrategyRequest::StrategyReqType::kTopoUpdateOnly;
      }

      StrategyBlock strategy_block;
      try {
        strategy_block = thinker_->GenerateStrategy(req);
      } catch (StrategyCheckException& e) {
        LOG(FATAL) << e.what();
        break;
      }
      auto maintask = [app, strategy_block, this]() {
        auto& transtopo = strategy_block.global_topo_;
        auto& global_model_sync_conf = strategy_block.global_model_sync_conf_;
        // Decide a new future timestamp
        uint32_t future_timestamp = this->GetFutureTimtestamp();

        std::unordered_map<int, std::string> data;
        ScaleClock::Tick tick;
        tick.timestamp = future_timestamp;

        // update controller's tick to record the topo
        if (future_timestamp == 0) {
          global_transtopo_ = transtopo;
        } else {
          tick.transtopo = transtopo;
          this->clock_.setAlarm(tick);
        }

        GlobalTransTopo topo;
        for (const auto& [id, related_topo] : transtopo) {
          // send to all trainers in the topo
          // TODO: may need to notify the trainer that is not in the topo
          topo.clear();
          topo[id] = related_topo;  // only one node in the topo
          tick.transtopo = std::move(topo);
          if (global_model_sync_conf.find(id) != global_model_sync_conf.end()) {
            const auto& model_sync_conf = global_model_sync_conf.at(id);
            tick.model_sync_conf = std::move(model_sync_conf);
          } else {
            tick.model_sync_conf.Clear();
          }
          auto serialized_tick = serilite::serialize(tick);
          auto str = serialized_tick.as_string();
          data.emplace(std::make_pair(id, str));
          PS_VLOG(2) << "Send new tick to node: " << id
                     << " with tick: " << tick.debug_string();
        }
        int head = static_cast<int>(kControllerSignal::kUpdateTransTopoAnouce);
        // send to all trainers and wait for response, should not wait!(dead
        // lock)
        app->Request(head, data);
        // set alarm for the new future timestamp
        tick.transtopo = transtopo;
        tick.model_sync_conf = {};
      };
      main_task_queue_->push(std::move(maintask));
      break;
    }
    case kControllerSignal::kUpdateClockSignal: {
      ClockSignalBody clock_signal_body;
      serilite::deserialize(body, clock_signal_body);
      uint32_t timestamp = clock_signal_body.timestamp;

      // batch time record
      if (clock_signal_body.batch_dur_ms > 0) {
        batch_t_window_->push_back(clock_signal_body.batch_dur_ms);
      }
      // delay record
      if (clock_signal_body.rtt > 0) {
        rtt_window_->push_back(clock_signal_body.rtt);
      }
      response_body = serilite::serialize(clock_signal_body).as_string();

      auto ticked = clock_.clockTick(timestamp);

      if (ticked) {
        // remove the tick
        global_transtopo_ = ticked->transtopo;
        clock_.removeTickNow();
      }
      break;
    }

    default:
      LOG(WARNING) << "Controller received unknown signal from scheduler";
      break;
  }
  app->Response(recved, response_body);
}

void ConstelController::ResponseHandle(const ps::SimpleData& recved,
                                       ps::SimpleApp* app) {
  kControllerSignal signal = static_cast<kControllerSignal>(recved.head);
  const std::string& body = recved.body;
  if (signal != kControllerSignal::kQueryTimestamp) {
    return;
  }
  ClockSignalBody sig_body;
  serilite::deserialize(body, sig_body);
  if (sig_body.batch_dur_ms > 0) {
    batch_t_window_->push_back(sig_body.batch_dur_ms);
  }
  if (sig_body.rtt > 0) {
    rtt_window_->push_back(sig_body.rtt);
  }
  if (sig_body.tp_snd > 0) {
    auto now = get_time_point("us");
    auto rtt = now - sig_body.tp_snd;
    rtt_window_->push_back(rtt);
  }
  PS_VLOG(2) << "Update clock signal from node: " << sig_body.node_id
             << " with signal: " << sig_body.debug_string();
  clock_.clockTick(sig_body.timestamp);
}

void ConstelController::SchedulerSignalHandle(const ps::SimpleData& recved,
                                              ps::SimpleApp* app) {}

uint32_t ConstelController::GetFutureTimtestamp() {
  if (!is_sycn_add_finished_) {
    is_sycn_add_finished_ = true;
    return 0;
  }
  uint32_t future_timestamp = QueryTimestamp();
  auto batch_t = batch_t_window_->get_avg();
  if (batch_t == 0) {
    return future_timestamp + 15;
  }
  auto batch_us = batch_t * 1000.0;  // us
  auto rtt_us = rtt_window_->get_avg() * 1.2;
  auto ensure_us = std::max(rtt_us, 10000.0);  // 10ms
  rtt_us += ensure_us;
  uint32_t dt =
      static_cast<uint32_t>(std::ceil(rtt_us / batch_us)) + 2;
  return future_timestamp + dt;
}

int ConstelController::findRoot() const {
  for (const auto& [id, topo] : global_transtopo_) {
    if (id == 1)
      continue;
    if (topo.getType() == NodeTransTopo::Type::kRoot) {
      return id;
    }
  }
  return -1;
}

uint32_t ConstelController::QueryTimestamp(int id) {
  if (id == 0) {
    id = findRoot();
    PS_VLOG(2) << "Find root node: " << id;
    if (id == -1) {
      LOG(FATAL) << "Can not find the root node";
    }
  }
  ClockSignalBody body{0, 0, 0, 0, get_time_point("us")};
  ps_scheduler_->Wait(ps_scheduler_->Request(
      static_cast<int>(kControllerSignal::kQueryTimestamp),
      serilite::serialize(body).as_string(),
      id));
  PS_VLOG(2) << "Query timestamp from node: " << id;
  return clock_.getLocalTimestamp();
}

}  // namespace constellation
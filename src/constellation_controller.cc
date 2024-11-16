#include "constellation_controller.h"
#include "dmlc/logging.h"
#include "./internal/serilite.hpp"
#include "topo_graph.hpp"
#include "node_overlay_manager.h"

#if CONS_NETWORK_AWARE
#include "clusterRM/smq.h"
#include "./network_aware.h"
#endif

#include <cmath>
#include <algorithm>

namespace constellation {

ConstelController::ConstelController(ConstelThinker* thinker) {
  node_manager_ = std::make_shared<ReadyNodeOverlayManager>();
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
#if CONS_NETWORK_AWARE
  test_server_ = new moniter::Smq();
  test_server_->start_server();
#endif
}
ConstelController::~ConstelController() {
  ps::Finalize(0, false);
  delete ps_scheduler_;
  delete thinker_;

#if CONS_NETWORK_AWARE
  delete test_server_;
#endif
}

void ConstelController::run() {
  if (thinker_ == nullptr) {
    LOG(INFO) << "Thinker is not set, use default thinker";
    thinker_ = new ConstelTransTopoThinker();
  }
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1000));
  }
}

GlobalModelSyncConf ConstelController::ModelSycnConfTransform(
    int target_id,
    ModelLoadAssignment model_load_assignment) {
  GlobalModelSyncConf global_model_sync_conf;
  model_load_assignment.normalize();
  model_load_assignment.groupByFirstNode();
  int key = 0;
  uint64_t tot_len = 0;
  uint64_t cur_len = 0;
  int full_key_begin = -1;
  int full_key_end = -1;
  uint64_t piece;
  for (size_t i = 0; i < model_load_assignment.paths.size(); i++) {
    const auto& path = model_load_assignment.getPath(i);
    const int& node = path[0];
    const float& load = model_load_assignment.loads[i];
    uint64_t slice_len = (i < model_load_assignment.paths.size() - 1) ?
                             std::ceil(load * model_params_total_) :
                             model_params_total_ - tot_len;
    auto& model_sc_ = global_model_sync_conf[node];
    model_sc_.target_node_id.push_back(target_id);
    model_sc_.paths.emplace_back(path);
    model_sc_.kvslices.emplace_back();
    if (cur_len > 0) {
      piece = std::min(slice_len, model_params_dist_[key] - cur_len);
      model_sc_.kvslices.back().emplace_back(key, cur_len, piece);
      cur_len += piece;
      cur_len %= model_params_dist_[key];
      tot_len += piece;
      slice_len -= piece;
      if (cur_len == 0 && !std_isin(++key, model_params_dist_)) {
        CHECK_EQ(slice_len, 0);
        break;
      }
    }
    full_key_begin = key;
    while (std_isin(key, model_params_dist_) && slice_len >= model_params_dist_[key]) {
      auto lent = model_params_dist_[key++];
      slice_len -= lent;
      tot_len += lent;
    }
    full_key_end = key;
    if (full_key_end > full_key_begin) {
      model_sc_.kvslices.back().emplace_back(full_key_begin, full_key_end);
    }
    if (slice_len > 0) {
      model_sc_.kvslices.back().emplace_back(key, 0, slice_len);
      cur_len = slice_len;
      tot_len += slice_len;
    }
  }

  return global_model_sync_conf;
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
        for (int i = 0; i < ready_signal_body.keys.size(); i++) {
          const auto& key = ready_signal_body.keys[i];
          const auto& len = ready_signal_body.lens[i];
          if (model_params_dist_.find(key) == model_params_dist_.end()) {
            model_params_dist_.emplace(std::make_pair(key, len));
            model_params_total_ += len;
            PS_VLOG(3) << "Key: " << key << " has length: " << len;
          } else if (model_params_dist_[key] != len) {
            LOG(WARNING) << "Key: " << key << " has different length: " << len
                         << " from previous length: " << model_params_dist_[key];
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
      auto& strategy_block = thinker_->GenerateStrategy(req);
      auto& transtopo = strategy_block.global_topo_;
      auto& model_load_assignment = strategy_block.model_load_assignment_;
      auto global_model_sync_conf = ModelSycnConfTransform(ready_node_id, model_load_assignment);

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
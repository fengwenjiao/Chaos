#include "constellation_controller.h"
#include "dmlc/logging.h"

namespace constellation {

void ConstelController::RequestHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {
  kControllerSignal signal = static_cast<kControllerSignal>(recved.head);
  const std::string& body = recved.body;
  switch (signal) {
    // case : kControllerSignal::xxxx:
    //     break;
    case kControllerSignal::kNodeReadySignal: {
      int ready_node_id = stoi(body);
      if (!node_manager_.HandleNodeReady(ready_node_id)) {
        // duplicated node ready signal
        break;
      }
      if (!node_manager_.ShouldGetNewTransTopo()) {
        break;
      }
      // get new transtopo
      // TODO: now it is just a simple version, need to be improved
      // TODO: if need remote thinker, here should just send, can not get the result immediately
      AdjacencyList overlay = node_manager_.GetReadyOverlayStr();
      GlobalTransTopo transtopo = this->thinker_->SendOverlay(overlay);
      // Decide a new future timestamp
      uint32_t future_timestamp = GetFutureTimtestamp();

      std::unordered_map<int, std::string> data;
      ScaleClock::Tick tick;
      tick.timestamp = future_timestamp;
      GlobalTransTopo topo;
      for (const auto& entry : transtopo) {
        // send to all trainers in the topo
        // TODO: may need to notify the trainer that is not in the topo
        topo.clear();
        topo[entry.first] = entry.second;  // only one node in the topo
        tick.transtopo = std::move(topo);
        std::string str;
        tick.Encode(&str);
        data[entry.first] = str;
      }
      int head = static_cast<int>(kControllerSignal::kUpdateTransTopoAnouce);
      // send to all trainers and wait for response, should not wait!(dead lock)
      app->Request(head, data);
      // set alarm for the new future timestamp
      tick.transtopo = std::move(transtopo);
      clock_.setAlarm(std::move(tick));
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
  if(!is_sycn_add_finished_){
    is_sycn_add_finished_ = true;
    return 0;
  }
  uint32_t future_timestamp = clock_.getLocalTimestamp();
  return future_timestamp + 5;
}

std::string ConstelController::SerializeTransTopo(int timestamp,
                                                  const std::pair<int, std::vector<int>>& data) {
  std::ostringstream oss;
  oss << timestamp << "|" << data.first << ":";
  for (size_t i = 0; i < data.second.size(); ++i) {
    oss << data.second[i];
    if (i < data.second.size() - 1) {
      oss << ",";
    }
  }
  return oss.str();
}
bool ConstelController::DeserializeTransTopo(const std::string& serialized,
                                             int& timestamp,
                                             std::pair<int, std::vector<int>>& data) {
  std::istringstream iss(serialized);
  std::string temp;

  if (!std::getline(iss, temp, '|'))
    return false;
  timestamp = std::stoi(temp);

  if (!std::getline(iss, temp, ':'))
    return false;
  data.first = std::stoi(temp);

  data.second.clear();
  while (std::getline(iss, temp, ',')) {
    data.second.push_back(std::stoi(temp));
  }

  return true;
}
}  // namespace constellation
#include "constellation_controller.h"
#include "dmlc/logging.h"
#include "./topo_graph.hpp"

namespace constellation {

void ConstelController::RequestHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {
  int sender = recved.sender;
  // request from schduler
  if (sender == ps::kScheduler) {
    SchedulerSignalHandle(recved, app);
    return;
  }
}

void ConstelController::ResponseHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {}

void ConstelController::SchedulerSignalHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {
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
      int future_timestamp = GetFutureTimtestamp();

      std::unordered_map<int, std::string> data;
      for (const auto& entry : transtopo) {
        int node_id = entry.first;
        data[node_id] = entry.second.Encode();
      }
      int head = static_cast<int>(kControllerSignal::kUpdateTransTopoAnouce);
      // send to all trainers and wait for response
      app->Wait(app->Request(head, data));
      // TODO:update the transtopo, modify tick to contain global topo instead of node topoF
      break;
    }

    default:
      LOG(WARNING) << "Controller received unknown signal from scheduler";
      break;

      
  }
  app->Response(recved);
}

int ConstelController::GetFutureTimtestamp() {
  int future_timestamp = clock_.getLocalTimestamp();
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
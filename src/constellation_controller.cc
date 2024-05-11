#include "constellation_controller.h"
#include "dmlc/logging.h"

namespace constellation {
using ps::kControllerSignal;

void ConstelController::RequestHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {
    int sender = recved.sender;
    // request from schduler
    if(sender==ps::kScheduler) {
        SchedulerSignalHandle(recved, app);
        return;
    }
    
}
void ConstelController::ResponseHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {
}

void ConstelController::SchedulerSignalHandle(const ps::SimpleData& recved, ps::SimpleApp* app){
    kControllerSignal signal = static_cast<kControllerSignal>(recved.head);
    switch(signal){
        //case : kControllerSignal::xxxx:
        //    break;

        //NOTE:
        case kNodeReadySignal:
            int ready_node_id = stoi(recved.body);//NOTE body should be the new ndoe id
            //sync join stage
            if(addnode_stage_ == 0){
                if(!ready_nodes_.count(ready_node_id)){//whether it is a new node
                    ready_nodes_.insert(ready_node_id);
                    //whether sync nodes all ready
                    //NOTE: Ensure whether it works(get environment variable)
                    if(ready_nodes.size()==atoi(CHECK_NOTNULL(Environment::Get()->find("DMLC_PS_ROOT_PORT")))){
                        //Set Trans topo means report the overlay and stategy layter return transtopo
                        std::unordered_map<int, std::pair<int, std::vector<int>>> new_topo_  = SetTranstopo(GetReadyOverlay());
                        if(new_topo_.size() != 0){
                            for(const auto& entry :new_topo_){
                                int node_id_ = entry.first;
                                int topo_ = entry.second;
                                int head = UPDATETRANSTOPOSIGNAL;//TODO define the signal
                                int future_timestamp = GetFutureTimtestamp();
                                int body = SerializeTransTopo(future_timestamp, topo_);
                                SendToTrainer(head, body, node_id_);
                            }
                        }
                        addnode_stage_ = 1;
                    }
                }
            }
            //async join stage
            if(addnode_stage_ == 1){
                //when new node ready, report
                if(!ready_nodes_.count(ready_node_id))ready_nodes_.insert(ready_node_id);
                std::unordered_map<int, std::pair<int, std::vector<int>>> new_topo_  = SetTranstopo(GetReadyOverlay());
                if(new_topo_.size() != 0){
                    for(const auto& entry :new_topo_){
                        int node_id_ = entry.first;
                        int topo_ = entry.second;
                        int head = UPDATETRANSTOPOSIGNAL;//TODO define the signal
                        int future_timestamp = GetFutureTimtestamp();
                        int body = SerializeTransTopo(future_timestamp, topo_);
                        SendToTrainer(head, body, node_id_);
                    }
                }
            }

        default:
            LOG(WARNING) << "Controller received unknown signal from scheduler";
    }
}

std::unordered_map<int,std::pair<int,std::vector<int>>> ConstelController::SetTranstopo(std::unordered_map<int, std::vector<int>> overlay){
    std::unordered_map<int, std::pair<int, std::vector<int>>> transtopo_;
    if (overlay.empty()) {
        return transtopo_;
    }
    std::queue<int> q;
    std::unordered_map<int, bool> visited;

    int root = overlay.begin()->first;
    q.push(root);
    visited[root] = true;
    transtopo_[root] = { -1, std::vector<int>() };

    while (!q.empty()) {
        int current = q.front();
        q.pop();
        for (int neighbor : overlay[current]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                q.push(neighbor);
                transtopo_[current].second.push_back(neighbor);
                transtopo_[neighbor] = { current, std::vector<int>() };
            }
        }
    }

    global_transtopo = transtopo_;
    return transtopo_;
}

int ConstelController::GetReadyOverlay(){
    overlay = Postoffice::Get()->GetOverlay();
    for (auto it = overlay.begin(); it != overlay.end();) {
        if (!ready_nodes_.count(node->first)) {
            it = overlay.erase(it); 
        } else {
            ++it;
        }
    }
    return overlay;
}

int ConstelController::GetFutureTimtestamp(){
    future_timestamp_ = timestamp_ + 5;
    return future_timestamp_;
}
void ConstelController::SendToTrainer(int head, const std::string& body, int recv_id){
    Message msg;
    msg.meta.app_id = obj_->app_id();
    msg.meta.customer_id = obj_->customer_id();
    msg.meta.request     = true;
    msg.meta.head        = head; 
    msg.meta.sender      = kscheduler;
    msg.meta.recver      = recv_id;
    Postoffice::Get()->van()->Send(msg);
}

void ConstelController::SendToALLTrainers(int head, const std::string& body){
    for(const auto & ready_node_ : ready_nodes_){
        SendToTrainer(head,body,ready_node_);
    }
}

std::string ConstelController::SerializeTransTopo(int timestamp, const std::pair<int, std::vector<int>>& data) {
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
bool ConstelController::DeserializeTransTopo(const std::string& serialized, int& timestamp, std::pair<int, std::vector<int>>& data) {
    std::istringstream iss(serialized);
    std::string temp;
    
    if (!std::getline(iss, temp, '|')) return false;
    timestamp = std::stoi(temp);

    if (!std::getline(iss, temp, ':')) return false;
    data.first = std::stoi(temp);

    data.second.clear();
    while (std::getline(iss, temp, ',')) {
        data.second.push_back(std::stoi(temp));
    }
    
    return true;
}
}  // namespace constellation
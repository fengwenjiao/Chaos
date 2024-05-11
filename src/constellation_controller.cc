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
                        std::unordered_map<int, pair<std::int, std::vector<int>>> new_topo_ = SetTranstopo(Postoffice::Get()->GetOverlay());
                        if(new_topo_.size() != 0){
                            for(const auto& entry :new_topo_){
                                int node_id_ = entry.first;
                                int topo_ = entry.second;//NOTE how to transfer the topo？
                                Message msg;
                                msg.meta.app_id = obj_->app_id();
                                msg.meta.customer_id = obj_->customer_id();
                                msg.meta.body = ;//insert the encoeded transtoopo
                                msg.meta.request     = true;//NOTE: not sure
                                msg.meta.head        = cmd; //NOTE head define
                                msg.meta.future_timestamp   = GetFutureTimtestamp();
                                msg.meta.sender      = kscheduler;//NOTE:controller as sender 
                                msg.meta.recver      = node_id_;
                                Postoffice::Get()->van()->Send(msg);
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
                std::unordered_map<int, std::pair<int, std::vector<int>>> new_topo_  = SetTranstopo(Postoffice::Get()->GetOverlay());
                if(new_topo_.size() != 0){
                    for(const auto& entry :new_topo_){
                        int node_id_ = entry.first;
                        int topo_ = entry.second;//NOTE how to transfer the topo？
                        Message msg;
                        msg.meta.app_id = obj_->app_id();
                        msg.meta.customer_id = obj_->customer_id();
                        msg.meta.request     = true;//NOTE: not sure
                        msg.meta.head        = cmd; //NOTE head define
                        msg.meta.future_timestamp   = GetFutureTimtestamp();
                        msg.meta.sender      = kscheduler;//NOTE:controller as sender 
                        msg.meta.recver      = node_id_;
                        Postoffice::Get()->van()->Send(msg);
                    }
                }
            }

        default:
            LOG(WARNING) << "Controller received unknown signal from scheduler";
    }
}

std::unordered_map<int,std::pair<int,std::vector<int>>> ConstelController::SetTranstopo(std::unordered_map<int,std::pair<int,std::vector<int>>> overlay){
    return overlay;
}

int ConstelController::GetFutureTimtestamp(){
    future_timestamp_ = timestamp_ + 5;
    return future_timestamp_;
}
Void sendtosingletrainer//NOTE
void ConstelController::SendToALLTrainers(cmd,string){

    }
}  // namespace constellation
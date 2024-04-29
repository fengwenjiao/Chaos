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
        default:
            LOG(WARNING) << "Controller received unknown signal from scheduler";
    }
}
}  // namespace constellation
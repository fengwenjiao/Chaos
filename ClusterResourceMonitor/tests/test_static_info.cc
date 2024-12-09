#include "static_info.h"
#include "util.h"
using namespace moniter;

// void get_cpu_info(StaticInfo info){
//     std::vector<std::pair<std::string, int>> cpu_model_count;
//     cpu_model_count = info.get_cpu_model_and_num();
//     // int num_processors =info.get_processors_num();
// }

// void get_mem_info(StaticInfo info){
//     unsigned long total_ram;
//     total_ram = info.get_totalram();
// }
// void get_gpu_info(StaticInfo info){
//     std::vector<std::tuple<int, std::string, std::string >> gpu_info;
//     gpu_info = info.get_gpu_model_and_num();
// }

int main(int argc, char *argv[]) {
    StaticInfo::get_totalram();
    StaticInfo::get_cpu_info();
    StaticInfo::get_gpu_info();
    StaticInfo::get_attached_gpus();
    
    LOG_INFO_("total ram: " + std::to_string(StaticInfo::get_totalram()) + "GB");
    LOG_INFO_("attached gpus: " + std::to_string(StaticInfo::get_attached_gpus()));
    for (auto &cpu : StaticInfo::get_cpu_info()) {
        LOG_INFO_("cpu: " + std::to_string(cpu.physical_id) + " " + cpu.model_name);
    }
    for (auto &gpu : StaticInfo::get_gpu_info()) {
        LOG_INFO_("gpu: " + std::to_string(gpu.minor_number) + " " + gpu.model_name + " " + std::to_string(gpu.gpu_mem_total)+"GB");
    }
    return 0;
}

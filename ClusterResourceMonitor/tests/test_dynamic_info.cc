#include "dynamic_info.h"
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
    DynamicInfo::Get().get_cpu_usage();
    DynamicInfo::Get().get_gpu_usage();
    DynamicInfo::Get().get_available_ram();
    LOG_INFO_("cpu usage: " + std::to_string(DynamicInfo::Get().get_cpu_usage()) + "%");
    LOG_INFO_("available ram: " + std::to_string(DynamicInfo::Get().get_available_ram()) + "GB");
    for(auto &gpu : DynamicInfo::Get().get_gpu_usage()){
        LOG_INFO_("gpu: " + std::to_string(gpu.minor_number) + " " + std::to_string(gpu.gpu_util) + "% " + std::to_string(gpu.mem_util) + "%");
    }
    return 0;
}

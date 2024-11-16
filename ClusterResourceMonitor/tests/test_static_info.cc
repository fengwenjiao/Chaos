#include "moniter.h"
#include "smq.h"
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
    
    // StaticInfo info;
    // int memunit;
    // get_cpu_info(info);
    // get_mem_info(info);
    // get_gpu_info(info);

    // memunit = info.get_memunit();
    // printf("%d \n",memunit);
    // unsigned long totalram = info.get_totalram();
    // printf("%lu \n",totalram);
    // // do nothing
    return 0;
}

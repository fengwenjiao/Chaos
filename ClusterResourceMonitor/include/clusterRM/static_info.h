#ifndef MONITER_STATIC_INFO_H_
#define MONITER_STATIC_INFO_H_

#include <vector>
#include <map>
#include <cstring>
#include <string>

namespace moniter{

class StaticInfo{
public:
    struct cpu{
        int physical_id;
        std::string model_name;
        cpu() = default;
        cpu(int physical_id,std::string model_name):model_name(model_name), physical_id(physical_id) {}
    };
    struct gpu{
        int minor_number;
        std::string model_name;
        float gpu_mem_total;
        gpu() = default;
        gpu( int minor_number, std::string model_name, float gpu_mem_total):model_name(model_name), minor_number(minor_number), gpu_mem_total(gpu_mem_total) {}
    };

    /**
     *\brief get total ram
     *\return total ram size (GB)
     */
    static float get_totalram();
    /**
     *\brief get the cpu information
     *\return cpu id, cpu model
     */
    static std::vector<cpu> get_cpu_info();
    /**
     *\brief get the gpu information
     *\return gpu id, gpu model, gpu memory size(GB)
     */
    static std::vector<gpu> get_gpu_info();
    /**
     *\brief get number of gpus 
     */
    static int get_attached_gpus();
};

} // namespace moniter


#endif // _MONITER_STATIC_INFO_H_


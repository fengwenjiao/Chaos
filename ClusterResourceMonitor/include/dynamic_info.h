#ifndef MONITER_DYNAMIC_INFO_H_
#define MONITER_DYNAMIC_INFO_H_
#include "static_info.h"
#include <fcntl.h>
#include <unistd.h>
#include <atomic>
#include <thread>
// #include "3rdparty/iperf-cmake/iperf/src/iperf_api.h"
#include "iperf_api.h"
namespace moniter{



class DynamicInfo{
public:
    
    /**
     *\brief get the instance of DynamicInfo
     */
    static DynamicInfo& Get(){
        static DynamicInfo dynamic_info;
        return dynamic_info;
    }

    struct cpu_times_stat{
        unsigned long total_time;
        unsigned long idle_time;
        cpu_times_stat(unsigned long total_time,unsigned long idle_time):total_time(total_time),idle_time(idle_time){}
    };

    struct gpu_usage{
        int minor_number;
        float gpu_util;
        float mem_util;
        gpu_usage() = default;
        gpu_usage(int minor_number,float gpu_util,float mem_util):minor_number(minor_number),gpu_util(gpu_util),mem_util(mem_util){}
    };

    /**
     *\brief get total ram
     *\return total ram size (GB)
     */
    float get_totalram_(){
        if(!total_ram){
             total_ram = StaticInfo::get_totalram();
        }
        return total_ram;
    }
    /**
     *\brief get the gpu information
     *\return gpu id, gpu model, gpu memory size(GB)
     */
    std::vector<StaticInfo::gpu> get_gpu_info_(){
        if(gpu_info.empty()){
             gpu_info= StaticInfo::get_gpu_info();
        }
        return gpu_info;
    }
    /**
     *\brief get the cpu information
     *\return cpu id, cpu model
     */
    std::vector<StaticInfo::cpu> get_cpu_info_(){
        if(cpu_info.empty()){
             cpu_info= StaticInfo::get_cpu_info();
        }
        return cpu_info;
    }
    /**
     *\brief get the number of available gpu
     */
    int get_attached_gpus_(){
        if(!attached_gpus){
             attached_gpus= StaticInfo::get_attached_gpus();
        }
        return attached_gpus;
    }
    /**
     *\brief get the gpu usage information
     *\return gpu id, gpu utility, gpu memory utility
     */
    std::vector<DynamicInfo::gpu_usage> get_gpu_usage();
    /**
     *\brief get the average cpu utility
     */
    float get_cpu_usage();
    /**
     *\brief get bandwidth with some node with iperf command
     *\param ip IP address of the node to test bandwith with 
     *\return bandwidth (Mbps)
     */
    float get_bandwidth(const std::string& ip);
    /**
     *\brief test bandwidth with some node with lib iperf
     *\param ip IP address of the node to test bandwith with 
     *\return bandwidth (Mbps)
     */
    float test_bandwidth(const std::string& ip);
    /**
     * \brief start bandwidth test server(iperf server)
     */
    void start_bandwidth_test_server();
    
    inline bool is_server_ready(){
        return server_ready;
    }
    /**
     * \brief stop bandwidth test server(iperf server)
     */
    void stop_bandwidth_test_server();
    /**
     *\brief get available ram
     *\return available ram size (GB)
     */
    float get_available_ram();
private:
    /** \brief constructor, initialize with */
    DynamicInfo(){
        // cpu_info = StaticInfo::get_cpu_info();
       
        // gpu_info = StaticInfo::get_gpu_info();
        // attached_gpus = gpu_info.size();
    }
    /** \brief empty deconstrcutor */
    ~DynamicInfo(){}
    /** \brief get current time cpu stat */
    struct cpu_times_stat get_cpu_times();
    std::vector<StaticInfo::cpu> cpu_info;
    float total_ram ;
    std::vector<StaticInfo::gpu> gpu_info;
    int attached_gpus;
    std::atomic<bool> server_ready{false};

};
} // namespace moniter


#endif //MONITER_DYNAMIC_INFO_H_


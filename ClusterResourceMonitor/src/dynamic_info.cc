#include "dynamic_info.h"


namespace moniter{
struct DynamicInfo::cpu_times_stat DynamicInfo::get_cpu_times(){
    std::string proc_stat = Util::open_file("/proc/stat");
    std::string proc_stat_first_line = proc_stat.substr(0,proc_stat.find("/n"));
    std::string times = proc_stat_first_line.substr(proc_stat_first_line.find(" "));
    std::istringstream iss(times);
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
    unsigned long  total_time =  user + nice + system + idle + iowait + irq + softirq + steal;
    struct cpu_times_stat cpu_times(total_time, idle);
    return cpu_times;
};

float DynamicInfo::get_cpu_usage(){
    struct DynamicInfo::cpu_times_stat cpu_stat_1 = get_cpu_times();
    sleep(1);
    struct DynamicInfo::cpu_times_stat cpu_stat_2 = get_cpu_times();
    unsigned long total_diff =  cpu_stat_2.total_time - cpu_stat_1.total_time;
    unsigned long idle_diff =  cpu_stat_2.idle_time - cpu_stat_1.idle_time;
    float cpu_usage =100.0*(total_diff - idle_diff)/total_diff;
#if DEBUG
LOG("cpu_usage: "<< cpu_usage << " %");
//std::cout <<"[dynamic_info.cc:"<<__LINE__<<"]"<< "cpu_usage: "<< cpu_usage << " %"<< std::endl;
#endif
    return cpu_usage;
}

std::vector<DynamicInfo::gpu_usage> DynamicInfo::get_gpu_usage(){
    std::vector<DynamicInfo::gpu_usage> gpu_usage_;
    for(int i=0; i< DynamicInfo::Get().get_attached_gpus_(); ++i){
        std::string cmd = std::string("nvidia-smi -i ") + std::to_string(i) + " -q -d UTILIZATION";
        std::string shell_output = Util::exec(cmd);
        std::string gpu_util_str = Util::find_value(shell_output, "Gpu"); 
        float gpu_util = std::stof(Util::remove_unit(gpu_util_str));  
        std::string mem_util_str = Util::find_value(shell_output, "Memory");
        float mem_util = std::stof(Util::remove_unit(mem_util_str));  
        gpu_usage_.emplace_back(i, gpu_util, mem_util);
    }
#if DEBUG
    LOG("gpu_usage: ");
    // std::cout <<"[dynamic.cc:"<<__LINE__<<"]"<< "gpu_usage: "<< std::endl;
    for (const auto& entry : gpu_usage_) {
        std::cout <<  "minor_number: " << entry.minor_number <<"\t\tgpu_util: "<< entry.gpu_util <<" %\t\tmem_util: "<<entry. mem_util<<" % "<<std::endl;
    }
#endif
    return gpu_usage_;
}

float DynamicInfo::get_bandwidth(const std::string& ip){
    std::string cmd = std::string("iperf3 -c ") + ip + std::string(" -J");
    std::string shell_output = Util::exec(cmd);
    size_t pos = shell_output.find("sum_received");
    std::string bandwidth_ = Util::find_value(shell_output, "bits_per_second", pos);
    float bandwidth = std::stof(bandwidth_)/1024/1024;
    // size_t start_pos = shell_output.find("bits_per_second", pos);
    // size_t end_pos = shell_output.find_first_not_of("0123456789", start_pos);
    // std::string bandwidth = shell_output.substr(start_pos, end_pos-start_pos);

#if DEBUG
    LOG("Bandwith with "<< ip<<": "<< bandwidth<< "Mbits");
    // std::cout <<"[dynamic.cc:"<<__LINE__<<"]"<<"Bandwith with "<<ip<<": "<< bandwidth<< "Mbits"<< std::endl;
#endif
}

float DynamicInfo::test_bandwidth(const std::string& ip){
    struct iperf_test *test;
    // struct iperf_stream *stream; 

    test = iperf_new_test();
    if (test == NULL) {
        fprintf(stderr, "Failed to create test\n");
        return -1;
    }

    //configure iperf
    iperf_defaults(test);
    iperf_set_test_role(test, 'c');
    iperf_set_test_server_hostname(test, const_cast<char*>(ip.c_str()));
    iperf_set_test_duration(test, 1);
    iperf_set_test_json_output(test, 1);
    iperf_set_verbose(test, 0);
    iperf_set_test_zerocopy(test, 1);
    iperf_set_test_logfile(test, "/dev/null");

    // if(ip == std::string("127.0.0.1")){
    //     unsigned int local_test_bandwidth_limit = 128000;
    //     iperf_set_test_rate(test, local_test_bandwidth_limit);
    //     LOG("test with: "<< ip<<", Max rate:"<<local_test_bandwidth_limit/(1000*1000)<<"Mbps");
    //     // std::cout <<"[dynamic.cc:"<<__LINE__<<"]"<< "test with: "<< ip<<", Max rate:"<<local_test_bandwidth_limit/(1000*1000)<<"Mbps"<<std::endl;
    // }
    
    if (iperf_run_client(test) < 0) {
        fprintf(stderr, "Error in client: %s\n", iperf_strerror(i_errno));
        iperf_free_test(test);
        return -1;
    }

    char *json_output = iperf_get_test_json_output_string(test);
    if (json_output == NULL) {
        std::cerr << "failed to get JSON output" << std::endl;
        iperf_free_test(test);
        return -1;
    }
    std::string json_str(json_output);

    size_t pos = json_str.find("sum_received");
    std::string bandwidth = moniter::Util::find_value( json_str, "bits_per_second",pos);
    float band = std::stof(bandwidth);
    LOG("test with: "<< ip << ", bandwidth "<< band/(1000*1000) <<"Mbps");
    //std::cout <<"[dynamic.cc:"<<__LINE__<<"]"<<"test with: "<< ip << ", bandwidth "<< band/(1000*1000) <<"Mbps"<<std::endl;
    iperf_free_test(test);
    LOG("iperf_free");
    // std::cout <<"[dynamic.cc:"<<__LINE__<<"]"<<"iperf_free"<<std::endl;
    return band;
}

void DynamicInfo::start_bandwidth_test_server(){

/* ----------------old version-----------*/
// #if DEBUG
//     std::cout <<"[dynamic.cc:"<<__LINE__<<"]"<< "version: "<< iperf_get_iperf_version() <<std::endl;
// #endif
//     struct iperf_test *test = iperf_new_test();
//     if (test == NULL) {
//         fprintf(stderr, "Failed to create test\n");
//         return ;
//     }
//     iperf_defaults(test); 
//     iperf_set_test_role(test, 's');
//     iperf_set_test_one_off(test, 1);
//     iperf_set_verbose(test, 0);
//     server_ready = true;
//     while (server_ready.load()) {
// #if DEBUG
//     std::cout <<"[dynamic.cc:"<<__LINE__<<"]"<< "server_ready: "<< server_ready <<std::endl;
// #endif
//         if (iperf_run_server(test) < 0) { 
//             std::cerr << "Error running iperf server: " << iperf_strerror(i_errno) << std::endl;
//             iperf_free_test(test);
//             return;
//         }
//     }
//     iperf_free_test(test);
//     return ;

    // Ideally, it is not necessaty to free iperf every time test finished
    // However, iperf will be stuck if test with localhost for serveral times in a short period of time
    server_ready.store(true);
    do{
        struct iperf_test *test = iperf_new_test();
        if (test == NULL) {
            fprintf(stderr, "Failed to  create test\n");
            return ;
        }
        iperf_defaults(test); 
        iperf_set_test_role(test, 's');
        iperf_set_test_one_off(test, 1);
        iperf_set_verbose(test, 0);
        iperf_set_test_omit(test,0);
        iperf_set_test_logfile(test, "/dev/null");

        if (iperf_run_server(test) < 0) {  
            std::cerr << "Error running iperf server: " << iperf_strerror(i_errno) << std::endl;
            iperf_free_test(test);
            return;
        }
        iperf_free_test(test);
    }while(server_ready.load());
    LOG("iperf server stop");
    //std::cout <<"[dynamic.cc:"<<__LINE__<<"]"<<"iperf server stop"<<std::endl;
    return;

}

float DynamicInfo::get_available_ram(){
    std::string proc_meminfo = Util::open_file("/proc/meminfo");
    std::string avail_mem_str = Util::find_value(proc_meminfo, "MemAvailable");
    unsigned long avail_mem_ul = std::stoul(Util::remove_unit(avail_mem_str));
    float avail_mem_flt_GB = float(avail_mem_ul)/1024/1024;
#if DEBUG
    LOG("available_ram: "<< avail_mem_flt_GB<< " GB");
    //std::cout <<"[static_info.cc:"<<__LINE__<<"]"<< "available_ram: "<< avail_mem_flt_GB<< " GB"<< std::endl;
#endif
    return avail_mem_flt_GB;
    }

void DynamicInfo::stop_bandwidth_test_server(){
    server_ready = false;
#if DEBUG
    LOG("server_ready: "<< server_ready);
    //std::cout <<"[dynamic.cc:"<<__LINE__<<"]"<< "server_ready: "<< server_ready <<std::endl;
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    test_bandwidth("127.0.0.1");
    return ;
}
} // namespace moniter



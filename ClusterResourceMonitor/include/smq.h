#ifndef SMQ_H_
#define SMQ_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <cstdio>
#include <fcntl.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <utility> 

#include "json.hpp"

#include "base.h"
#include "dynamic_info.h"


#define SERVER_IP "192.168.1.16"
#define SERVER_PORT 13011
#define MAX_EVENTS 50
#define BUFFER_SIZE 1024





namespace moniter{

struct DynamicInfoMeta{
    float available_ram;
    float cpu_usage;
    std::vector<DynamicInfo::gpu_usage> gpu_usage;
};

struct StaticInfoMeta{
    std::vector<StaticInfo::cpu> cpu_models;
    std::vector<StaticInfo::gpu> gpu_models;
    float total_ram;
};

struct NetworkInfoMeta{
    std::unordered_map<std::string, float> bandwidth;
};

struct SmqMeta{
    int id;
    DynamicInfoMeta dynamic_info;
    StaticInfoMeta static_info;
    NetworkInfoMeta network_info;
};


class Smq{
    public:
        Smq(){}
        ~Smq(){}
        /**
         * @brief start the smq server thread
         */
        inline void start_server(){
            smq_thread_ =std::unique_ptr<std::thread>(new std::thread(&Smq::server, this));
        }
        
        /**
         * @brief start the smq client thread 
         */
        inline void start_client(){
            smq_thread_ =std::unique_ptr<std::thread>(new std::thread(&Smq::client, this));
        }

        
        /**
         * @brief stop smq
         */
        inline void stop(){
            ready_ = false;
            smq_thread_->join();
        }

        /**
         * @brief gather info corresponding to signal
         * @param  {int} options:kSignalStatic, kSignalDynamic,kSignalBandwidth and their combination
         */
        const std::vector<std::string>&  gather_info(int ksignal);


        static SmqMeta convert_to_meta(const std::string& info);

        /**
         * @brief set neighbors_ of clients
         */
        void set_neighbors_(std::vector<std::string>& nb){neighbors_ = nb;}
        /**
         * @brief set id 
         * @param  {int} global_id : 
         */
        inline void set_id(int global_id){id_ = global_id;};
    private:
        /**
         * @brief  whether smq is ready: 
         */
        std::atomic<bool> ready_{false};
        std::unique_ptr<std::thread> smq_thread_;
        int id_;

        int epoll_fd_;
        std::vector<int> client_sockets_;


        std::mutex tracker_mu_;
        std::condition_variable tracker_cond_;
        std::pair<int, int> tracker_;
        std::vector<std::string> recved_info_;

        /**
         * @brief send signal to all clients
         * @param  {char*} signal :
         */
        void send_signal_to_clients(const char* signal);
        
        /**
         * @brief send signal to all clients and wait for the return value 
         * @return {std::string}: collected info of all nodes as json string 
         */
        std::string get_info_with_ksignal(int ksignal);
        
        //hosts that need to test bandwidth with 
        std::vector<std::string> neighbors_;

        inline void setNonBlocking(int socket) {
            int flags = fcntl(socket, F_GETFL, 0);
            fcntl(socket, F_SETFL, flags | O_NONBLOCK);
        }
        
        /**
         * @brief server function
         */
        void server();
        /**
         * @brief client function
         */
        void client();
        /**
         * @brief get static info include cpu info, gpu info, and total ram
         * @return {std::string} with json syntax  : 
         */
        nlohmann::json get_static_info();
        /**
         * @brief get dyamic info include cup usage, gou usage and available ram
         * @return {std::string} with json syntax: 
         */
        nlohmann::json get_dynamic_info();
        /**
         * @brief get network info include cup usage, gou usage and available ram
         * @return {std::string} with json syntax: 
         */
        nlohmann::json get_network_info();

        
        /**
         * @brief handle the data received from clients 
         * @param  {std::string} data : 
         */
        void data_handler(const std::string& data);

};


}
#endif // SMQ_H_



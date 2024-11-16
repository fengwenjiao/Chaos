#include "smq.h"

namespace moniter{


    /*overload the to_json and from_json function of nlohmann/json to enable convertation between json and some structures*/
    void to_json(nlohmann::json& j, const moniter::StaticInfo::gpu& g) {
        j = nlohmann::json{{"minor_number", g.minor_number}, {"model_name", g.model_name}, {"gpu_mem_total", g.gpu_mem_total}};
    }

    void from_json(const nlohmann::json& j, moniter::StaticInfo::gpu& g) {
        j.at("minor_number").get_to(g.minor_number);
        j.at("model_name").get_to(g.model_name);
        j.at("gpu_mem_total").get_to(g.gpu_mem_total);
    }

    void to_json(nlohmann::json& j, const moniter::StaticInfo::cpu& c) {
        j = nlohmann::json{{"physical_id", c.physical_id}, {"model_name", c.model_name}};
    }

    void from_json(const nlohmann::json& j, moniter::StaticInfo::cpu& c) {
        j.at("physical_id").get_to(c.physical_id);
        j.at("model_name").get_to(c.model_name);
    }


    void to_json(nlohmann::json& j, const moniter::DynamicInfo::gpu_usage& g) {
        j = nlohmann::json{{"minor_number", g.minor_number}, {"gpu_util", g.gpu_util}, {"mem_util", g.mem_util}};
    }


    void from_json(const nlohmann::json& j, moniter::DynamicInfo::gpu_usage& g) {
        j.at("minor_number").get_to(g.minor_number);
        j.at("gpu_util").get_to(g.gpu_util);
        j.at("mem_util").get_to(g.mem_util);
    }

    void to_json(nlohmann::json& j, const moniter::DynamicInfoMeta& d) {
        j = nlohmann::json{{"available_ram", d.available_ram}, {"cpu_usage", d.cpu_usage}, {"gpu_usage", d.gpu_usage}};
    }

    void from_json(const nlohmann::json& j, moniter::DynamicInfoMeta& d) {
        j.at("available_ram").get_to(d.available_ram);
        j.at("cpu_usage").get_to(d.cpu_usage);
        j.at("gpu_usage").get_to(d.gpu_usage);
    }

    void to_json(nlohmann::json& j, const moniter::StaticInfoMeta& s) {
        j = nlohmann::json{{"cpu", s.cpu_models}, {"gpu", s.gpu_models}, {"total_ram", s.total_ram}};
    }

    void from_json(const nlohmann::json& j, moniter::StaticInfoMeta& s) {
        j.at("cpu").get_to(s.cpu_models);
        j.at("gpu").get_to(s.gpu_models);
        j.at("total_ram").get_to(s.total_ram);
    }

    void to_json(nlohmann::json& j, const moniter::NetworkInfoMeta& n) {
        j = n.bandwidth;
    }

    void from_json(const nlohmann::json& j, moniter::NetworkInfoMeta& n) {
        j.get_to(n.bandwidth);
    }

    void to_json(nlohmann::json& j, const moniter::SmqMeta& s) {
        j = nlohmann::json{{"id", s.id}, {"dynamic_info", s.dynamic_info}, {"static_info", s.static_info}, {"network_info", s.network_info}};
    }

    void from_json(const nlohmann::json& j, moniter::SmqMeta& s) {
        j.at("id").get_to(s.id);
        j.at("dynamic_info").get_to(s.dynamic_info);
        j.at("static_info").get_to(s.static_info);
        j.at("network_info").get_to(s.network_info);
    }



    std::string Smq::get_info_with_ksignal(int ksignal){
        nlohmann::json info;
        info["id"] = id_;
        if(ksignal & kSignalStatic ){info["static_info"] = get_static_info();}
        if(ksignal & kSignalDynamic ){info["dynamic_info"] = get_dynamic_info();}
        if(ksignal & kSignalBandwidth ){info["network_info"] = get_network_info();}
        
        std::string json_string = info.dump(4);
        // std::cout << json_string<< std::endl;
        return json_string;
    }

    nlohmann::json Smq::get_static_info(){
        nlohmann::json static_info;

        nlohmann::json cpu_json = moniter::DynamicInfo::Get().get_cpu_info_();
        nlohmann::json gpu_json = moniter::DynamicInfo::Get().get_gpu_info_();
        nlohmann::json ram_total_json = moniter::DynamicInfo::Get().get_totalram_();

        static_info["cpu"] = cpu_json;
        static_info["gpu"] = gpu_json;
        static_info["total_ram"] = ram_total_json;

        //std::string jsonString = static_info.dump(4);
        // std::cout << jsonString<< std::endl;

        return static_info;
    }

    nlohmann::json  Smq::get_dynamic_info(){
        nlohmann::json dynamic_info;

        nlohmann::json cpu_usage_json = moniter::DynamicInfo::Get().get_cpu_usage();
        nlohmann::json gpu_usage_json = moniter::DynamicInfo::Get().get_gpu_usage();
        nlohmann::json ram_available_json = moniter::DynamicInfo::Get().get_available_ram();

        
        dynamic_info["cpu_usage"] = cpu_usage_json;
        dynamic_info["gpu_usage"] = gpu_usage_json;
        dynamic_info["available_ram"] = ram_available_json;
        // dynamic_info["bandwidth"] = banwidth_json;
        // std::string jsonString = dynamic_info.dump(4);
        // std::cout << jsonString<< std::endl;

        return dynamic_info;
    }

    nlohmann::json Smq::get_network_info(){
        nlohmann::json network_info;
        nlohmann::json bandwidth;
        for(const auto& entry : neighbors_){
            network_info[entry] = moniter::DynamicInfo::Get().test_bandwidth(entry);
        }
        return network_info;
    }


    const std::vector<std::string>&  Smq::gather_info(int ksignal){
        
        std::string signal = std::string("SIGNAL:") + std::to_string(ksignal);
        send_signal_to_clients(signal.c_str());
        
        std::unique_lock<std::mutex> lk(tracker_mu_);
        recved_info_.clear();
        tracker_ = std::make_pair(client_sockets_.size(),0);

        tracker_cond_.wait(lk, [this]{
            return tracker_.first == tracker_.second;
        });

        return recved_info_;

    }

    SmqMeta Smq::convert_to_meta(const std::string& info){
        nlohmann::json j = nlohmann::json::parse(info);
        SmqMeta smq_meta = j.get<SmqMeta>();
        return smq_meta;
    }

    
    void Smq::send_signal_to_clients( const char* signal) {
        
        char buffer[BUFFER_SIZE];
        // std::cout<<signal<<std::endl;
        
        if(client_sockets_.size() == 0)LOG("no connected client");
        for (int client_socket : client_sockets_) {
            send(client_socket, signal, strlen(signal), 0);
            LOG("Instruction sent to client socket " << client_socket << ": " << signal );
        }
    }

    void Smq::server(){
        int server_fd;
        // int epoll_fd_;
        int num_fds;
        int opt = 1;
        
        struct sockaddr_in server_addr;
        struct epoll_event ev, events[MAX_EVENTS];

        int new_socket;
        struct sockaddr_in ns_address;
        int ns_addrlen = sizeof(ns_address);

        char buffer[BUFFER_SIZE] = {0};

        if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("socket create failed");
            return ;
        }

        if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))<0){
            perror("setsocket failed");
            return ;
        }
        
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_port = htons(SERVER_PORT);

        if(bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr))<0){
            perror("bind failed");
            return ;
        }
        
        if(listen(server_fd, 10 ) < 0){
            perror("listen failed");
            return ;
        }

        setNonBlocking(server_fd);

        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ == -1) {
            perror("epoll_create1 failed");
            exit(EXIT_FAILURE);
        }


        ev.events = EPOLLIN;
        ev.data.fd = server_fd;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
            perror("epoll_ctl:  add server_fd failed");
            exit(EXIT_FAILURE);
        }

        LOG("Server is listening on port " << SERVER_PORT);
        // std::cout << "Server is listening on port " << SERVER_PORT << std::endl;  
        
        std::thread iperf_server_thread([]() { moniter::DynamicInfo::Get().start_bandwidth_test_server(); });

        ready_.store(true);
        while(true){
            num_fds = epoll_wait(epoll_fd_, events, MAX_EVENTS, 100);
            if(!ready_.load()){
                break;
            }
            if (num_fds == -1){
                perror("epoll_wait() failed");
                exit(EXIT_FAILURE);
            }

            for (int n = 0; n < num_fds; ++n){
                
                if(events[n].data.fd == server_fd){
                    new_socket = accept(server_fd, (struct sockaddr*)&ns_address, (socklen_t*)&ns_addrlen);

                    if (new_socket == -1) {
                        perror("accept new socket failed");
                        exit(EXIT_FAILURE);
                    }
                    
                    setNonBlocking(new_socket);
                    ev.events = EPOLLIN | EPOLLET; 
                    ev.data.fd = new_socket;
                    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                        perror("epoll_ctl: new_socket");
                        exit(EXIT_FAILURE);
                    }
                    client_sockets_.push_back(new_socket);

                    std::cout << "New connection established" << std::endl;
                }else{
                    int client_socket = events[n].data.fd;
                    int valread = read(client_socket, buffer, BUFFER_SIZE);
                    if (valread == 0) {
                        std::cout << "Client disconnected" << std::endl;
                        close(client_socket);
                        tracker_mu_.lock();
                        tracker_.first--;
                        client_sockets_.erase(std::remove(client_sockets_.begin(), client_sockets_.end(), client_socket), client_sockets_.end());
                        tracker_mu_.unlock();
                    } else if (valread > 0) {
                        std::vector<char> recv_data;
                        recv_data.insert(recv_data.end(), buffer, buffer + valread);
                        while ((valread = read(client_socket, buffer, BUFFER_SIZE)) > 0) {
                                std::cout << "Receiving... " << std::endl;
                                recv_data.insert(recv_data.end(), buffer, buffer + valread);
                                
                                if (valread < BUFFER_SIZE) {
                                    break;
                                }
                            }
                        std::string json_str(recv_data.begin(), recv_data.end());
                        data_handler(json_str);
                        // std::cout << "Data received:\n " << json_str << std::endl;
                        memset(buffer, 0, BUFFER_SIZE);
                    }
                }
            }
            
        }
        moniter::DynamicInfo::Get().stop_bandwidth_test_server();
        iperf_server_thread.join();
        for (int client_socket : client_sockets_) {
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_socket, nullptr); 
            close(client_socket); 
        }
        close(epoll_fd_);
        close(server_fd);
    }


    void Smq::client(){
        int sock = 0;
        struct sockaddr_in server_addr;
        char buffer[BUFFER_SIZE] = {0};

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Socket creation error");
            return ;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_port = htons(SERVER_PORT);

        // if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        //     perror("Invalid address");
        //     return -1;
        // }


        if (connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            perror("Connection Failed");
            return ;
        }

        std::thread iperf_server_thread([]() { moniter::DynamicInfo::Get().start_bandwidth_test_server(); });

        ready_.store(true);
        while (ready_.load()) {
            if (read(sock, buffer, BUFFER_SIZE) <= 0) {
                LOG("Server disconnected");
                break;
            }
            std::string body(buffer);
            body += std::string("\n");
            if(body.find("SIGNAL:") != std::string::npos){
                int ksignal = std::stoi(Util::find_value(body, "SIGNAL:"));
                LOG("kSignal:"<<ksignal);
                std::string data = get_info_with_ksignal(ksignal);
                LOG("data length:"<<data.length());
                send(sock, data.c_str(), data.length(), 0);
            }else{
                std::cout << "Unkown mesage" << std::endl;
            }
            memset(buffer, 0, BUFFER_SIZE);
        }

        moniter::DynamicInfo::Get().stop_bandwidth_test_server();
        iperf_server_thread.join();
        close(sock);
        return ;

    };

    void Smq::data_handler(const std::string& data){
        std::unique_lock<std::mutex> lk(tracker_mu_);
        recved_info_.push_back(data);
        tracker_.second++;
        std::cout << "Client responded, total responses: " << tracker_.second << std::endl;
        
        tracker_cond_.notify_one();
    }


}
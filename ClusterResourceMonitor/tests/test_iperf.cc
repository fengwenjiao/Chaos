
#include <smq.h>

using namespace moniter;
int main() {
    DynamicInfo& dynamic = DynamicInfo::Get();
    std::unique_ptr<std::thread> server_thread;
    server_thread =std::unique_ptr<std::thread>(new std::thread(&DynamicInfo::start_bandwidth_test_server, &dynamic));
    // std::thread serverThread([]() { DynamicInfo::Get().start_bandwidth_test_server(); });
    while(!DynamicInfo::Get().is_server_ready())std::this_thread::sleep_for(std::chrono::milliseconds(500));;
    // DynamicInfo::Get().test_bandwidth("127.0.0.1");
    // //  std::cout <<"[test_iperf.cc:"<<__LINE__<<"]"<< "test finished"<<std::endl;
    DynamicInfo::Get().stop_bandwidth_test_server();
    server_thread->join();
    return 0;
}

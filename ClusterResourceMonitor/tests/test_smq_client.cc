#include "smq.h"


int main() {
    moniter::Smq test ;
    test.set_id(1);
    test.start_client();

    std::vector<std::string> neightbors;
    neightbors.push_back("192.168.1.11");
    test.set_neighbors_(neightbors);
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    test.stop();
    LOG("clinet stop")
    return 0;
}

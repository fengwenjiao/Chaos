#include "smq.h"
#include "util.h"
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <id>" << std::endl;
        return 1;
    }

    int id = std::atoi(argv[1]);
    if (id <= 0) {
        std::cerr << "Invalid id: " << argv[1] << std::endl;
        return 1;
    }

    moniter::Smq test("192.168.1.16", 60000);
    test.start_client(id);
    while(true){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    // test.stop_smq();
    return 0;
}
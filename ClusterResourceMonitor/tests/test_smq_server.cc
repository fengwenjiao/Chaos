#include "smq.h"

using namespace moniter;

int main() {
    moniter::Smq test;
    test.start_server();
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        auto &infos =
            test.gather_info(kSignalStatic + kSignalDynamic + kSignalBandwidth);
        LOG("recved info:");
        for (auto& info : infos) {
            auto obj = moniter::Smq::convert_to_meta(info);
            nlohmann::json j = nlohmann::json::parse(info);
            LOG("id:" << obj.id << " static_info:" << j["static_info"]
                       << " dynamic_info:" << j["dynamic_info"]
                       << " network_info:" << j["network_info"]);
        }
    }

    // test.stop();
    return 0;
}

#include <cstddef>
#include <stdio.h>
#include <stdint.h>
#include "moniter.h"
#include "iperf_api.h"
#include <vector>
// #include <iperf_api.h>
// #include "/usr/include/iperf_api.h"

int main() {
    struct iperf_test *test;
    struct iperf_stream *stream; // Ensure this declaration is visible before use
    test = iperf_new_test();
    if (test == NULL) {
        fprintf(stderr, "Failed to create test\n");
        return -1;
    }

    iperf_defaults(test);
    iperf_set_test_role(test, 'c');
    iperf_set_test_server_hostname(test, "192.168.1.13");
    iperf_set_test_duration(test, 1);
    iperf_set_test_json_output(test, 1);
    
    if (iperf_run_client(test) < 0) {
        fprintf(stderr, "Error in client: %s\n", iperf_strerror(i_errno));
        iperf_free_test(test);
        return -1;
    }
    char *json_output = iperf_get_test_json_output_string(test);
    if (json_output == NULL) {
        std::cerr << "Failed to get JSON output " << std::endl;
        iperf_free_test(test);
        return -1;
    }
    std::string json_str(json_output);


    size_t pos = json_str.find("sum_received");
    std::string bandwidth = moniter::Util::find_value( json_str, "bits_per_second",pos);
    float band = std::stof(bandwidth);
    std::cout <<"[test_iperf.cc:"<<__LINE__<<"]"<< "bandwidth "<< band/(1000*1000) <<"Mbps"<<std::endl;
    iperf_free_test(test);
    return 0;
}
#include "include/test_pushpull_utils.h"
#include "include/test_debug_utils.h"
#include "constellation.h"

using namespace constellation;

constexpr int KEY_NUM = 20;

int main(int argc, char* argv[]) {
//  bool is_trainer = test::IsTrainer();
//  if (!is_trainer) {
//    // start Controller: kvapp layer, process datamsg
//    constellation::ConstelController controller;
//    while (true) {
//      std::this_thread::sleep_for(std::chrono::seconds(1000));
//    }
//  } else {
//    // for other nodes
//    constellation::ConstelTrainer trainer;
//    // simulate the process of ctx prepare
//    int sleep_time = test::RandomUtils::generate_random_number(10, 20);
//    std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
//
//  }


  // generate test parameters
  auto params = test::ParameterMock(KEY_NUM);
  params.fill();
  // print
  using namespace test;
  std::cout<<params._parameters;

  return 0;
}
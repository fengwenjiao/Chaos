#include "constellation.h"
#include <iostream>
#include "include/test_pushpull_utils.h"
using namespace constellation;

int main(int argc, char* argv[]) {
  bool is_trainer = test::IsTrainer();
  if (!is_trainer) {
    // start Controller: kvapp layer, process datamsg
    constellation::ConstelController controller;
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1000));
    }
  } else {
    // for other nodes
    constellation::ConstelTrainer trainer;
    // simulate the process of ctx prepare
    int sleep_time = test::RandomUtils::generate_random_number(10, 20);
    std::this_thread::sleep_for(std::chrono::seconds(sleep_time));

//        trainer.Init(0, nullptr); // send ready signal to controller
  }

}
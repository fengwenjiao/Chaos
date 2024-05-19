#include "constellation.h"
#include <random>
#include <iostream>

int generate_random_number(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(min, max);

  return distrib(gen);
}

/**
 * \return whether or not this process is a worker node.
 *
 * Always returns true when type == "local"
 */
static bool IsTrainer() {
  const char* role_str = ps::Environment::Get()->find("DMLC_ROLE");
  return (role_str == nullptr) || (!strcmp(role_str, "trainer"));
}

int main(int argc, char* argv[]) {
  bool is_trainer = IsTrainer();
  if (!is_trainer) {
    // start Controller: kvapp layer, process datamsg
    constellation::ConstelController controller;
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1000));
    }
  } else {
    // for other nodes
    constellation::ConstelTrainer trainer;
    //simulate the process of ctx prepare
    int sleep_time = generate_random_number(10, 20);
    std::this_thread::sleep_for(std::chrono::seconds(sleep_time));

    trainer.Init(0, nullptr); // send ready signal to controller
  }

  return 0;
}
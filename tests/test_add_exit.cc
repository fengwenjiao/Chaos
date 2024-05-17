#include "constellation.h"
#include <random>
#include <iostream>

int generate_random_number(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(min, max);

  return distrib(gen);
}

int main(int argc, char* argv[]) {
  // start van and postoffice layer
  ps::StartAsync(0, "constellation\0");
  // if (!ps::Postoffice::Get()->is_scale() && !ps::Postoffice::Get()->is_recovery()) {
  //   ps::Postoffice::Get()->Barrier(0, ps::kWorkerGroup + ps::kServerGroup + ps::kScheduler);
  // }

  bool is_scheduler = ps::IsScheduler();
  if (is_scheduler) {
    // start Controller: kvapp layer, process datamsg
    constellation::ConstelController controller;
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1000));
    }
  } else {
    // for other nodes
    int sleep_time = generate_random_number(10, 20);
    std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
  }

  ps::Finalize(0, false);
  return 0;
}
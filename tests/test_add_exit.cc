#include <iostream>

#include "constellation.h"
#include "include/test_pushpull_utils.h"
#include "include/trainer_debug_utils.h"

using namespace constellation;

const int KEY_NUM = 20;
const int TIMES = 200000;

int main(int argc, char* argv[]) {
  test::RandomUtils::set_seed(10);
  bool is_trainer = test::IsTrainer();
  if (!is_trainer) {
    // start Controller: kvapp layer, process datamsg
    constellation::ConstelController controller;
    controller.setThinker("SinglePointConfThinker");
    controller.run();
  }
  // for other nodes
  constellation::ConstelTrainer trainer;
  int rank = trainer.myRank();

  auto params = test::ParameterMock(KEY_NUM, 1000000, 1000000);
  params.fill(rank, 0);
  using namespace test;
  // std::cout<<params;
  trainer.NotifyReadyAndWait(true, params.keys(), params.lens());
  if (trainer.is_scale()) {
    trainer.Recv(params.keys(), params.pointers());
  } else {
    trainer.Broadcast(params.keys(), params.pointers());
  }
  // std::cout<<params;

  using namespace test;
  for (int i = 0; i < TIMES; i++) {
    // formated print time
    auto start = std::chrono::system_clock::now();
    std::cout << "=====================" << test::GetTimestamp(trainer)
              << "=====================" << std::endl;
    auto time = std::chrono::system_clock::to_time_t(start);
    std::cout << "Time: " << std::put_time(std::localtime(&time), "%H:%M:%S") << std::endl;
    std::cout << "MyRank: " << trainer.myRank() << std::endl;
    std::cout << "NodeTransTopo: " << trainer.GetNodeTransTopo() << std::endl;

    trainer.PushPull(params._ids, params._parameters, params._pointers);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::vector<int> keys_to_migrate;
    trainer.BatchEnd(&keys_to_migrate);
    if (keys_to_migrate.size() > 0) {
      trainer.Migrate(params.keys(), params.values());
    }
    std::cout << "======================================\n" << std::endl;
  }

  // since exit logic is not implemented, we need to sleep for a while to keep the process alive
  std::this_thread::sleep_for(std::chrono::seconds(5));

  return 0;
}
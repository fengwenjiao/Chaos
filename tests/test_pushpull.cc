#include "include/test_pushpull_utils.h"
#include "include/test_debug_utils.h"
#include "constellation.h"

#include <chrono>
#include <iomanip>

using namespace constellation;

constexpr int KEY_NUM = 100;
constexpr int TIMES = 10;

constexpr int PARAMSIZE_DOWN = 1000000;
constexpr int PARAMSIZE_UP = 1000000;

int main(int argc, char* argv[]) {
  test::RandomUtils::set_seed(10);
  bool is_trainer = test::IsTrainer();
  if (!is_trainer) {
    // start Controller: kvapp layer, process datamsg
    constellation::ConstelController controller;
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1000));
    }
  }
  // for other nodes
  constellation::ConstelTrainer trainer;
  // simulate the process of ctx prepare
  int sleep_time = test::RandomUtils::generate_random_number(1, 3);
  std::this_thread::sleep_for(std::chrono::seconds(sleep_time));

  int rank = trainer.myRank();
  // generate test parameters
  auto params = test::ParameterMock(KEY_NUM, PARAMSIZE_DOWN, PARAMSIZE_UP);
  params.fill(rank, 0);
  // print
  using namespace test;
  trainer.NotifyReadyAndWait();
  trainer.Broadcast(params._ids, params._pointers);

  auto start = std::chrono::high_resolution_clock ::now();
  for (int i = 1; i <= TIMES; ++i) {
    params.fill(rank, i);

    trainer.PushPull(params._ids, params._parameters, params._pointers);

    //        std::cout<<params;

    int num = trainer.NumTrainers();

    auto expected = params.expected_values(num, i);
    //        std::cout<<"expected"<<std::endl;
    //        std::cout<<expected;

    for (int key = 0; key < KEY_NUM; ++key) {
      auto tot = 0.0f;
      auto* data1 = reinterpret_cast<float*>(params._parameters[key].data());
      auto* data2 = reinterpret_cast<float*>(expected._parameters[key].data());
      int size = params[key].size() / sizeof(float);
      for (int j = 0; j < size; ++j) {
        auto diff = std::fabs(data1[j] - data2[j]);
        tot += diff;
      }
      tot /= size;
      tot /= num;
      //      std::cout << "Test times " << i << " key " << key << " size " << size << " diff " <<
      //      tot
      //                << std::endl;
      if (tot > 1e-4) {
        std::cout << "Test times " << i << " key " << key << " size " << size << " diff " << tot
                  << std::endl;
        //                std::cout<<params;
        //                std::cout<<expected;
      }
      CHECK_LT(tot, 1e-4);
    }
  }
  auto finish = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
  auto time_cost = duration / 1e3;
  auto bytes_M = (PARAMSIZE_DOWN + PARAMSIZE_UP) / 2.0f * sizeof(float) / 1e6 * KEY_NUM * TIMES;
  std::cout << "Test pass!!" << std::endl;
  std::cout << "Total Epoches: " << TIMES << std::endl;
  std::cout << std::fixed << std::setprecision(3);
  std::cout << "Bytes transferred per epoch(approximately): " << bytes_M / TIMES << "M"
            << std::endl;
  std::cout << "Average time cost per epoch: " << time_cost / TIMES << "s " << std::endl;
  std::cout << "speed: " << bytes_M / time_cost << "Mb/s" << std::endl;
  
  // since exit logic is not implemented, we need to sleep for a while to keep the process alive
  std::this_thread::sleep_for(std::chrono::seconds(5));

  return 0;
}
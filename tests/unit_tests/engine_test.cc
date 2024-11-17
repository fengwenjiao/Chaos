#include <gtest/gtest.h>
#include "../src/engine.hpp"
#include <iostream>

// Define a test fixture class
class ConstelAggEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Set up the engine with 2 threads
    engine = new constellation::ConstelAggEngine<int, int>(2);
    for (int i = 1; i < 4; ++i) {
      record[i] = 0;
      processed_data[i] = 0;
    }
    // Set the data handle function
    engine->set_data_handle(
        [this](int id, int data, std::shared_ptr<constellation::ReturnOnAgg<int, int>> cb) {
          // Simulate some processing
          processed_data[id] += data;
          record[id]++;
          // Call the callback with the processed data
          if (record[id] == 2) {
            (*cb)(processed_data[id]);
            processed_data[id] = 0;
            record[id] = 0;
          }
        });
    // Set the measure function
    engine->set_messure_func([this](int id, int data) {
      // Simulate measuring the data
      return 1;
    });
    engine->Start();
  }

  void TearDown() override {
    record.clear();
    processed_data.clear();
    engine->Stop();
    // Clean up the engine
    delete engine;
  }

  // Declare variables needed for the tests
  constellation::ConstelAggEngine<int, int>* engine;
  std::unordered_map<int, int> record;
  std::unordered_map<int, int> processed_data;
  std::unordered_map<int, int> res;
};

// Test case1 for the PushAndWait function
TEST_F(ConstelAggEngineTest, PushAndWaitTest) {
  std::thread t([this]() { engine->PushAsync({1, 2, 3}, {10, 20, 30}); });

  engine->PushAndWait({1, 2, 3}, {10, 20, 30}, &res);

  std::unordered_map<int, int> expected_results;
  expected_results[1] = 20;  // 10 + 10
  expected_results[2] = 40;  // 20 + 20
  expected_results[3] = 60;  // 30 + 30

  // Check if the results match the expected results
  for (const auto& pair : expected_results) {
    EXPECT_EQ(pair.second, res[pair.first]);
  }
  t.join();
}

// Test case2 for the PushAndWait function
TEST_F(ConstelAggEngineTest, PushAsyncTest) {
  std::thread t([this]() { engine->PushAsync({1, 2, 3, 4}, {10, 20, 30, 40}); });

  engine->PushAndWait({1, 2, 3}, {10, 20, 30}, &res);

  std::unordered_map<int, int> expected_results;
  expected_results[1] = 20;  // 10 + 10
  expected_results[2] = 40;  // 20 + 20
  expected_results[3] = 60;  // 30 + 30

  // Check if the results match the expected results
  for (const auto& pair : expected_results) {
    EXPECT_EQ(pair.second, res[pair.first]);
  }

  engine->PushAndWait({4}, {40}, &res);
  expected_results[4] = 80;  // 40 + 40
  for (const auto& pair : expected_results) {
    EXPECT_EQ(pair.second, res[pair.first]);
  }

  t.join();
}
#include <random>
TEST_F(ConstelAggEngineTest, PushTestConcurrent) {
  int count = 10;
  std::mutex mu;
  std::condition_variable cv;
  bool is_ready = false;

  // generate random numbers,val1(4,10000),vals(4,10000)
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 100);
  std::vector<std::vector<int>> vals1;
  std::vector<std::vector<int>> vals2;
  for (int i = 0; i < count; i++) {
    std::vector<int> val1;
    std::vector<int> val2;
    for (int j = 0; j < 4; j++) {
      val1.push_back(dis(gen));
      val2.push_back(dis(gen));
    }
    vals1.push_back(val1);
    vals2.push_back(val2);
  }
  auto expected_results = vals1;  // copy vals1
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < 4; j++) {
      expected_results[i][j] += vals2[i][j];
    }
  }

  std::thread t([this, &count, &vals1, &mu, &cv, &is_ready]() mutable {
    std::unique_lock<std::mutex> lock(mu);
    for (int i = 0; i < count; i++) {
      lock.unlock();
      engine->PushAsync({1, 2, 3, 4}, std::move(vals1[i]));
      // wait is_ready to be true
      lock.lock();
      cv.wait(lock, [&is_ready] { return is_ready; });
      is_ready = false;
    }
  });
  std::unordered_map<int, int> res;

  std::unique_lock<std::mutex> lock(mu, std::defer_lock);
  for (int i = 0; i < count; i++) {
    // lock.unlock();
    engine->PushAndWait({1, 2, 3, 4}, std::move(vals2[i]), &res);
    for (int j = 0; j < 4; j++) {
      EXPECT_EQ(expected_results[i][j], res[j + 1]);
    }
    mu.lock();
    is_ready = true;
    mu.unlock();
    cv.notify_one();
  }

  t.join();
}

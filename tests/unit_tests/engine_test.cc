#include <gtest/gtest.h>
#include "internal/engine.h"
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
        [this](int id, int data, constellation::ConstelAggEngine<int, int>::ReturnOnAgg cb) {
          // Simulate some processing
          processed_data[id] += data;
          record[id]++;
          // Call the callback with the processed data
          if (record[id] == 2) {
            cb(processed_data[id]);
            processed_data[id] = 0;
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
  std::shared_ptr<std::unordered_map<int, int>> res =
      std::make_shared<std::unordered_map<int, int>>();
};

// Test case1 for the PushAndWait function
TEST_F(ConstelAggEngineTest, PushAndWaitTest) {
  std::thread t([this]() { engine->PushAsync({1, 2, 3}, {10, 20, 30}); });

  engine->PushAndWait({1, 2, 3}, {10, 20, 30}, res);

  std::unordered_map<int, int> expected_results;
  expected_results[1] = 20;  // 10 + 10
  expected_results[2] = 40;  // 20 + 20
  expected_results[3] = 60;  // 30 + 30

  // Check if the results match the expected results
  for (const auto& pair : expected_results) {
    EXPECT_EQ(pair.second, (*res)[pair.first]);
  }
  t.join();
}

// Test case2 for the PushAndWait function
TEST_F(ConstelAggEngineTest, PushAsyncTest) {
  std::thread t([this]() { engine->PushAsync({1, 2, 3, 4}, {10, 20, 30, 40}); });

  engine->PushAndWait({1, 2, 3}, {10, 20, 30}, res);

  std::unordered_map<int, int> expected_results;
  expected_results[1] = 20;  // 10 + 10
  expected_results[2] = 40;  // 20 + 20
  expected_results[3] = 60;  // 30 + 30


  // Check if the results match the expected results
  for (const auto& pair : expected_results) {
    EXPECT_EQ(pair.second, (*res)[pair.first]);
  }

  engine->PushAndWait({4},{40},res);
  expected_results[4] = 80; // 40 + 40
  for (const auto& pair : expected_results) {
    EXPECT_EQ(pair.second, (*res)[pair.first]);
  }

  t.join();
}

// Run the tests
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

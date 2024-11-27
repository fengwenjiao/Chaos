#include <gtest/gtest.h>
#include "../src/algorithm/basic.h"
#include "constellation_commons.h"
#include <iostream>

using namespace constellation;
class AlgoTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup code here, if needed.
  }

  void TearDown() override {
    // Teardown code here, if needed.
  }
};

TEST_F(AlgoTest, DijstraPathsAlgo1) {
  AdjacencyList overlay = {{1, {2, 3}}, {2, {4}}, {3, {4}}, {4, {}}};

  AdjacencyListT<float> weights = {{1, {1.0, 2.0}}, {2, {1.0}}, {3, {4.0}}, {4, {}}};
  
  int target = 4;

  using namespace constellation::algorithm::basic;
  std::vector<float> path_weights;
  auto paths = dijsktra_paths(overlay, weights, target, &path_weights);
  // expeced paths: {3,4}, {2,4}, {1,2,4}
  // expected path weights: 4, 1, 2
  ASSERT_EQ(paths.size(), 3);
  ASSERT_EQ(paths[0] == TransPath({3, 4}), true);
  ASSERT_EQ(paths[1] == TransPath({2, 4}), true);
  ASSERT_EQ(paths[2] == TransPath({1, 2, 4}), true);
  ASSERT_EQ(path_weights.size(), 3);
  ASSERT_EQ(path_weights[0], 4);
  ASSERT_EQ(path_weights[1], 1);
  ASSERT_EQ(path_weights[2], 2);
}


TEST_F(AlgoTest, DijstraPathsAlgo2) {
  AdjacencyList overlay = {
      {1, {2, 3, 4}}, 
      {2, {1, 4, 5}}, 
      {3, {1, 7}}, 
      {4, {1, 2, 5, 7}}, 
      {5, {2, 4}}, 
      {7, {3, 4}}
      };

  AdjacencyListT<float> weights = {
      {1, {2.0, 5.0, 1.0}}, 
      {2, {2.0, 3.0, 8.0}}, 
      {3, {5.0, 7.0}}, 
      {4, {1.0, 3.0, 4.0, 2.0}}, 
      {5, {8.0, 4.0}}, 
      {7, {7.0, 2.0}}
      };

  int target = 2;

  using namespace constellation::algorithm::basic;
  std::vector<float> path_weights;
  auto paths = dijsktra_paths(overlay, weights, target,&path_weights);
  // expeced paths: {7,4,2},{5,4,2},{4,2},{3,1,2},{1,2}
  ASSERT_EQ(paths.size(), 5);
  ASSERT_EQ(paths[0] == TransPath({7, 4, 2}), true);
  ASSERT_EQ(paths[1] == TransPath({5, 4, 2}), true);
  ASSERT_EQ(paths[2] == TransPath({4, 2}), true);
  ASSERT_EQ(paths[3] == TransPath({3, 1, 2}), true);
  ASSERT_EQ(paths[4] == TransPath({1, 2}), true);
  // expected path weights: 5,7,3,7,2
  ASSERT_EQ(path_weights.size(), 5);
  ASSERT_EQ(path_weights[0], 5);
  ASSERT_EQ(path_weights[1], 7);
  ASSERT_EQ(path_weights[2], 3);
  ASSERT_EQ(path_weights[3], 7);
  ASSERT_EQ(path_weights[4], 2);
}

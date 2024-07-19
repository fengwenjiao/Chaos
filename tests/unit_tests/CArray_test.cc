#include "constellation_trainer.h"
#include <gtest/gtest.h>
using namespace constellation;
class CArrayTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code here, if needed.
  }

  void TearDown() override {
    // Teardown code here, if needed.
  }
};


TEST_F(CArrayTest, SizeConstructor) {
  size_t size = 10;
  CArray array(size);
  EXPECT_EQ(array.size(), size);
  ASSERT_NE(array.data(), nullptr);
}

TEST_F(CArrayTest, CopyFromOtherCArray) {
  size_t size = 5;
  CArray source(size);
  memset(source.data(), 'a', size);

  CArray destination(size);
  destination.CopyFrom(source);

  EXPECT_EQ(destination.size(), source.size());
  EXPECT_EQ(memcmp(destination.data(), source.data(), size), 0);

  CArray destination2;
  destination2.CopyFrom(destination);
  EXPECT_EQ(destination2.size(), destination.size());
  EXPECT_EQ(memcmp(destination2.data(), destination.data(), size), 0);

  for (size_t i = 0; i < size; ++i) {
    EXPECT_EQ(destination2.data()[i], 'a');
  }
}

TEST_F(CArrayTest, CopyFromRawData) {
  size_t size = 5;
  char data[size];
  memset(data, 'b', size);

  CArray array(size);
  array.CopyFrom(data, size);

  EXPECT_EQ(memcmp(array.data(), data, size), 0);
}

TEST_F(CArrayTest, CopyFromRawPointer){
  CArray source(5);
  memset(source.data(), 'a', source.size());
  char data[] = {'b', 'b', 'b', 'b', 'b'};
  // This should fail due to size mismatch
  EXPECT_ANY_THROW(source.CopyFrom(data, 4));
}

TEST_F(CArrayTest, IsNone) {
  CArray array;
  EXPECT_TRUE(array.isNone());
}

TEST_F(CArrayTest, InitializesCorrectly) {
  size_t size = 100;
  {
    CArray array(size);
    EXPECT_NE(array.sptr_, nullptr); // check sptr_ is not nullptr
    EXPECT_EQ(array.sptr_->size_, size); // check size_ is correct

    CArray array2 = array;
    CArray array3(array2);
    CArray array4(std::move(array3));

    EXPECT_NE(array2.sptr_, nullptr); // check sptr_ is not nullptr
    EXPECT_EQ(array2.sptr_->size_, size); // check size_ is correct
  }
    
}


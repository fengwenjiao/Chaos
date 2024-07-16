#ifndef CONSTELLATION_TEST_PUSH_PULL_UTILS_H
#define CONSTELLATION_TEST_PUSH_PULL_UTILS_H

#include "test_utils.h"
#include "../include/internal/CArray.h"
#include "../ps-lite-elastic/include/ps/base.h"

namespace constellation {
namespace test {
/**
 * \return whether or not this process is a worker node.
 *
 * Always returns true when type == "local"
 */
inline bool IsTrainer() {
  const char* role_str = ps::Environment::Get()->find("DMLC_ROLE");
  return (role_str == nullptr) || (!strcmp(role_str, "trainer"));
}

/**
 * Generates a vector of random value sizes.
 *
 * This function generates a collection of random sizes corresponding to a given number of keys.
 * Each value size is randomly set between 1 and 100.
 *
 * @param key_num The number of keys for which value sizes need to be generated. This parameter determines the length of the vector.
 * @return Returns a vector containing the random sizes of the values. Each element represents the size of a value corresponding to a key.
 */
std::vector<int> value_sizes_generator(int key_num) {
  std::vector<int> value_sizes;
  for (int i = 0; i < key_num; ++i) {
    value_sizes.push_back(RandomUtils::generate_random_number(1, 100));
  }
  return value_sizes;
}


/**
 * @brief A mock structure for simulating parameter collections.
 *
 * This structure provides a simple way to simulate a collection of parameters,
 * including storage for ids, sizes of values, and the actual parameter values.
 * It allows access to parameter values by id and offers a method to fill the
 * parameter values, useful for testing purposes.
 */
struct ParameterMock {
  int _size; /**< The size of the parameter collection, i.e., the number of ids. */
  std::vector<int> _ids; /**< Vector storing the ids of the parameters. */
  std::vector<int> _value_sizes; /**< Vector storing the sizes of each parameter value. */
  std::vector<CArray> _parameters; /**< Vector storing the parameter values. */

  /**
   * @brief Constructor, initializes the parameter mock.
   *
   * @param num The number of the parameters collection.
   */
  ParameterMock(int num) {
    _size = num;
    _value_sizes = value_sizes_generator(num);
    for (int i = 0; i < _size; ++i) {
      _parameters.push_back(CArray(_value_sizes[i] * sizeof(float)));
      _ids.push_back(i);
    }
  }

  /**
   * @brief Destructor, cleans up parameter resources.
   */
  ~ParameterMock() {
    _parameters.clear();
    _ids.clear();
  }

  /**
   * @brief Accessor for the keys.
   *
   * @return const std::vector<int>& Reference to the vector of keys.
   */
  const std::vector<int>& keys()const {
    return _ids;
  }

  /**
   * @brief Accessor for the parameter values.
   *
   * @return std::vector<CArray>& Reference to the vector of parameter values.
   */
  std::vector<CArray>& values() {
    return _parameters;
  }

  /**
   * @brief Fills the parameter values.
   *
   * @param rank Rank used when filling the parameter values.
   * @param ts Timestamp used when filling the parameter values.
   *
   * This method fills each parameter value with the result of a specific
   * mathematical formula based on the given rank and timestamp.
   * If the size of the parameter collection does not match expectations or there
   * are uninitialized parameters, a runtime error will be thrown.
   */
  void fill(int rank = 0, int ts = 0) {
    if(_parameters.size() != _size || _parameters[_size - 1].isNone()) {
      throw std::runtime_error("ParameterMock::fill() called with invalid parameters");
    }
    for (int key = 0; key < _size; ++key) {
      float* data = (float*)_parameters[key].data();
      for (int i = 0; i < _value_sizes[key]; ++i) {
        data[i] = rank * 2.3 + ts * 3.7 + i * 7.3 + key * 98.1;
      }
    }
  }

  /**
   * @brief Accesses a parameter value by id.
   *
   * @param id The id of the parameter.
   * @return CArray& Reference to the parameter value.
   */
  CArray& operator[](int id) {
    return _parameters[id];
  }
};


}  // namespace test
}  // namespace constellation

#endif  // CONSTELLATION_TEST_PUSH_PULL_UTILS_H

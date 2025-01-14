#ifndef CONSTELLATION_TEST_UTILS_H
#define CONSTELLATION_TEST_UTILS_H

#include <random>
#include <cstring>
#include <stdexcept>

namespace constellation {
namespace test {

class RandomUtils {
 public:
  static std::mt19937& engine() {
    static std::mt19937 instance(std::random_device{}());
    return instance;
  }
  static void set_seed(unsigned int seed) {
    engine().seed(seed);
  }

  template <typename T>
  static T generate_random_number(T min, T max) {
    if (min > max) {
      throw std::invalid_argument("min should be less than or equal to max");
    }
    using distribution_type =
        std::conditional_t<std::is_integral<T>::value,
                           std::uniform_int_distribution<T>,
                           std::uniform_real_distribution<T>>;
    distribution_type distrib(min, max);
    return distrib(engine());
  }
};

}  // namespace test
}  // namespace constellation

#endif  // CONSTELLATION_TEST_UTILS_H

#pragma once

#include <cassert>
#include <random>

namespace magic {

//////////////////////////////////////////////////////////////////////

// Represents a pseudo-random number generator

class RandomGenerator final {
 public:
  RandomGenerator() : twister_(GenerateSeed()) {
  }

  RandomGenerator(size_t seed) : twister_(seed) {
  }

  // Returns a non-negative random integer
  uint32_t Next() {
    return twister_();
  }

  // Returns a random integer that is within a specified range.
  uint32_t Next(uint32_t min_value, uint32_t max_value) {
    assert(max_value >= min_value);
    auto distrib = std::uniform_int_distribution<uint32_t>(min_value, max_value);
    return distrib(twister_);
  }

  // Returns a non-negative random integer that is less than the specified maximum
  uint32_t Next(uint32_t max_value) {
    return Next(0, max_value);
  }

  uint64_t NextInt64() {
    return twister_();
  }

  // Returns a random integer64 that is within a specified range.
  uint64_t Next(uint64_t min_value, uint64_t max_value) {
    assert(max_value >= min_value);
    auto distrib = std::uniform_int_distribution<uint64_t>(min_value, max_value);
    return distrib(twister_);
  }

  // Returns a non-negative random integer that is less than the specified maximum
  uint64_t Next(uint64_t max_value) {
    return Next(0, max_value);
  }

 private:
  static std::random_device::result_type GenerateSeed() {
    return std::random_device{}();
  }

 private:
  std::mt19937_64 twister_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
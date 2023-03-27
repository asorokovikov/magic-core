#pragma once

#include <cstdint>

namespace magic {

//////////////////////////////////////////////////////////////////////

// Thread-safe methods that may be used concurrently from any thread

struct Random {

  // Returns a non-negative random integer
  static uint32_t Next();

  // Returns a non-negative random integer that is less than the specified maximum
  static uint32_t Next(uint32_t max_value);

  // Returns a random integer that is within a specified range
  static uint32_t Next(uint32_t min_value, uint32_t max_value);

  // Returns a non-negative random integer
  static uint64_t NextInt64();

};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
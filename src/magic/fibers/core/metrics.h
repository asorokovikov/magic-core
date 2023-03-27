#pragma once

#include <cstdlib>

namespace magic {

struct AllocatorMetrics {
  size_t total_allocate = 0;
  size_t allocate_new_count = 0;
  size_t release_count = 0;
  size_t total_allocate_bytes = 0;
};

}  // namespace magic
#include <magic/fibers/core/stack.h>
#include <magic/fibers/core/metrics.h>

#include <wheels/core/assert.hpp>

#include <vector>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

class StackAllocator final {
  using Stack = sure::Stack;

 public:
  Stack Allocate() {
    std::lock_guard lock(mutex_);

    ++metrics_.total_allocate;

    if (stack_pool_.empty()) {
      ++metrics_.allocate_new_count;
      metrics_.total_allocate_bytes += 16 * 1024 * 4;
      return AllocateNew();
    }

    return TakeFromPool();
  }

  void Release(Stack stack) {
    std::lock_guard lock(mutex_);

    ++metrics_.release_count;
    stack_pool_.push_back(std::move(stack));
  }

  AllocatorMetrics GetMetrics() {
    return metrics_;
  }

 private:
  static Stack AllocateNew() {
    static const size_t kStackPages = 16;  // 16 * 4KB = 64KB;
    return Stack::AllocateBytes(kStackPages * 1024 * 4);
  }

  Stack TakeFromPool() {
    WHEELS_ASSERT(!stack_pool_.empty(), "Stack is empty");
    Stack stack = std::move(stack_pool_.back());
    stack_pool_.pop_back();
    return stack;
  }

 private:
  std::mutex mutex_;
  std::vector<Stack> stack_pool_;
  AllocatorMetrics metrics_;
};

} // namespace detail

//////////////////////////////////////////////////////////////////////

detail::StackAllocator allocator;

sure::Stack AllocateStack() {
  return allocator.Allocate();
}

void ReleaseStack(sure::Stack stack) {
  allocator.Release(std::move(stack));
}

AllocatorMetrics GetAllocatorMetrics() {
  return allocator.GetMetrics();
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic
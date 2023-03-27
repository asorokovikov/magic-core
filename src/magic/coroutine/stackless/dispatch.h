#pragma once

#include <magic/coroutine/stackless/task.h>
#include <magic/executors/executor.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

class DispatchAwaiter final : public TaskNode {
 public:
  DispatchAwaiter(IExecutor& executor) : executor_(executor) {
  }

  bool await_ready() {
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) {
    handle_ = handle;
    executor_.Execute(this);
  }

  void await_resume() {
  }

  void Run() noexcept override {
    handle_.resume();
  }

  void Discard() noexcept override {
    handle_.destroy();
  }

 private:
  IExecutor& executor_;
  std::coroutine_handle<> handle_;
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

// Reschedule current coroutine to executor `target`
inline auto DispatchTo(IExecutor& target) {
  return detail::DispatchAwaiter(target);
}

}  // namespace magic
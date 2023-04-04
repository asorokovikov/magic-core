#pragma once

#include <magic/executors/task.h>

#include <wheels/logging/logging.hpp>
#include <wheels/core/exception.hpp>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

template <typename Func>
class DefaultTask final : public TaskNode {
 public:

  static DefaultTask* Create(Func func) {
    return new DefaultTask(std::move(func));
  }

  void Run() noexcept override {
    try {
      func_();
    } catch (...) {
      LOG_DEBUG("An error occurred while executing a task: " << wheels::CurrentExceptionMessage());
    }
    DestroySelf();
  }

  void Discard() noexcept override {
    DestroySelf();
  }

 private:
  DefaultTask(Func func) : func_(std::move(func)) {
  }

  void DestroySelf() {
    delete this;
  }

 private:
  Func func_;
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

template <typename Func>
TaskNode* CreateTask(Func func) {
  return detail::DefaultTask<Func>::Create(std::move(func));
}

}  // namespace magic
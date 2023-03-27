#pragma once

#include <magic/common/result.h>
#include <magic/common/result/make.h>

#include <coroutine>
#include <optional>
#include <variant>

namespace magic {

//////////////////////////////////////////////////////////////////////

using Unit = std::monostate;

template <typename T = Unit>
struct Task {
  struct Promise {
    // NOLINTNEXTLINE
    auto get_return_object() {
      return Task{std::coroutine_handle<Promise>::from_promise(*this)};
    }

    // NOLINTNEXTLINE
    std::suspend_always initial_suspend() noexcept {
      return {};
    }

    // NOLINTNEXTLINE
    std::suspend_never final_suspend() noexcept {
      return {};
    }

    // NOLINTNEXTLINE
    void set_exception(std::exception_ptr ex) {
      result = Fail(ex);
    }

    // NOLINTNEXTLINE
    void unhandled_exception() {
      result = CurrentException();
    }

    // NOLINTNEXTLINE
    void return_void() {
      result = Ok();
    }

    std::optional<Status> result;
  };

  using CoroutineHandle = std::coroutine_handle<Promise>;

  Task(CoroutineHandle handle) : handle_(handle) {
  }

  Task(Task&&) = default;

  // Non-copyable
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  CoroutineHandle ReleaseCoroutine() {
    return std::exchange(handle_, CoroutineHandle());
  }

  ~Task() {
    if (handle_ && !handle_.done()) {
      std::terminate();
    }
  }

 private:
  CoroutineHandle handle_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic

template <typename T, typename... Args>
struct std::coroutine_traits<magic::Task<T>, Args...> {
  using promise_type = typename magic::Task<T>::Promise;  // NOLINT
};

template <typename... Args>
struct std::coroutine_traits<void, Args...> {
  using promise_type = typename magic::Task<>::Promise;
};
#pragma once

#include <coroutine>
#include <future>

//////////////////////////////////////////////////////////////////////

namespace detail {

}  // namespace detail

template <typename T>
struct StdFutureCoroutinePromise {
  std::promise<T> promise_;

  auto get_return_object() {
    return promise_.get_future();
  }

  std::suspend_never initial_suspend() const noexcept {
    return {};
  }

  std::suspend_never final_suspend() const noexcept {
    return {};
  }

  void set_exception(std::exception_ptr ex) {
    promise_.set_exception(std::move(ex));
  }

  void unhandled_exception() {
    promise_.set_exception(std::current_exception());
  }

  void return_value(T value) {
    promise_.set_value(std::move(value));
  }
};

template<>
struct StdFutureCoroutinePromise<void> {
  std::promise<void> promise_;

  auto get_return_object() {
    return promise_.get_future();
  }

  std::suspend_never initial_suspend() const noexcept {
    return {};
  }

  std::suspend_never final_suspend() const noexcept {
    return {};
  }

  void set_exception(std::exception_ptr ex) {
    promise_.set_exception(std::move(ex));
  }

  void unhandled_exception() {
    promise_.set_exception(std::current_exception());
  }

  void return_void() {
    promise_.set_value();
  }
};

//////////////////////////////////////////////////////////////////////

template <typename R, typename... Args>
struct std::coroutine_traits<std::future<R>, Args...> {
  using promise_type = StdFutureCoroutinePromise<R>;
};

template <typename... Args>
struct std::coroutine_traits<void, Args...> {
  using promise_type = StdFutureCoroutinePromise<void>;
};
#pragma once

#include <magic/coroutine/core/impl.h>
#include <magic/fibers/core/stack.h>
#include <magic/concurrency/local/ptr.h>

#include <optional>

namespace magic {

//////////////////////////////////////////////////////////////////////

template <typename T>
class Generator;

// Shortcut
template <typename T>
void Send(T value) {
  Generator<T>::Send(std::move(value));
}

//////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
ThreadLocalPtr<Generator<T>> this_thread_generator;

}  // namespace detail

//////////////////////////////////////////////////////////////////////

template <typename T>
class Generator final {
  using Stack = sure::Stack;
  using Coroutine = detail::CoroutineImpl;

 public:
  Generator(Routine routine)
      : stack_(AllocateStack()), coroutine_(std::move(routine), stack_.MutView()) {
  }

  // ~ Public Interface

  std::optional<T> Receive() {
    if (GenerateNextValue()) {
      return value_;
    }
    return std::nullopt;
  }

  static void Send(T value) {
    auto generator = GetCurrentGenerator();
    generator->SetValue(std::move(value));
  }

  //////////////////////////////////////////////////////////////////////

 private:
  static Generator* GetCurrentGenerator() {
    assert(detail::this_thread_generator<T>);
    return detail::this_thread_generator<T>;
  }

  void SetValue(T value) {
    value_ = std::move(value);
    coroutine_.Suspend();
  }

  bool GenerateNextValue() {
    if (coroutine_.IsCompleted()) {
      return false;
    }

    auto prev = detail::this_thread_generator<T>.Exchange(this);
    coroutine_.Resume();
    detail::this_thread_generator<T> = prev;

    return !coroutine_.IsCompleted();
  }

 private:
  Stack stack_;
  Coroutine coroutine_;
  std::optional<T> value_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
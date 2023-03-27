#pragma once

#include <magic/coroutine/core/impl.h>
#include <magic/fibers/core/stack.h>
#include <magic/concurrency/local/ptr.h>

#include <optional>

namespace magic {

//////////////////////////////////////////////////////////////////////

template <typename T>
class Processor;

// Shortcut
template <typename T>
std::optional<T> Receive() {
  return Processor<T>::Receive();
}

//////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
ThreadLocalPtr<Processor<T>> this_thread_processor;

} // namespace detail

//////////////////////////////////////////////////////////////////////

// Processor

template <typename T>
class Processor final {
  using Stack = sure::Stack;
  using Coroutine = detail::CoroutineImpl;

 public:
  explicit Processor(Routine routine)
      : stack_(AllocateStack()),
        coroutine_(std::move(routine), stack_.MutView()) {
  }

  // ~ Public Interface

  void Send(T value) {
    SetValue(std::move(value));
  }

  void Close() {
    SetValue(std::nullopt);
  }

  static std::optional<T> Receive() {
    auto processor = GetCurrentProcessor();
    while (!processor->HasNext()) {
      processor->Suspend();
    }

    return processor->TakeValue();
  }

  //////////////////////////////////////////////////////////////////////

 private:
  static Processor<T>* GetCurrentProcessor() {
    WHEELS_VERIFY(detail::this_thread_processor<T>, "Not a processor context");
    return detail::this_thread_processor<T>;
  }

  bool HasNext() const {
    return has_next_value_;
  }

  void Suspend() {
    coroutine_.Suspend();
  }

  void SetValue(std::optional<T> value) {
    auto prev = detail::this_thread_processor<T>.Exchange(this);

    value_ = std::move(value);
    has_next_value_ = true;
    coroutine_.Resume();

    detail::this_thread_processor<T> = prev;
  }

  std::optional<T> TakeValue() {
    WHEELS_ASSERT(has_next_value_, "Doesn't have the next value");
    has_next_value_ = false;
    return std::move(value_);
  }

 private:
  Stack stack_;
  Coroutine coroutine_;
  std::optional<T> value_;

  bool has_next_value_ = false;
};

//////////////////////////////////////////////////////////////////////


}  // namespace magic
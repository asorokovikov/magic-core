#pragma once

#include <magic/futures/core/callback.h>

#include <magic/common/result.h>
#include <magic/executors/executor.h>

#include <wheels/core/assert.hpp>

#include <atomic>
#include <optional>
#include <memory>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

// Shared state between Future and Promise

template <typename T>
class SharedState {
  enum States {
    Start = 0,
    OnlyResult = 1,
    OnlyCallback = 2,
    Finish = 3,
  };

 public:
  explicit SharedState(IExecutor* executor) : executor_(executor), state_(States::Start) {
  }

  bool HasResult() const {
    auto state = state_.load(std::memory_order::acquire);
    return state == States::OnlyResult || state == States::Finish;
  }

  bool HasCallback() const {
    auto state = state_.load(std::memory_order::acquire);
    return state == States::OnlyCallback || state == States::Finish;
  }

  std::optional<Result<T>> GetResult() {
    return std::move(result_);
  }

  void SetExecutor(IExecutor* executor) {
    executor_ = executor;
  }

  IExecutor& GetExecutor() const {
    return *executor_;
  }

  void SetResult(Result<T> result) {
    WHEELS_VERIFY(!HasResult(), "Result is already added");

    result_.emplace(std::move(result));
    auto state = state_.load(std::memory_order::acquire);

    if (state == States::Start) {
      if (state_.compare_exchange_strong(state, States::OnlyResult, std::memory_order::release,
                                         std::memory_order::acquire)) {
        return;
      }
      assert(state == States::OnlyCallback);
    }

    if (state == States::OnlyCallback) {
      state_.store(States::Finish, std::memory_order::relaxed);
      InvokeCallback();
      return;
    }

    WHEELS_UNREACHABLE();
  }

  void SetCallback(Callback<T> callback) {
    WHEELS_VERIFY(!HasCallback(), "Callback is already added");
    callback_ = std::move(callback);
    auto state = state_.load(std::memory_order::acquire);

    if (state == States::Start) {
      if (state_.compare_exchange_strong(state, States::OnlyCallback)) {
        return;
      }
      assert(state == States::OnlyResult);
    }

    if (state == States::OnlyResult) {
      state_.store(States::Finish, std::memory_order::relaxed);
      InvokeCallback();
      return;
    }

    WHEELS_UNREACHABLE();
  }

 private:
  void InvokeCallback() {
    Execute(*executor_, [cb = std::move(callback_), res = std::move(*result_)]() mutable {
      cb(std::move(res));
    });
//    Execute(*executor_, [cb = callback_, res = std::move(*result_)]() mutable {
//      cb->Invoke(std::move(res));
//    });
  }

 private:
  IExecutor* executor_;
  Callback<T> callback_;
  std::optional<Result<T>> result_;
  std::atomic<States> state_;
};

//////////////////////////////////////////////////////////////////////

template <typename T>
using StateRef = std::shared_ptr<SharedState<T>>;

template <typename T>
auto MakeSharedState(IExecutor& executor) {
  return std::make_shared<SharedState<T>>(&executor);
}

//////////////////////////////////////////////////////////////////////

// Common base for Future and Promise
template <typename T>
class HoldState {
  using State = SharedState<T>;

 protected:
  explicit HoldState(StateRef<T> state) : state_(std::move(state)) {
  }

  // Movable
  HoldState(HoldState&& that) = default;
  HoldState& operator=(HoldState&& that) = default;

  // Non-copyable
  HoldState(const HoldState& that) = delete;
  HoldState& operator=(const HoldState& that) = delete;

  bool HasState() const {
    return (bool)state_;
  }

  const State& AccessState() const {
    Verify();
    return *state_.get();
  }

  StateRef<T> ReleaseState() {
    Verify();
    return std::move(state_);
  }

 private:
  void Verify() const {
    WHEELS_VERIFY(HasState(), "No shared state. Most likely shared state was released");
  }

 protected:
  StateRef<T> state_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace detail
}  // namespace magic
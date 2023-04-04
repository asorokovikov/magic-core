#pragma once

#include <magic/futures/core/callback.h>

#include <magic/common/result.h>
#include <magic/concurrency/rendezvous.h>
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
 public:
  explicit SharedState(IExecutor* executor) : executor_(executor) {
  }

  bool HasResult() const {
    return state_.Produced();
  }

  Result<T> GetResult() {
    WHEELS_VERIFY(state_.Consume(), "Future is not completed");
    return std::move(*result_);
  }

  void SetExecutor(IExecutor* executor) {
    executor_ = executor;
  }

  IExecutor& GetExecutor() const {
    return *executor_;
  }

  // Wait-free
  void SetResult(Result<T>&& result) {
    result_.emplace(std::move(result));
    if (state_.Produce()) {
      InvokeCallback();
    }
  }

  void SetCallback(CallbackBase<T>* callback) {
    WHEELS_ASSERT(callback, "Expected not null");
    callback_ = callback;
    if (state_.Consume()) {
      InvokeCallback();
    }
  }

 private:
  void InvokeCallback() {
    callback_->SetResult(std::move(*result_));
    executor_->Execute(callback_);
  }

 private:
  IExecutor* executor_;
  CallbackBase<T>* callback_ = nullptr;
  std::optional<Result<T>> result_;
  Rendezvous state_;
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
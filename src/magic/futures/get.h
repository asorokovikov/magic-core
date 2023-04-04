#pragma once

#include <magic/futures/core/future.h>
#include <magic/concurrency/oneshotevent.h>

namespace magic::futures {

namespace detail {

template <typename T>
class BlockingGetter : public CallbackBase<T> {
 public:
  explicit BlockingGetter(Future<T> future) : future_(std::move(future)) {
  }

  Result<T> WaitAndGetResult() {
    std::move(future_).Subscribe(this);
    event_.Wait();
    return std::move(*result_);
  }

  // ICallback
  void Invoke(Result<T> result) noexcept override {
    result_.emplace(std::move(result));
    event_.Fire();
  }

 private:
  Future<T> future_;
  OneShotEvent event_;
  std::optional<Result<T>> result_;
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

template <typename T>
Result<T> WaitResult(Future<T>&& future) {
  detail::BlockingGetter<T> getter(std::move(future));
  return getter.WaitAndGetResult();
}

template <typename T>
T WaitValue(Future<T>&& future) {
  return WaitResult(std::move(future)).ValueOrThrow();
}

}  // namespace magic::futures
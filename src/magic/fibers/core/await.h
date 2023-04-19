#pragma once

#include <magic/fibers/api.h>
#include <magic/fibers/core/awaiter.h>
#include <magic/futures/core/future.h>

namespace magic::detail {

template <typename T>
class FutureAwaiter : public IAlwaysSuspendAwaiter, public CallbackBase<T> {
 public:
  explicit FutureAwaiter(Future<T>&& future) : future_(std::move(future)) {
  }

  // Awaiter protocol

  bool AwaitReady() {
    if (future_.IsReady()) {
      result_.emplace(std::move(future_).GetResult());
      return true;
    }
    return false;
  }

  void AwaitSuspend(FiberHandle handle) override {
    fiber_ = handle;
    std::move(future_).Subscribe(this);
  }

  Result<T> GetResult() {
    return std::move(*result_);
  }

  // ICallback
  void Invoke(Result<T> result) noexcept override {
    result_.emplace(std::move(result));
    fiber_.Resume();
  }

 private:
  Future<T> future_;
  FiberHandle fiber_;
  std::optional<Result<T>> result_;
};

template <typename T>
auto GetAwaiter(Future<T>&& f) {
  return FutureAwaiter<T>(std::move(f));
}

template <typename Awaitable>
auto Await(Awaitable&& awaitable) {
  auto awaiter = GetAwaiter(std::forward<Awaitable>(awaitable));

  if (!awaiter.AwaitReady()) {
    self::Suspend(awaiter);
  }

  return awaiter.GetResult();
}

}  // namespace magic
#pragma once

#include <magic/futures/core/future.h>
#include <magic/concurrency/oneshotevent.h>

namespace magic::futures {

//////////////////////////////////////////////////////////////////////

template <typename T>
Result<T> WaitResult(Future<T>&& future) {
  auto event = OneShotEvent();
  auto result = std::optional<Result<T>>();

  std::move(future).Subscribe([&](auto value) mutable {
    result.emplace(std::move(value));
    event.Fire();
  });
  event.Wait();

  return std::move(result.value());
}

template <typename T>
T WaitValue(Future<T>&& future) {
  return WaitResult(std::move(future)).ValueOrThrow();
}

}  // namespace magic::futures
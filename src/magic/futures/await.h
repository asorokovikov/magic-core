#pragma once

#include <magic/futures/get.h>
#include <magic/fibers/core/await.h>

namespace magic {

template <typename T>
T Await(Future<T>&& future) {
  if (self::IsFiber()) {
    return detail::Await(std::move(future));
  }
  else {
    // Block current thread
    return futures::WaitValue(std::move(future));
  }
}

}  // namespace magic
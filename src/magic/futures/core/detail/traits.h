#pragma once

#include <magic/common/result.h>

namespace magic {

template <typename T>
class Future;

//////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
struct Flatten {
  using ValueType = T;
};

template <typename T>
struct Flatten<Future<T>> {
  using ValueType = typename Flatten<T>::ValueType;
};

//////////////////////////////////////////////////////////////////////

}  // namespace detail
}  // namespace magic
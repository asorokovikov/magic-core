#pragma once

#include <magic/common/result.h>
#include <magic/common/result/make.h>
#include <magic/fibers/core/fiber.h>

#include <mutex>
#include <optional>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

class ParkingLot final {
 public:
  void Park();
  void Wake();

 private:
  Fiber* fiber_ = nullptr;
  std::mutex mutex_;
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

template <typename T>
class FutureLite final {
 public:
  Result<T> Get() {
    if (result_.has_value()) {
      return *result_;
    }
    park_.Park();

    return result_.value();
  }

  void SetValue(T value) {
    Set(Ok(value));
  }

  void SetError(std::error_code ec) {
    Set(Fail(ec));
  }

  void SetException(std::exception_ptr ex) {
    Set(Fail(ex));
  }

 private:
  void Set(Result<T>&& result) {
    result_ = std::move(result);
    park_.Wake();
  }

 private:
  detail::ParkingLot park_;
  std::optional<Result<T>> result_{std::nullopt};
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
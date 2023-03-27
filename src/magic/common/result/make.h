#pragma once

#include <magic/common/result.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

class [[nodiscard]] Failure final {
 public:
  explicit Failure(std::error_code& error_code) : error_(error_code) {
  }

  explicit Failure(std::exception_ptr exception) : error_(std::move(exception)) {
  }

  explicit Failure(Error error) : error_(std::move(error)) {
  }

  Failure(const Failure&) = delete;
  Failure& operator=(const Failure&) = delete;

  Failure(Failure&&) = delete;
  Failure& operator=(Failure&&) = delete;

  template <typename T>
  operator Result<T>() {
    return Result<T>::Fail(error_);
  }

 private:
  const Error error_;
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

namespace make_result {

template <typename T>
Result<T> Ok(T&& value) {
  return Result<T>::Ok(std::move(value));
}

template <typename T>
Result<T> Ok(T& value) {
  return Result<T>::Ok(value);
}

template <typename T>
Result<T> Ok(const T& value) {
  return Result<T>::Ok(value);
}

// Usage: make_result::Ok()
Status Ok();

// Usage: make_result::Fail(error)
detail::Failure Fail(std::error_code error);

detail::Failure Fail(Error error);

detail::Failure CurrentException();

// Produce std::errc::not_supported error
detail::Failure NotSupported();

template <typename T>
detail::Failure PropagateError(const Result<T>& result) {
  return detail::Failure{result.Error()};
}

Status ToStatus(std::error_code error);

template <typename T>
Status JustStatus(const Result<T>& result) {
  if (result.IsOk()) {
    return Ok();
  } else {
    return Status::Fail(result.Error());
  }
}

// Make result with exception
template <typename E, typename... Args>
detail::Failure Throw(Args&&... args) {
  try {
    throw E(std::forward<Args>(args)...);
  } catch (const E& e) {
    return detail::Failure{std::current_exception()};
  }
}

}  // namespace make_result

//////////////////////////////////////////////////////////////////////

}  // namespace magic

using magic::make_result::Fail;
using magic::make_result::JustStatus;
using magic::make_result::NotSupported;
using magic::make_result::Ok;
using magic::make_result::PropagateError;
using magic::make_result::ToStatus;
using magic::make_result::CurrentException;
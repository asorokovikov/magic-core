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

template <typename R>
struct Invoker {
  template <typename F, typename... Args>
  static Result<R> Invoke(F&& f, Args&&... args) {
    try {
      return Result<R>::Ok(f(std::forward<Args>(args)...));
    } catch (...) {
      return Result<R>::Fail({std::current_exception()});
    }
  }
};

template <>
struct Invoker<void> {
  template <typename F, typename... Args>
  static Status Invoke(F&& f, Args&&... args) {
    try {
      f(std::forward<Args>(args)...);
      return Status::Ok();
    } catch (...) {
      return Status::Fail({std::current_exception()});
    }
  }
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
detail::Failure NotImplemented();

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

template <typename F, typename... Args>
auto Invoke(F&& f, Args&&... args) {
  using R = decltype(f(std::forward<Args>(args)...));
  return detail::Invoker<R>::Invoke(std::forward<F>(f),
                                    std::forward<Args>(args)...);
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
using magic::make_result::JustStatus;
using magic::make_result::Ok;
using magic::make_result::PropagateError;
using magic::make_result::ToStatus;
using magic::make_result::CurrentException;
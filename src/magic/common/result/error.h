#pragma once

#include <cassert>
#include <variant>
#include <exception>
#include <system_error>

namespace magic {

//////////////////////////////////////////////////////////////////////

class Error final {
 public:
  Error() : error_(std::monostate{}) {
  }
  Error(std::exception_ptr ep) : error_(std::move(ep)) {
  }
  Error(std::error_code ec) : error_(std::move(ec)) {
  }

  bool HasException() const {
    return std::holds_alternative<std::exception_ptr>(error_);
  }

  bool HasErrorCode() const {
    return std::holds_alternative<std::error_code>(error_);
  }

  bool HasError() const {
    return HasErrorCode() || HasException();
  }

  auto ErrorCode() const -> std::error_code {
    assert(HasErrorCode());
    return std::get<std::error_code>(error_);
  }

  void ThrowIfError() const {
    if (HasException()) {
      std::rethrow_exception(std::get<std::exception_ptr>(error_));
    } else if (HasErrorCode()) {
      throw std::system_error(std::get<std::error_code>(error_));
    }
  }

 private:
  std::variant<std::monostate, std::exception_ptr, std::error_code> error_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
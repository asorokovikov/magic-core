#pragma once

#include <exception>
#include <system_error>

namespace magic {

namespace curl::easy {
const std::error_category& CURLCategory();

inline std::error_code make_error_code(int error) {
  return std::error_code(error, CURLCategory());
}
}  // namespace curl::easy

//////////////////////////////////////////////////////////////////////

class BaseException : public std::exception {
 public:
  BaseException() = default;
  BaseException(std::string message) : message_(std::move(message)) {
  }
  ~BaseException() override = default;

  const char* what() const noexcept override {
    return message_.c_str();
  }

 private:
  std::string message_;
};

class BaseCodeException : public BaseException {
 public:
  BaseCodeException(std::error_code ec, std::string_view msg, std::string_view url);
  ~BaseCodeException() override = default;

  const std::error_code& ErrorCode() const noexcept {
    return error_;
  }

 private:
  std::error_code error_;
};

class TimeoutException : public BaseException {
 public:
  using BaseException::BaseException;
  ~TimeoutException() override = default;
};

class SSLException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~SSLException() override = default;
};

class TechnicalError : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~TechnicalError() override = default;
};

class BadArgumentException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~BadArgumentException() override = default;
};

class TooManyRedirectsException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~TooManyRedirectsException() override = default;
};

class NetworkProblemException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~NetworkProblemException() override = default;
};

class DNSProblemException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~DNSProblemException() override = default;
};

class AuthFailedException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~AuthFailedException() override = default;
};



}  // namespace magic

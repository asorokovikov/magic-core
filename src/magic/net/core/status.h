#pragma once

#include <string>

namespace magic {

/// https://en.wikipedia.org/wiki/List_of_HTTP_status_codes

struct HttpStatus {
  enum class Code : uint16_t {
    Invalid = 0,
    OK = 200,
    Created = 201,
    NoContent = 204,
    BadRequest = 400,
    NotFound = 404,
    Conflict = 409,
    InternalServerError = 500,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    InsufficientStorage = 507,
    BandwidthLimitExceeded = 509,
    WebServerIsDown = 520,
    ConnectionTimedOut = 522,
    OriginIsUnreachable = 523,
    TimeoutOccurred = 524,
    SslHandshakeFailed = 525,
    InvalidSslCertificate = 526,
  };

  // ~ Static constructors

  static HttpStatus FromStatusCode(int code) {
    return HttpStatus{static_cast<Code>(code)};
  }

  static HttpStatus Ok() {
    return {Code::OK};
  }

  static HttpStatus NotFound() {
    return {Code::NotFound};
  }

  static HttpStatus BadRequest() {
    return {Code::BadRequest};
  }
  
  // ~ Public Properties

  const Code Value;

  // ~ Public Methods

  std::string ToString() const;

  // ~ Operators

  constexpr bool operator==(HttpStatus::Code code) const {
    return Value == code;
  }

  constexpr bool operator!=(HttpStatus::Code code) const {
    return Value != code;
  }

};

}  // namespace magic

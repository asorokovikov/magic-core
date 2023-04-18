#include <magic/net/core/status.h>

namespace magic {

std::string HttpStatus::ToString() const {
  switch (Value) {
    case Code::Invalid:
      return "Invalid status";
    case Code::OK:
      return "200 OK";
    case Code::Created:
      return "201 Created";
    case Code::NoContent:
      return "204 NoContent";
    case Code::BadRequest:
      return "400 BadRequest";
    case Code::NotFound:
      return "404 Not Found";
    case Code::Conflict:
      return "409 Conflict";
    case Code::InternalServerError:
      return "500 Internal Server Error";
    case Code::BadGateway:
      return "502 Bad Gateway";
    case Code::ServiceUnavailable:
      return "503 Service Unavailable";
    case Code::GatewayTimeout:
      return "504 Gateway Timeout";
    case Code::InsufficientStorage:
      return "507 Insufficient Storage";
    case Code::BandwidthLimitExceeded:
      return "509 Bandwidth Limit Exceeded";
    case Code::WebServerIsDown:
      return "520 Web Server Is Down";
    case Code::ConnectionTimedOut:
      return "522 Connection Timed Out";
    case Code::OriginIsUnreachable:
      return "523 Origin Is Unreachable";
    case Code::TimeoutOccurred:
      return "524 A Timeout Occurred";
    case Code::SslHandshakeFailed:
      return "525 SSL Handshake Failed";
    case Code::InvalidSslCertificate:
      return "526 Invalid SSL Certificate";
  }

  return "Unknown HttpStatusCode (" + std::to_string(static_cast<uint16_t>(Value)) + ")";
}

}  // namespace magic
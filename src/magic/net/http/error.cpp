#include <magic/net/http/error.h>

#include <curl/curl.h>
#include <fmt/format.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace curl::easy {

namespace detail {

class CurlCategory : public std::error_category {
 public:
  const char* name() const noexcept override {
    return "CurlCategory";
  }

  std::string message(int value) const override {
    return curl_easy_strerror(static_cast<CURLcode>(value));
  }
};

}  // namespace detail

const std::error_category& CURLCategory() {
  static detail::CurlCategory instance;
  return instance;
}

}  // namespace curl::easy

//////////////////////////////////////////////////////////////////////

BaseCodeException::BaseCodeException(std::error_code ec, std::string_view msg, std::string_view url)
    : BaseException(fmt::format("{}, curl_error: {}, url: {}", msg, ec.message(), url)),
      error_(ec)
{
}

}  // namespace magic
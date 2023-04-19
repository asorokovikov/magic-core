#pragma once

#include <magic/net/http/request.h>
#include <magic/net/http/error.h>

#include <fmt/core.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

class HttpRequestBuilder final {
 public:
  HttpRequestBuilder() {
    Init();
  }

  // Non-copyable
  HttpRequestBuilder(const HttpRequestBuilder&) = delete;
  HttpRequestBuilder& operator=(const HttpRequestBuilder&) = delete;

  HttpRequestBuilder& SetContent(const std::string body) {
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.length()));
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());

    return *this;
  }

  HttpRequestBuilder& Get(std::string url) {
    return SetUrl(std::move(url));
  }

  HttpRequestBuilder& Post(std::string url, std::string content) {
    SetUrl(std::move(url));
    content_ = std::move(content);
    auto result = curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());
    result = curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, content_.c_str());

    return *this;
  }

  HttpRequest Build() {
    auto curl = std::exchange(curl_, nullptr);

    return Ok(HttpRequest(curl));
  }

  ~HttpRequestBuilder() {
    if (curl_ != nullptr) {
      fmt::println("free");
      curl_easy_cleanup(curl_);
    }
  }

  HttpRequestBuilder& SetUrl(std::string url) {
    url_ = std::move(url);
    curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());
    return *this;
//    if (error != CURLE_OK) {
//      auto ec = curl::easy::make_error_code(error);
//      throw BadArgumentException(ec, "Bad URL", url);
//    }
  }

 private:
  void Init() {
    curl_ = curl_easy_init();
    assert(curl_);
  }

 private:
  CURL* curl_;
  CURLcode result_;
  std::string url_;
  std::string content_;
};

}  // namespace magic
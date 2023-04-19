#pragma once

#include <curl/curl.h>
#include <cassert>

#include <magic/common/result.h>
#include <magic/common/result/make.h>

#include <magic/net/http/status.h>
#include <magic/net/http/response.h>
#include <magic/net/http/error.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

static size_t WriteFunction(char* ptr, size_t size, size_t nmemb, std::string* data) {
  size_t real_size = size * nmemb;
  data->append(ptr, real_size);
  return real_size;
}

}  // namespace detail

//////////////////////////////////////////////////////////////////////

class HttpRequest final {
 public:
  HttpRequest(CURL* curl) : curl_(curl) {
    assert(curl_);
  }

  ~HttpRequest() {
    curl_easy_cleanup(curl_);
  }

  // Non-copyable
  HttpRequest(const HttpRequest& that) = delete;
  HttpRequest& operator=(const HttpRequest& that) = delete;

  // Movable
  HttpRequest(HttpRequest&& that) {
    std::swap(curl_, that.curl_);
  }

  HttpRequest& operator=(HttpRequest&& that) {
    std::swap(curl_, that.curl_);
    return *this;
  }


  Result<HttpResponse> Run() {
    auto content = std::string();
    auto header = std::string();

    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, detail::WriteFunction);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &content);

    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, detail::WriteFunction);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header);

    const auto result = curl_easy_perform(curl_);

    if (result != CURLE_OK) {
      return Fail(curl::easy::make_error_code(result));
    }

    int64_t response_code;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);

    double total_time;
    curl_easy_getinfo(curl_, CURLINFO_TOTAL_TIME, &total_time);
    auto elapsed = ToTimeSpan(total_time);

    char* url_string{nullptr};
    curl_easy_getinfo(curl_, CURLINFO_EFFECTIVE_URL, &url_string);

    // Metrics

    double downloaded, uploaded, average_download_speed, average_upload_speed;
    curl_easy_getinfo(curl_, CURLINFO_SIZE_DOWNLOAD, &downloaded);
    curl_easy_getinfo(curl_, CURLINFO_SIZE_UPLOAD, &uploaded);
    curl_easy_getinfo(curl_, CURLINFO_SPEED_DOWNLOAD, &average_download_speed);
    curl_easy_getinfo(curl_, CURLINFO_SPEED_UPLOAD, &average_upload_speed);

    auto metrics = HttpRequestMetrics{.DownloadBytes = downloaded,
                                      .UploadBytes = uploaded,
                                      .AverageDownloadSpeed = average_download_speed,
                                      .AverageUploadSpeed = average_upload_speed,
                                      .Duration = elapsed};

    return Ok(HttpResponse{HttpStatus::FromStatusCode(response_code), HttpHeader::Parse(header),
                           std::move(metrics), std::string(url_string), std::move(content)});
  }

 private:
  CURL* curl_ = nullptr;
};

}  // namespace magic
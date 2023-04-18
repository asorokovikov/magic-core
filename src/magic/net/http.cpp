#include <magic/net/http.h>
#include <magic/futures/execute.h>
#include <magic/executors/thread_pool.h>
#include <magic/common/stopwatch.h>

#include <curl/curl.h>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/std.h>

#include <memory>

//////////////////////////////////////////////////////////////////////

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
  HttpRequest(const HttpRequest&) = delete;
  HttpRequest& operator=(const HttpRequest&) = delete;

  HttpResponse Run() {
    auto content = std::string();
    auto header = std::string();

    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, detail::WriteFunction);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &content);

    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, detail::WriteFunction);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header);

    const auto result = curl_easy_perform(curl_);

    if (result != CURLE_OK) {
      fmt::println("curl_easy_perform() failed: {}", curl_easy_strerror(result));
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

    return {HttpStatus::FromStatusCode(response_code), HttpHeader::Parse(header),
            std::move(metrics), std::string(url_string), std::move(content)};
  }

 private:
  CURL* curl_;
};

class HttpRequestBuilder final {
 public:
  HttpRequestBuilder() {
    Init();
  }

  // Non-copyable
  HttpRequestBuilder(const HttpRequestBuilder&) = delete;
  HttpRequestBuilder& operator=(const HttpRequestBuilder&) = delete;

  HttpRequestBuilder& SetUrl(std::string url) {
    url_ = std::move(url);
    auto res = curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());
    if (res != CURLE_OK) {
      fmt::println("curl_easy_perform() failed: {}", curl_easy_strerror(res));
    }
    return *this;
  }

  HttpRequest Build() {
    auto curl = std::exchange(curl_, nullptr);
    return HttpRequest(curl);
  }

  ~HttpRequestBuilder() {
    if (curl_ != nullptr) {
      curl_easy_cleanup(curl_);
    }
  }

 private:
  void Init() {
    curl_ = curl_easy_init();
    assert(curl_);
  }

 private:
  CURL* curl_;
  std::string url_;
};

//////////////////////////////////////////////////////////////////////

HttpResponse Http::Get(std::string url) {
  auto request = HttpRequestBuilder().SetUrl(std::move(url)).Build();
  auto response = request.Run();
  return response;
}

Future<HttpResponse> Http::GetAsync(std::string url) {
  return GetAsync(*ThreadPool::Current(), std::move(url));
}

Future<HttpResponse> Http::GetAsync(IExecutor& executor, std::string url) {
  return futures::Execute(executor, [url = std::move(url)]() {
    auto result = Http::Get(url);
    return result;
  });
}

Future<std::string> Http::GetStringAsync(std::string url) {
  return std::move(GetAsync(std::move(url))).Then([](HttpResponse response) {
    return response.GetContent();
  });
}

}  // namespace magic

#include <magic/net/http.h>
#include <magic/net/http/request_builder.h>

#include <magic/futures/execute.h>
#include <magic/executors/thread_pool.h>
#include <magic/common/stopwatch.h>

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/std.h>

#include <memory>

//////////////////////////////////////////////////////////////////////

namespace magic {

HttpResponse Http::Get(std::string url) {
  std::string response_header, response_content;

  CURL* curl = curl_easy_init();
  struct curl_slist* list = nullptr;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, detail::WriteFunction);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_content);

  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, detail::WriteFunction);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

  const auto result = curl_easy_perform(curl);

  if (result != CURLE_OK) {
    curl_easy_cleanup(curl);
    throw BaseException(curl_easy_strerror(result));
  }

  // HttpResponse

  int64_t response_code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

  double total_time;
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
  auto elapsed = ToTimeSpan(total_time);

  char* url_string{nullptr};
  curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url_string);
  auto effective_url = std::string(url_string);

  // Metrics

  double downloaded, uploaded, average_download_speed, average_upload_speed;
  curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &downloaded);
  curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, &uploaded);
  curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &average_download_speed);
  curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &average_upload_speed);

  auto metrics = HttpRequestMetrics{.DownloadBytes = downloaded,
                                    .UploadBytes = uploaded,
                                    .AverageDownloadSpeed = average_download_speed,
                                    .AverageUploadSpeed = average_upload_speed,
                                    .Duration = elapsed};

  // Cleanup

  curl_easy_cleanup(curl);

  return HttpResponse{HttpStatus::FromStatusCode(response_code), HttpHeader::Parse(response_header),
                      std::move(metrics), effective_url, std::move(response_content)};
}

HttpResponse Http::PostJson(std::string url, std::string json) {
  std::string response_header, response_content;

  CURL* curl = curl_easy_init();
  struct curl_slist* list = nullptr;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, detail::WriteFunction);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_content);

  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, detail::WriteFunction);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

  list = curl_slist_append(list, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

  const auto result = curl_easy_perform(curl);
  curl_slist_free_all(list);

  if (result != CURLE_OK) {
    curl_easy_cleanup(curl);
    throw BaseException(curl_easy_strerror(result));
  }

  // HttpResponse

  int64_t response_code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

  double total_time;
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
  auto elapsed = ToTimeSpan(total_time);

  char* url_string{nullptr};
  curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url_string);
  auto effective_url = std::string(url_string);

  // Metrics

  double downloaded, uploaded, average_download_speed, average_upload_speed;
  curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &downloaded);
  curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, &uploaded);
  curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &average_download_speed);
  curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &average_upload_speed);

  auto metrics = HttpRequestMetrics{.DownloadBytes = downloaded,
                                    .UploadBytes = uploaded,
                                    .AverageDownloadSpeed = average_download_speed,
                                    .AverageUploadSpeed = average_upload_speed,
                                    .Duration = elapsed};

  // Cleanup

  curl_easy_cleanup(curl);

  return HttpResponse{HttpStatus::FromStatusCode(response_code), HttpHeader::Parse(response_header),
                      std::move(metrics), effective_url, std::move(response_content)};
}

Future<HttpResponse> Http::PostJsonAsync(IExecutor& executor, std::string url, std::string json) {
  return futures::Execute(executor, [url = std::move(url), json = std::move(json)]() mutable {
    return Http::PostJson(std::move(url), std::move(json));
  });
}

Future<HttpResponse> Http::PostJsonAsync(std::string url, std::string json) {
  return PostJsonAsync(*ThreadPool::Current(), std::move(url), std::move(json));
}

Future<HttpResponse> Http::GetAsync(std::string url) {
  return GetAsync(*ThreadPool::Current(), std::move(url));
}

Future<HttpResponse> Http::GetAsync(IExecutor& executor, std::string url) {
  return futures::Execute(executor, [url = std::move(url)]() mutable {
    return Http::Get(std::move(url));
  });
}

Future<std::string> Http::GetStringAsync(std::string url) {
  return std::move(GetAsync(std::move(url))).Then([](HttpResponse response) {
    return response.GetContent();
  });
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic

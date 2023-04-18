#pragma once

#include <magic/net/core/status.h>
#include <magic/net/core/header.h>
#include <magic/common/time.h>

namespace magic {

struct HttpRequestMetrics {
  double DownloadBytes;
  double UploadBytes;
  double AverageDownloadSpeed;
  double AverageUploadSpeed;
  TimeSpan Duration;
};

class HttpResponse final {
 public:
  HttpResponse(HttpStatus status, HttpHeader header, HttpRequestMetrics metrics, std::string url,
               std::string content)
      : status_(status),
        header_(std::move(header)),
        metrics_(std::move(metrics)),
        url_(std::move(url)),
        content_(std::move(content)) {
  }

  bool IsOk() const {
    return status_ == HttpStatus::Code::OK;
  }

  HttpStatus GetStatus() const {
    return status_;
  }

  HttpHeader GetHeader() const {
  }

  std::string GetContent() const {
    return content_;
  }

  std::string ToString() const;

 private:
  const HttpStatus status_;
  const HttpHeader header_;
  const HttpRequestMetrics metrics_;
  const std::string url_;
  const std::string content_;
};

}  // namespace magic
#pragma once

#include <magic/net/core/response.h>
#include <magic/executors/executor.h>
#include <magic/futures/core/future.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

// Thread-safe methods that may be used concurrently from any thread

struct Http {

  // Send a GET request to the specified url.
  static HttpResponse Get(std::string url);

  // Send a GET request to the specified url as an asynchronous operation.
  static Future<HttpResponse> GetAsync(std::string url);

  // Send a GET request and return the response body as a string in an asynchronous operation.
  static Future<std::string> GetStringAsync(std::string url);

  // Send a GET request to the specified url as an asynchronous operation.
  static Future<HttpResponse> GetAsync(IExecutor& executor, std::string url);

};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
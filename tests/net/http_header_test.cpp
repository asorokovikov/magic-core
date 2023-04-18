#include <gtest/gtest.h>
#include "../test_helper.h"

#include <magic/common/cpu_time.h>
#include <magic/common/unit.h>

#include <magic/executors/thread_pool.h>

#include <magic/futures/get.h>
#include <magic/fibers/api.h>

#include <magic/net/http.h>
#include <magic/net/core/format.h>
#include <magic/net/core/header.h>

//////////////////////////////////////////////////////////////////////

TEST(HttpHeader, JustWork) {
  const auto source = R"(
    HTTP/1.1 200 OK
    X-Powered-By: Express
    Content-Type: application/json; charset=utf-8
    Content-Length: 388
    ETag: W/"184-btA3qP8tg9Wrq1KC5bpgvj92E8w"
    Date: Mon, 17 Apr 2023 18:32:32 GMT
    Connection: keep-alive
    Keep-Alive: timeout=5
  )";

  auto header = HttpHeader::Parse(source);

  ASSERT_EQ(header["Content-Length"], "388");
  ASSERT_EQ(header["Content-Type"], "application/json; charset=utf-8");
  ASSERT_EQ(header["X-Powered-By"], "Express");
  ASSERT_EQ(header["Date"], "Mon, 17 Apr 2023 18:32:32 GMT");
  ASSERT_EQ(header["Connection"], "keep-alive");
  ASSERT_EQ(header["Keep-Alive"], "timeout=5");
}
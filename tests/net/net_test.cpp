#include <gtest/gtest.h>
#include "../test_helper.h"

#include <magic/common/cpu_time.h>
#include <magic/common/unit.h>

#include <magic/executors/thread_pool.h>

#include <magic/futures/get.h>
#include <magic/fibers/api.h>

#include <magic/net/http.h>
#include <magic/net/core/format.h>

//////////////////////////////////////////////////////////////////////

TEST(Network, HttpStatus) {
  ASSERT_EQ(HttpStatus::NotFound().Value, HttpStatus::Code::NotFound);
  ASSERT_EQ(static_cast<uint16_t>(HttpStatus::NotFound().Value), 404);
  ASSERT_EQ(HttpStatus::FromStatusCode(404), HttpStatus::Code::NotFound);

  auto ok_status = HttpStatus::FromStatusCode(200);
  ASSERT_TRUE(ok_status == HttpStatus::Code::OK);
}

TEST(Network, JustWorks) {
  auto response = Http::Get("192.168.1.2:3000/api/books");
  fmt::println("\n{}", response);
}

TEST(Network, Async) {
  auto scheduler = ThreadPool(4);

  auto f = Http::GetAsync(scheduler, "https://jsonplaceholder.typicode.com/users/1");

  const auto response = futures::WaitValue(std::move(f));
  fmt::println("{}", response);

  scheduler.WaitIdle();
  scheduler.Stop();
}

TEST(Network, FiberAsync) {
  RunScheduler(4, []() {
    auto f = Http::GetAsync("http://neverssl.com").Then([](auto response) {
      fmt::println("Code: {}", response.GetStatus());
      return response;
    });

    futures::WaitValue(std::move(f));
  });
}
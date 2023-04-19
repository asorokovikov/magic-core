#include <gtest/gtest.h>
#include "../test_helper.h"

#include <magic/common/cpu_time.h>
#include <magic/common/unit.h>

#include <magic/executors/thread_pool.h>
#include <magic/futures/await.h>
#include <magic/fibers/api.h>

#include <magic/net/http.h>
#include <magic/net/http/format.h>
#include <magic/net/http/request_builder.h>

//////////////////////////////////////////////////////////////////////

TEST(Network, HttpStatus) {
  ASSERT_EQ(HttpStatus::NotFound().Value, HttpStatus::Code::NotFound);
  ASSERT_EQ(static_cast<uint16_t>(HttpStatus::NotFound().Value), 404);
  ASSERT_EQ(HttpStatus::FromStatusCode(404), HttpStatus::Code::NotFound);

  auto ok_status = HttpStatus::FromStatusCode(200);
  ASSERT_TRUE(ok_status == HttpStatus::Code::OK);
}

TEST(Network, PostJson) {
  const std::string endpoint = "192.168.1.2:3000/api/books/add";
  const auto json = "{\"title\":\"new book\",\"description\":\"nice book\"}"s;
  auto res = Http::PostJson(endpoint, json);
  fmt::println("{}", res);
}

TEST(Network, GetAsync) {
  const auto endpoint = "192.168.1.2:3000/api/books"s;

  RunScheduler(4, [&]() {
    try {
      auto f = Http::GetAsync(endpoint);
      auto response = Await(std::move(f));
      fmt::println("{}", response);

    } catch (std::exception& ex) {
      fmt::println("an error occurred: {}", ex.what());
    }
  });
}

TEST(Network, GetStringAsync) {
  const auto endpoint = "192.168.1.2:3000/api/books"s;

  RunScheduler(4, [&]() {
    try {
      auto f = Http::GetStringAsync(endpoint);
      auto content = Await(std::move(f));
      fmt::println("{}", content);

    } catch (std::exception& ex) {
      fmt::println("an error occurred: {}", ex.what());
    }
  });
}

TEST(Network, PostJsonAsync) {
  const auto endpoint = "192.168.1.2:3000/api/books"s;
  const auto json = R"({"title":"Teach Yourself C++ in 14 Days","description":"description"})"s;

  RunScheduler(4, [&]() {
    try {
      auto f = Http::PostJsonAsync(endpoint, json);
      auto response = Await(std::move(f));
      fmt::println("{}", response);

    } catch (std::exception& ex) {
      fmt::println("an error occurred: {}", ex.what());
    }
  });
}

TEST(Network, JustWorks) {
  const std::string endpoint = "192.168.1.2:3000/api/books/";
  {
    auto response = Http::PostJson(endpoint, "{\"project\":\"rapidjson\",\"stars\":10}");
    fmt::println("{}", response);
  }
}

TEST(Network, Async) {
  RunScheduler(4, []() {
      auto f = Http::GetAsync("https://jsonplaceholder.typicode.com/users/1");
      auto response = Await(std::move(f));
      fmt::println("{}", response);
  });
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
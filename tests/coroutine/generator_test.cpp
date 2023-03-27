#include <gtest/gtest.h>

#include <magic/coroutine/generator.h>

#include <fmt/core.h>
#include <chrono>
#include <thread>

using namespace magic;

Generator<int> Range(int start, int end) {
  return {[start, end]{
    for (int it = start; it < end; ++it) {
      Send<int>(it);
    }
  }};
}

//////////////////////////////////////////////////////////////////////

TEST(Generator, JustWorks) {
  auto generator = Generator<std::string>([&]{
    Send<std::string>("hello");
  });

  auto next = generator.Receive();
  ASSERT_TRUE(next);
  ASSERT_EQ(*next, "hello");

  next = generator.Receive();
  ASSERT_FALSE(next);

  next = generator.Receive();
  ASSERT_FALSE(next);
}

//////////////////////////////////////////////////////////////////////

TEST(Generator, Range) {
  auto index = 1;
  auto numbers = Range(1, 10);
  while (auto value = numbers.Receive()) {
    fmt::println("value: {}", *value);
    ASSERT_EQ(index++, *value);
  }
}

//////////////////////////////////////////////////////////////////////

TEST(Generator, Countdown) {
  Generator<int> countdown([]() {
    for (int i = 10; i >= 0; --i) {
      Send<int>(i);
    }
  });

  for (int i = 10; i >= 0; --i) {
    auto next = countdown.Receive();
    ASSERT_TRUE(next);
    ASSERT_EQ(*next, i);
  }

  ASSERT_FALSE(countdown.Receive());
}

//////////////////////////////////////////////////////////////////////

void Reverse(Generator<std::string>& g) {
  if (auto next = g.Receive()) {
    Reverse(g);
    Send(*next);
  }
}

TEST(Generator, Reverse) {
  Generator<std::string> generator([]() {
    Send<std::string>("Hello");
    Send<std::string>(",");
    Send<std::string>("World");
    Send<std::string>("!");
  });

  Generator<std::string> reverser([&generator]() {
    Reverse(generator);
  });

  std::stringstream out;
  while (auto msg = reverser.Receive()) {
    out << *msg;
  }

  ASSERT_EQ(out.str(), "!World,Hello");
}

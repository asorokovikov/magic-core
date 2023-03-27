#include <gtest/gtest.h>

#include <magic/coroutine/processor.h>

#include <fmt/core.h>
#include <chrono>
#include <thread>

using namespace magic;

void PrintAllocatorMetrics(const AllocatorMetrics& metrics) {
  fmt::println("Allocator metrics");
  fmt::println("Total allocate: {} ({} KB)", metrics.total_allocate,
               metrics.total_allocate_bytes / (1024));
  fmt::println("AllocateNew: {} times", metrics.allocate_new_count);
  fmt::println("Reused: {} times", metrics.total_allocate - metrics.allocate_new_count);
  fmt::println("Release: {} times", metrics.release_count);
}

//////////////////////////////////////////////////////////////////////

TEST(Processor, JustWorks) {
  bool done = false;

  auto consumer = Processor<std::string>([&]{
    while (auto&& data = Receive<std::string>()) {
      fmt::println("Received: {}", *data);
    }
    done = true;
  });

  consumer.Send("Hello");
  consumer.Send("World");
  consumer.Close();

  ASSERT_TRUE(done);
}

//////////////////////////////////////////////////////////////////////

TEST(Processor, PipeLine) {
  std::string output;

  Processor<std::string> p3([&]() {
    while (auto message = Receive<std::string>()) {
      output += *message;
    }
  });

  Processor<std::string> p2([&p3]() {
    while (auto message = Receive<std::string>()) {
      p3.Send(std::move(*message));
    }
    p3.Close();
  });

  Processor<std::string> p1([&p2]() {
    while (auto message = Receive<std::string>()) {
      p2.Send(std::move(*message));
    }
    p2.Close();
  });

  p1.Send("Hello");
  p1.Send(", ");
  p1.Send("World");
  p1.Send("!");
  p1.Close();

  ASSERT_EQ(output, "Hello, World!");
}

//////////////////////////////////////////////////////////////////////

TEST(Processor, Tokenize) {
  std::vector<std::string> tokens;

  Processor<std::string> consumer([&]() {
    while (auto token = Receive<std::string>()) {
      tokens.push_back(*token);
    }
  });

  Processor<char> tokenizer([&consumer]() {
    std::string token = "";
    while (auto chr = Receive<char>()) {
      if (*chr == ' ') {
        if (!token.empty()) {
          consumer.Send(token);
          token.clear();
        }
      } else {
        token += *chr;
      }
    }
    if (!token.empty()) {
      consumer.Send(token);
    }
    consumer.Close();
  });

  Processor<std::string> streamer([&tokenizer]() {
    while (auto chunk = Receive<std::string>()) {
      for (auto& chr : *chunk) {
        tokenizer.Send(chr);
      }
    }
    tokenizer.Close();
  });

  streamer.Send("Hel");
  streamer.Send("lo W");
  streamer.Send("orld");
  streamer.Close();

  ASSERT_EQ(tokens.size(), 2);
  ASSERT_EQ(tokens[0], "Hello");
  ASSERT_EQ(tokens[1], "World");
}

//////////////////////////////////////////////////////////////////////

TEST(Processor, Stress) {
  static const size_t kIterationCount = 100500;

  auto done = false;
  auto total = 0ul;

  auto consumer1 = Processor<size_t>([]{
    auto sum = 0ul;
    while (auto&& data = Receive<size_t>()) {
      sum += *data;
    }
    ASSERT_EQ(sum, kIterationCount);
  });

  auto consumer2 = Processor<size_t>([&]{
    while (auto&& data = Receive<size_t>()) {
      total += *data;
      consumer1.Send(*data);
    }

    done = true;
  });

  for (size_t index = 0; index < kIterationCount; ++index) {
    consumer2.Send(1);
  }

  consumer2.Close();

  ASSERT_TRUE(done);
  ASSERT_EQ(total, kIterationCount);

  PrintAllocatorMetrics(GetAllocatorMetrics());
}

//////////////////////////////////////////////////////////////////////
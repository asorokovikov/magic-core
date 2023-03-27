#include <fmt/core.h>
#include <magic/fibers/api.h>

using namespace magic;
using namespace std::chrono_literals;

int main() {
  fmt::println("Fibers example");

  RunScheduler(1, []{
    Go([]{
      fmt::println("Sleep for 100ms");
//      self::SleepFor(100ms);
      fmt::println("Another fiber");
    });
    self::Yield();
    fmt::println("Hello from fiber");
  });

  return 0;
}

#include <magic/common/stopwatch.h>

#include <magic/executors/manual.h>
#include <magic/coroutine/stackless/task.h>
#include <magic/coroutine/stackless/fire.h>

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/std.h>

#include <asio.hpp>

// #include "std_future.h"

using namespace magic;
using namespace std::chrono_literals;

using Scheduler = asio::io_context;

//////////////////////////////////////////////////////////////////////

void Log(std::string_view s) {
  static Stopwatch sw;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;
  using std::chrono::seconds;
  fmt::println("(~{}) {}", duration_cast<seconds>(sw.Elapsed()), s);
}

//////////////////////////////////////////////////////////////////////

struct TimerAwaiter {
  asio::steady_timer& timer;

  bool await_ready() {
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) {
    timer.async_wait([handle](std::error_code) mutable {
      handle.resume();
    });
  }

  void await_resume() {
  }
};

auto operator co_await(asio::steady_timer& timer) {
  return TimerAwaiter{timer};
}

Task<> TestTimer(Scheduler& scheduler) {
  fmt::println("Running on thread {}", std::this_thread::get_id());

  asio::steady_timer timer{scheduler};
  timer.expires_after(2s);
  Log("run timer");

  co_await timer;

  fmt::println("Running on thread {}", std::this_thread::get_id());

  Log("after timer");
}

int main() {
    Scheduler scheduler;

    FireAndForget(TestTimer(scheduler));

    scheduler.run();

  return 0;
}
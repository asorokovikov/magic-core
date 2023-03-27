#include <fmt/core.h>

#include <magic/common/stopwatch.h>

#include <magic/executors/thread_pool.h>
#include <magic/fibers/sync/mutex.h>
#include <magic/fibers/sync/wait_group.h>

#include <vector>
#include <numeric>

using namespace magic;
using namespace magic::fibers;

//////////////////////////////////////////////////////////////////////

void WorkloadMutex() {
  for (size_t index = 0; index < 13; ++index) {
    Go([&] {
      WaitGroup wg;

      size_t cs1 = 0;
      Mutex mutex1;

      size_t cs2 = 0;
      Mutex mutex2;

      for (size_t i = 0; i < 100; ++i) {
        wg.Add();

        Go([&] {
          for (size_t j = 0; j < 65536; ++j) {
            {
              std::lock_guard lock(mutex1);
              ++cs1;
            }
            if (j % 17 == 0) {
              self::Yield();
            }
            {
              std::lock_guard lock(mutex2);
              ++cs2;
            }
          }
          wg.Done();
        });
      }

      wg.Wait();
    });
  }
}

////////////////////////////////////////////////////////////////////////

auto RunBenchmark() {
  Stopwatch stopwatch;
  ThreadPool scheduler(4);

  Go(scheduler, [] {
    WorkloadMutex();
  });

  scheduler.WaitIdle();
  scheduler.Stop();

  const auto elapsed = stopwatch.Elapsed();

  //  const auto elapsedMs = stopwatch.ElapsedMs();
  fmt::println("~({} ms) elapsed", elapsed.Millisecond());

  return elapsed;
}

//////////////////////////////////////////////////////////////////////

int main() {
  static const size_t kIteration = 5;

  auto metrics = std::vector<size_t>(kIteration);
  for (size_t index = 0; index < kIteration; ++index) {
    auto elapsed = RunBenchmark();
    metrics.push_back(elapsed.Millisecond());
  }

  auto average = std::reduce(metrics.begin(), metrics.end()) / kIteration;
  fmt::println("\nAverage: {} ms", average);

  return 0;
}
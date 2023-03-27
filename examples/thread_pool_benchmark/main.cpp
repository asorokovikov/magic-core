#include <fmt/core.h>

#include <magic/common/stopwatch.h>
#include <magic/executors/thread_pool.h>

#include <atomic>
#include <vector>
#include <thread>
#include <numeric>

using namespace magic;

//////////////////////////////////////////////////////////////////////

class AtomicCounter final {
 public:
  void Increment() {
    value_.fetch_add(1);
  }

  size_t Value() const {
    return value_.load();
  }

 private:
  std::atomic<size_t> value_ = 0;
};

//////////////////////////////////////////////////////////////////////

void Stress() {
  AtomicCounter counter;
  Stopwatch stopwatch;

  std::vector<std::thread> threads;
  for (size_t index = 0; index < 10; ++index) {
    threads.emplace_back([&counter] {
      for (size_t index = 0; index < 1'000'000; ++index) {
        counter.Increment();
      }
    });
  }

  for (auto&& t : threads) {
    t.join();
  }

  auto elapsed_ms = stopwatch.ElapsedMs();

  fmt::println("Iteration: {}", counter.Value());
  fmt::println("Elapsed: {} ms", elapsed_ms);
}

//////////////////////////////////////////////////////////////////////

inline void Pause() {
  asm volatile("pause\n" : : : "memory");
}

//////////////////////////////////////////////////////////////////////

class SpinLock final {
 public:
  void Lock() {
    while (locked_.exchange(1, std::memory_order::acquire) == 1) {
      while (locked_.load(std::memory_order::relaxed) == 1) {
        Pause();
      }
    }
  }

  void Unlock() {
    locked_.store(0, std::memory_order::release);
  }

 private:
  std::atomic<uint32_t> locked_ = 0;
};

size_t TestSpinLock() {
  Stopwatch stopwatch;
  SpinLock spinlock;

  size_t counter = 0;

  std::vector<std::thread> threads;
  for (size_t index = 0; index < 10; ++index) {
    threads.emplace_back([&] {
      // Contender thread
      for (size_t index = 0; index < 100'500; ++index) {
        spinlock.Lock();
        ++counter;  // Critical section
        spinlock.Unlock();
      }
    });
  }

  for (auto&& t : threads) {
    t.join();
  }

  auto elapsed_ms = stopwatch.ElapsedMs();

  fmt::println("Iteration: {}", counter);
  fmt::println("Elapsed: {} ms", elapsed_ms);

  return elapsed_ms;
}

//////////////////////////////////////////////////////////////////////
//
//void WorkloadMutex() {
//  for (size_t index = 0; index < 13; ++index) {
//    Go([&]{
//      WaitGroup wg;
//
//      size_t critical_section_1 = 0;
//      Mutex mutex1;
//
//      Mutex mutex2;
//
//
//    });
//  }
//}
//
////////////////////////////////////////////////////////////////////////
//
//void RunBenchmark() {
//  Stopwatch stopwatch;
//  auto scheduler = ThreadPool(4);
//
//  Go(scheduler, []{
//    WorkloadMutex();
//  });
//
//  scheduler.WaitAll();
//  scheduler.Stop();
//
//  const auto elapsedMs = stopwatch.ElapsedMs();
//  fmt::println("~({} ms) elapsed", elapsedMs);
//}

//////////////////////////////////////////////////////////////////////

int main() {
  fmt::println("Stress test");

  static const size_t kIteration = 20;

  auto metrics = std::vector<size_t>(kIteration);
  for (size_t index = 0; index < kIteration; ++index) {
    //    Stress();
    auto elapsed_ms = TestSpinLock();
    metrics.push_back(elapsed_ms);
  }

  auto average = std::reduce(metrics.begin(), metrics.end()) / kIteration;
  fmt::println("\nAverage: {} ms", average);

  return 0;
}
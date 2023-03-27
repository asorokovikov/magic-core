#pragma once

#include <chrono>

using namespace std::chrono_literals;

namespace magic {

using Clock = std::chrono::steady_clock;
using Timestamp = std::chrono::time_point<Clock>;

struct Duration : public std::chrono::duration<double> {
  size_t Millisecond() const {
    return duration_cast<std::chrono::milliseconds>(*this).count();
  }
};

//////////////////////////////////////////////////////////////////////

// Usage:
//
// Stopwatch sw;
// ...
// spdlog::debug("Elapsed: {} seconds", sw);    =>  "Elapsed 0.005116733 seconds"
// spdlog::info("Elapsed: {:.6} seconds", sw);  =>  "Elapsed 0.005163 seconds"
//
//
// If other units are needed (e.g. millis instead of double), include "fmt/chrono.h" and use
// "duration_cast<..>(sw.elapsed())":
//
// #include <spdlog/fmt/chrono.h>
//..
// using std::chrono::duration_cast;
// using std::chrono::milliseconds;
// spdlog::info("Elapsed {}", duration_cast<milliseconds>(sw.elapsed())); => "Elapsed 5ms"

class Stopwatch final {
 public:
  Stopwatch() : timestamp_{Clock::now()} {
  }

  Duration Elapsed() const {
    return {Clock::now() - timestamp_};
  }

  size_t ElapsedMs() const {
    return duration_cast<std::chrono::milliseconds>(Elapsed()).count();
  }

  void Reset() {
    timestamp_ = Clock::now();
  }

 private:
  Timestamp timestamp_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
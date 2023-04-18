#pragma once

#include <chrono>

namespace magic {

using Clock = std::chrono::steady_clock;
using Timestamp = std::chrono::time_point<Clock>;
using Duration = std::chrono::duration<double>;

//////////////////////////////////////////////////////////////////////

struct TimeSpan {
  Duration Value;

  // Gets the milliseconds component of the time interval
  size_t Milleseconds() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Value).count();
  }
};

//////////////////////////////////////////////////////////////////////

inline TimeSpan ToTimeSpan(Duration duration) {
  return TimeSpan{duration};
}

inline TimeSpan ToTimeSpan(double duration) {
  return TimeSpan{Duration{duration}};
}

}  // namespace magic
#pragma once

#include <utility>

namespace magic {

// Calls a function when this object goes out of scope
// Inspired by a 'defer' statement from Go programming language:
// https://tour.golang.org/flowcontrol/12

// Usage: Defer defer( [this]() { CleanUp(); } );

template <typename F>
class Defer final {
 public:
  Defer(F&& func) : func_(std::forward<F>(func)) {
  }

  ~Defer() {
    func_();
  }

 private:
  F func_;
};

}  // namespace magic
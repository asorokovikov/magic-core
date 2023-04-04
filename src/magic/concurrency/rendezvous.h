#pragma once

#include <wheels/core/panic.hpp>
#include <atomic>

namespace magic {
// Wait-free

class Rendezvous {
 private:
  struct States {
    enum _ {
      Initial = 0,
      Consumer = 1,
      Producer = 2,
      Rendezvous = Producer | Consumer
    };
  };

  using State = int64_t;

 public:
  bool Produce() {
    switch (state_.fetch_or(States::Producer)) {
      case States::Initial:
        // Initial -> Producer
        return false;
      case States::Consumer:
        // Consumer -> Rendezvous
        return true;
      default:
        WHEELS_PANIC("Unexpected state");
    }
  }

  bool Consume() {
    switch (state_.fetch_or(States::Consumer)) {
      case States::Initial:
        // Initial -> Consumer
        return false;
      case States::Producer:
        // Producer -> Rendezvous
        return true;
      default:
        WHEELS_PANIC("Unexpected state");
    }
  }

  bool Produced() const {
    return state_.load() == States::Producer;
  }

 private:
  std::atomic<State> state_{States::Initial};
};

}  // namespace name
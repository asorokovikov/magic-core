#include <magic/concurrency/future_lite.h>

namespace magic {
namespace detail {

void ParkingLot::Park() {
  fiber_ = &Fiber::GetCurrentFiber();
  fiber_->Suspend();
}

void ParkingLot::Wake() {
  auto fiber = std::exchange(fiber_, nullptr);
  if (fiber != nullptr) {
    fiber->Resume();
  }
}

}  // namespace detail
}  // namespace magic
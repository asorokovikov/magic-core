#include <magic/fibers/sync/condvar.h>

namespace magic::fibers {

void CondVar::Wait(CondVar::Lock& lock) {
  auto epoch = futex_.PrepareWait();
  lock.unlock();
  futex_.ParkIfEqual(epoch);
  lock.lock();
}

void CondVar::NotifyOne() {
  futex_.WakeOne();
}

void CondVar::NotifyAll() {
  futex_.WakeAll();
}

}  // namespace magic::fibers
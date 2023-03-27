#pragma once

#include <atomic>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

//inline uint64_t GetCpuCycleCount() {
//  uint32_t hi, lo;
//  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
//  return (static_cast<uint64_t>(hi) << 32) | lo;
//}

// Relax in favour of the CPU owning the lock

// https://c9x.me/x86/html/file_module_x86_id_232.html
// https://aloiskraus.wordpress.com/2018/06/16/why-skylakex-cpus-are-sometimes-50-slower-how-intel-has-broken-existing-code/

inline void SpinLockPause() {
#if defined(__wheels_arch_x86_64)
  asm volatile("pause\n" : : : "memory");
#elif defined(__wheels_arch_armv8_a_64)
  asm volatile("yield\n" : : : "memory");
#else
  ;
#endif
}

} // namespace detail

//////////////////////////////////////////////////////////////////////

class SpinLock final {
 public:

  void Lock() {
    while (locked_.exchange(1, std::memory_order::acquire) == 1) {
      while (locked_.load(std::memory_order::relaxed) == 1){
        detail::SpinLockPause();
      }
    }
  }

  void Unlock() {
    locked_.store(0, std::memory_order::release);
  }

  // BasicLocable

  void lock() {
    Lock();
  }

  void unlock() {
    Unlock();
  }

 private:
  std::atomic<uint32_t> locked_ = 0;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
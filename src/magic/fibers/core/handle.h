#pragma once

namespace magic {

//////////////////////////////////////////////////////////////////////

class Fiber;

class FiberHandle {
  friend class Fiber;

 public:
  static FiberHandle Invalid();

 public:
  FiberHandle() : FiberHandle(nullptr) {
  }

  bool IsValid() const {
    return fiber_ != nullptr;
  }

  // Schedule to an associated scheduler
  void Schedule();

  // Resume the fiber immediately in the current thread
  void Resume();

 private:
  FiberHandle(Fiber* fiber) : fiber_(fiber) {
  }

  Fiber* Release();

 private:
  Fiber* fiber_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
#pragma once

#include <magic/concurrency/lockfree/stamped_ptr.h>

#include <fmt/core.h>
#include <thread>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

struct SplitCount {
  int32_t transient;
  int32_t strong;  // > 0

  SplitCount operator+(const SplitCount& that) const {
    return {transient + that.transient, strong + that.strong};
  }

  SplitCount operator*(int32_t value) const {
    return {transient * value, strong * value};
  }

  bool IsZero() const {
    return transient == 0 && strong == 0;
  }
};

static_assert(sizeof(SplitCount) == sizeof(size_t), "Not supported");

template <typename T>
struct ControlBlock {
  T* obj_ptr = nullptr;
  std::atomic<SplitCount> global;

  ~ControlBlock() {
    delete obj_ptr;
  }

  void IncrementRefCount(SplitCount delta = {1, 1}) {
    auto current = global.load();
    auto total = current + delta;
    while (!global.compare_exchange_weak(current, total)) {
      total = current + delta;
    }

    if (total.IsZero()) {
      delete this;
    }
  }

  void DecrementRefCount(SplitCount delta = {1, 1}) {
    IncrementRefCount(delta * -1);
  }
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

template <typename T>
class AtomicSharedPtr;

template <typename T>
class SharedPtr {
  using ControlBlock = detail::ControlBlock<T>;

 public:
  SharedPtr(T* ptr = nullptr) {
    if (ptr) {
      cb_ptr_ = new ControlBlock();
      cb_ptr_->obj_ptr = ptr;
      cb_ptr_->IncrementRefCount();
    }
  }

  SharedPtr(ControlBlock* cb) : cb_ptr_(cb) {
  }

  // Copy

  SharedPtr(const SharedPtr<T>& that) noexcept {
    cb_ptr_ = that.cb_ptr_;
    if (cb_ptr_) {
      cb_ptr_->IncrementRefCount();
    }
  }

  SharedPtr<T>& operator=(const SharedPtr<T>& that) noexcept {
    auto temp = that;
    Swap(temp);
    return *this;
  }

  // Move

  SharedPtr(SharedPtr<T>&& that) noexcept {
    Swap(that);
  }

  SharedPtr<T>& operator=(SharedPtr<T>&& that) noexcept {
    Swap(that);
    return *this;
  }

  T* operator->() const {
    return cb_ptr_->obj_ptr;
  }

  T& operator*() const {
    return *cb_ptr_->obj_ptr;
  }

  explicit operator bool() const {
    return cb_ptr_ != nullptr;
  }

  void Reset() {
    if (cb_ptr_) {
      cb_ptr_->DecrementRefCount();
    }
    cb_ptr_ = nullptr;
  }

  ~SharedPtr() {
    Reset();
  }

 private:
  void Swap(SharedPtr& that) noexcept {
    std::swap(cb_ptr_, that.cb_ptr_);
  }

 private:
  ControlBlock* cb_ptr_ = nullptr;

  friend class AtomicSharedPtr<T>;
};

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
  return SharedPtr<T>(new T(std::forward<Args>(args)...));
}

//////////////////////////////////////////////////////////////////////

template <typename T>
class AtomicSharedPtr final {
  using ControlBlock = detail::ControlBlock<T>;

 public:
  // nullptr
  AtomicSharedPtr() : ptr_(nullptr) {
  }

  ~AtomicSharedPtr() {
  }

  SharedPtr<T> Load() {
    auto ptr = AcquireRef();
    return {ptr.raw_ptr};
  }

  void Store(SharedPtr<T> value) {
    auto cb = ptr_.Exchange({value.cb_ptr_});
    value.cb_ptr_->IncrementRefCount();

    if (!cb) {
      return;
    }
    int32_t ref_count = cb.stamp;
    cb->DecrementRefCount({ref_count, ref_count});
  }

  explicit operator SharedPtr<T>() {
    return Load();
  }

  bool CompareExchangeWeak(SharedPtr<T>& /*expected*/, SharedPtr<T> /*desired*/) {
    auto current = AcquireRef();

    return false;  // Not implemented
  }

 private:
  StampedPtr<ControlBlock> AcquireRef() {
    auto current = ptr_.Load();
    auto next = StampedPtr<ControlBlock>();

    if (!current) {
      return {nullptr};
    }

    // increment local ref count
    while (true) {
      next = current.IncrementStamp();
      if (next.IsNull() || ptr_.CompareExchangeWeak(current, next)) {
        break;
      }
    }

    return next;
  }


  void SyncRefCounts() {
    auto current = ptr_.Load();
    auto ref_count = current.stamp;

    if (ref_count == 0) {
      return;
    }

    // increment global ref count
    auto old_value = current->global.fetch_sub(ref_count);

    // decrement local ref count
    while (true) {
      auto next = current.DecrementStamp(ref_count);
      if (ptr_.CompareExchangeWeak(current, next)) {
        break;
      }
    }
  }

 private:
  AtomicStampedPtr<ControlBlock> ptr_;
};

}  // namespace magic
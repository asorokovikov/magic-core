#pragma once

#include <type_traits>
#include <utility>

namespace magic::detail {

//////////////////////////////////////////////////////////////////////

// Low-level storage for value
template <typename T>
class ValueStorage final {
  using Storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

 public:
  template <typename... Arguments>
  void Construct(Arguments&&... arguments) {
    new (&storage_) T(std::forward<Arguments>(arguments)...);
  }

  void MoveConstruct(T&& that) {
    new (&storage_) T(std::move(that));
  }

  void CopyConstruct(const T& that) {
    new (&storage_) T(that);
  }

  T& RefUnsafe() {
    return *PtrUnsafe();
  }

  const T& ConstRefUnsafe() const {
    return *PtrUnsafe();
  }

  T* PtrUnsafe() {
    return reinterpret_cast<T*>(&storage_);
  }

  const T* PtrUnsafe() const {
    return reinterpret_cast<const T*>(&storage_);
  }

  void Destroy() {
    PtrUnsafe()->~T();
  }

 private:
  Storage storage_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic::detail
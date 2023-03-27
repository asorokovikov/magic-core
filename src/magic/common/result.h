#pragma once

#include <cassert>
#include <utility>
#include <type_traits>

#include <magic/common/result/error.h>
#include <magic/common/result/value_storage.h>

/* References
 *
 * http://joeduffyblog.com/2016/02/07/the-error-model/
 *
 * C++:
 * https://www.boost.org/doc/libs/1_72_0/libs/outcome/doc/html/index.html
 * https://github.com/llvm-mirror/llvm/blob/master/include/llvm/Support/ErrorOr.h
 * https://github.com/facebook/folly/blob/master/folly/Try.h
 * https://abseil.io/docs/cpp/guides/status
 * https://github.com/TartanLlama/expected
 * https://www.youtube.com/watch?v=CGwk3i1bGQI
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0762r0.pdf
 *
 * Rust:
 * https://doc.rust-lang.org/book/ch09-02-recoverable-errors-with-result.html
 */

namespace magic {

//////////////////////////////////////////////////////////////////////

// Result = Value | Error

template <typename T>
class [[nodiscard]] Result final {
 public:
  static_assert(!std::is_reference<T>::value, "Reference types are not supported");
  using ValueType = T;

  // Static constructors
  template <typename... Arguments>
  static Result Ok(Arguments&&... arguments) {
    Result result;
    result.value_.Construct(std::forward<Arguments>(arguments)...);
    return result;
  }

  static Result Ok(T&& value) {
    return Result(std::move(value));
  }

  static Result Fail(magic::Error error) {
    return Result(std::move(error));
  }

  // Moving

  Result(Result&& that) {
    MoveImpl(std::move(that));
  }

  Result& operator=(Result&& that) {
    DestroyValueIfExists();
    MoveImpl(std::move(that));
    return *this;
  }

  // Copying

  Result(const Result& that) {
    CopyImpl(that);
  }

  Result& operator=(const Result& that) {
    DestroyValueIfExists();
    CopyImpl(std::move(that));
    return *this;
  }

  // Testing

  bool HasError() const {
    return error_.HasError();
  }

  bool IsOk() const {
    return !HasError();
  }

  bool HasValue() const {
    return !HasError();
  }

  void ThrowIfError() const {
    error_.ThrowIfError();
  }

  void Ignore() {
    // No-op
  }

  // Error accessors

  bool MatchErrorCode(int expected) const {
    return error_.ErrorCode().value() == expected;
  }

  auto Error() const -> const magic::Error& {
    return error_;
  }

  auto ErrorCode() const -> std::error_code {
    return error_.ErrorCode();
  }

  // Value accessors

  // Unsafe methods use after IsOk() only
  // It will be UB (Undefined behavior) if Result doesn't contain a value

  auto ValueUnsafe() & -> T& {
    return value_.RefUnsafe();
  }

  auto ValueUnsafe() const& -> const T& {
    return value_.ConstRefUnsafe();
  }

  auto ValueUnsafe() && -> T&& {
    return std::move(value_.RefUnsafe());
  }

  // Safe value getters
  // Throws if Result doesn't contain a value

  T& ValueOrThrow() & {
    ThrowIfError();
    return value_.RefUnsafe();
  }

  const T& ValueOrThrow() const& {
    ThrowIfError();
    return value_.ConstRefUnsafe();
  }

  T&& ValueOrThrow() && {
    ThrowIfError();
    return std::move(value_.RefUnsafe());
  }

  auto Unwrap() {
    return ValueOrThrow();
  }

  // operator -> overloads
  // Unsafe: behaviour is undefined if Result doesn't contain a value

  T* operator->() {
    return value_.PtrUnsafe();
  }

  const T* operator->() const {
    return value_.PtrUnsafe();
  }

  // operator * overloads
  // Unsafe: behaviour is undefined if Result doesn't contain a value

  T& operator*() & {
    return value_.RefUnsafe();
  }

  const T& operator*() const& {
    return value_.ConstRefUnsafe();
  }

  T&& operator*() && {
    return std::move(value_.RefUnsafe());
  }

  // Unwrap rvalue Result automatically

  operator T&&() && {
    return std::move(ValueOrThrow());
  }

  ~Result() {
    DestroyValueIfExists();
  }

 private:
  Result() {
  }

  Result(T&& value) {
    value_.MoveConstruct(std::move(value));
  }

  Result(const T& value) {
    value_.CopyConstruct(value);
  }

  Result(magic::Error error) : error_(std::move(error)) {
    assert(error.HasError());
  }

  void MoveImpl(Result&& that) {
    error_ = std::move(that.error_);
    if (that.HasValue()) {
      value_.MoveConstruct(std::move(that.ValueUnsafe()));
    }
  }

  void CopyImpl(const Result& that) {
    error_ = that.error_;
    if (that.HasValue()) {
      value_.CopyConstruct(that.ValueUnsafe());
    }
  }

  void DestroyValueIfExists() {
    if (IsOk()) {
      value_.Destroy();
    }
  }

 private:
  detail::ValueStorage<T> value_;
  magic::Error error_;
};

//////////////////////////////////////////////////////////////////////

template <>
class [[nodiscard]] Result<void> {
 public:
  //  using ValueType = void;

  // Static constructors

  static Result Ok() {
    return Result{};
  }

  static Result Fail(magic::Error error) {
    return Result{std::move(error)};
  }

  // Testing

  bool HasError() const {
    return error_.HasError();
  }

  bool IsOk() const {
    return !HasError();
  }

  void ThrowIfError() const {
    error_.ThrowIfError();
  }

  void Ignore() {
    // No-op
  }

  void Unwrap() const {
    ThrowIfError();
  }

  auto Error() const -> magic::Error {
    return error_;
  }

  bool MatchErrorCode(int expected) const {
    return error_.ErrorCode().value() == expected;
  }

  auto ErrorCode() const {
    return error_.ErrorCode();
  }

 private:
  Result() = default;

  Result(magic::Error error) {
    assert(error.HasError());
    error_ = std::move(error);
  }

 private:
  magic::Error error_;
};

using Status = Result<void>;

//////////////////////////////////////////////////////////////////////

}  // namespace base
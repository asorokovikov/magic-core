#pragma once

#include <magic/common/result.h>
#include <wheels/support/concepts.hpp>
#include <concepts>

namespace magic {

// Forward declarations

template <typename T>
class Future;

template <typename T>
class Promise;

template <typename T>
struct Contract;

//////////////////////////////////////////////////////////////////////

// Callback: void(Result<T>)

template <typename F, typename T>
concept CallbackConcept = requires (F callback, Result<T> result) {
  { callback(std::move(result)) } -> std::same_as<void>;
};


// Continuation: T -> U

template <typename F, typename T>
concept Continuation = requires(F f, T value) {
  f(std::move(value));
};

// Asynchronous continuation: T -> Future<U>

template <typename F, typename T>
concept AsyncContinuation = requires(F f, T value) {
  { f(std::move(value)) } -> wheels::InstantiationOf<Future>;
};

// Synchronous continuation: T -> U != Future

template <typename F, typename T>
concept SyncContinuation = Continuation<F, T> && !AsyncContinuation<F, T>;

// Error Handler: Error -> Result<T>

template <typename H, typename T>
concept ErrorHandler = requires(H handler, Error error) {
  { handler(error) } -> std::same_as<Result<T>>;
};

}  // namespace magic
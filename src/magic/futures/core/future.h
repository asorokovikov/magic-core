#pragma once

#include <magic/futures/core/promise.h>
#include <magic/futures/core/callback.h>
#include <magic/futures/core/concepts.h>
#include <magic/futures/core/detail/shared_state.h>
#include <magic/futures/core/detail/traits.h>

#include <magic/executors/inline.h>

#include <magic/common/result/make.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

template <typename T>
class [[nodiscard]] Future : public detail::HoldState<T> {
  template <typename U>
  friend auto MakeContractVia(IExecutor&);

  using detail::HoldState<T>::HasState;
  using detail::HoldState<T>::AccessState;
  using detail::HoldState<T>::ReleaseState;

 public:
  static Future<T> Invalid() {
    return Future<T>{nullptr};
  }

  bool IsValid() const {
    return HasState();
  }

  /// Non-blocking. May call from any thread.
  /// True if this future has result in its shared state
  bool IsReady() const {
    return AccessState().HasResult();
  }

  /// Non-blocking. May call from any thread.
  Result<T> GetResult() && {
    return ReleaseState()->GetResult();
  }

  // Asynchronous Interface

  /// Set executor for asynchronous callback / continuation
  /// Usage: std::move(f).Via(e).Then(c)
  Future<T> Via(IExecutor& executor) && {
    auto state = ReleaseState();
    state->SetExecutor(&executor);
    return Future<T>{std::move(state)};
  }

  IExecutor& GetExecutor() const {
    return AccessState().GetExecutor();
  }

  /// Consume future result with asynchronous callback
  /// Post-condition: IsValid() == false
  void Subscribe(CallbackBase<T>* callback) &&;

  /// Consume future result with asynchronous callback
  /// Post-condition: IsValid() == false
  template <typename F>
  requires CallbackConcept<F, T> void Subscribe(F callback) &&;

  // Combinators

  // Synchronous Then (also known as Map)
  // Future<T> -> U(T) -> Future<U>
  template <typename F>
  requires SyncContinuation<F, T> auto Then(F continuation) &&;

  // Asynchronous Then (also known as FlatMap)
  // Future<T> -> Future<U>(T) -> Future<U>
  template <typename F>
  requires AsyncContinuation<F, T> auto Then(F continuation) &&;

  // Error handling
  // Future<T> -> Result<T>(Error) -> Future<T>
  template <typename F>
  requires ErrorHandler<F, T> Future<T> Recover(F handler) &&;

 private:
  explicit Future(detail::StateRef<T> state) : detail::HoldState<T>(std::move(state)) {
  }
};

//////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T, typename F>
class UniqueCallback : public CallbackBase<T> {
 public:
  explicit UniqueCallback(F func) : func_(std::move(func)) {
  }

  void Invoke(Result<T> result) noexcept override {
    make_result::Invoke(func_, std::move(result));
    delete this;
  }

  void Discard() noexcept override {
    delete this;
  }

 private:
  F func_;
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

template <typename T>
void Future<T>::Subscribe(CallbackBase<T>* callback) && {
  ReleaseState()->SetCallback(std::move(callback));
}

template <typename T>
template <typename F>
requires CallbackConcept<F, T> void Future<T>::Subscribe(F callback) && {
  auto base = new detail::UniqueCallback<T, F>(std::forward<F>(callback));
  std::move(*this).Subscribe(base);
}

//////////////////////////////////////////////////////////////////////

template <typename T>
struct Contract {
  Future<T> future;
  Promise<T> promise;
};

template <typename T>
auto MakeContractVia(IExecutor& executor) {
  auto state = detail::MakeSharedState<T>(executor);
  return Contract<T>{Future<T>{state}, Promise<T>{state}};
}

template <typename T>
auto MakeContract() {
  return MakeContractVia<T>(GetInlineExecutor());
}

// Synchronous Then
// Future<T> -> U(T) -> Future<U>
template <typename T>
template <typename F>
requires SyncContinuation<F, T> auto Future<T>::Then(F cont) && {
  using U = std::invoke_result_t<F, T>;

  auto [f, p] = MakeContractVia<U>(GetExecutor());
  auto callback = [p = std::move(p), f = cont](Result<T> result) mutable {
    if (result.IsOk()) {
      std::move(p).Set(make_result::Invoke(f, *result));
    } else {
      std::move(p).SetError(result.Error());
    }
  };
  std::move(*this).Subscribe(std::move(callback));

  return std::move(f);
}

//////////////////////////////////////////////////////////////////////

// Asynchronous Then
// Future<T> -> Future<U>(T) -> Future<U>

template <typename T>
template <typename F>
requires AsyncContinuation<F, T> auto Future<T>::Then(F continuation) && {
  using U = typename detail::Flatten<std::invoke_result_t<F, T>>::ValueType;

  auto [f, p] = MakeContractVia<U>(GetExecutor());
  auto callback = [p = std::move(p), cont = std::move(continuation)](Result<T> t_result) mutable {
    if (t_result.IsOk()) {
      auto u_future = cont(std::move(*t_result));
      std::move(u_future).Subscribe([p = std::move(p)](Result<U> u_result) mutable {
        std::move(p).Set(std::move(u_result));
      });
    } else {
      std::move(p).SetError(t_result.Error());
    }
  };
  std::move(*this).Subscribe(std::move(callback));

  return std::move(f);
}

//////////////////////////////////////////////////////////////////////

// Recover

// Future<T> -> Result<T>(Error) -> Future<T>
template <typename T>
template <typename F>
requires ErrorHandler<F, T> Future<T> Future<T>::Recover(F handler) && {
  auto [f, p] = MakeContractVia<T>(GetExecutor());
  auto callback = [handler = handler, p = std::move(p)](Result<T> result) mutable {
    if (result.IsOk()) {
      std::move(p).SetValue(*result);
    } else {
      std::move(p).Set(handler(result.Error()));
    }
  };
  std::move(*this).Subscribe(std::move(callback));

  return std::move(f);
}

}  // namespace magic
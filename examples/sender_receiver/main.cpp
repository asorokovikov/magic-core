#include <fmt/core.h>
#include <fmt/std.h>

#include <magic/fibers/api.h>
#include <magic/common/stopwatch.h>
#include <magic/common/meta.h>

#include <future>
#include <thread>
#include <coroutine>
#include <optional>

#include "../stackless_coroutine/std_future.h"

struct Unit {};

//////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
struct EmptyReceiverOf {
  void SetValue(T) {
  }
  void SetError(std::exception_ptr) {
  }
  void SetDone(){};
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

template <typename R, typename T>
concept ReceiverOf = requires(R r, T value) {
  r.SetValue(value);
  r.SetError(std::exception_ptr{});  // Error handling
  r.SetDone();                       // Cancellation
};

template <typename S>
concept Sender = requires(S s) {
  typename S::ValueType;
  s.Connect(detail::EmptyReceiverOf<typename S::ValueType>{});
};

//////////////////////////////////////////////////////////////////////

namespace detail {

template <typename F>
struct FunctorReceiver {
  using T = typename magic::FunctorTraits<F>::ArgumentType;

  FunctorReceiver(F f) : functor(f) {
  }

  void SetValue(T value) {
    functor(value);
  }

  void SetError(std::exception_ptr) {
    std::abort();
  }

  void SetDone() {
    std::abort();
  }

  F functor;
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

template <typename F>
auto AsReceiver(F f) {
  return detail::FunctorReceiver<F>(f);
}

//////////////////////////////////////////////////////////////////////

// Just

namespace detail {

template <typename T>
struct JustSender {
  T value;

  using ValueType = T;

  template <typename R>
  void Connect(R receiver) {
    receiver.SetValue(value);
  }
};

}  // namespace detail

template <typename T>
auto Just(T value) {
  return detail::JustSender<T>{std::move(value)};
}

//////////////////////////////////////////////////////////////////////

// NewThread

// namespace detail {
//
// struct NewThreadSender {
//   using ValueType = Unit;
//
//   template <ReceiverOf<ValueType> R>
//   void Submit(R receiver) {
//     std::thread([&] {
//       receiver.SetValue({});
//     }).detach();
//   }
// };
//
// }  // namespace detail
//
// auto NewThread() {
//   return detail::NewThreadSender{};
// }

//////////////////////////////////////////////////////////////////////

// ThreadPool sender

class ThreadPool {
 public:
  using Task = std::function<void()>;

  struct ITask {
    virtual ~ITask() = default;

    // ~ Interface
    virtual void Run() = 0;
  };

  struct TaskBase : ITask {
    TaskBase* next = nullptr;
  };

  template <typename R>
  struct ManualOperationState : TaskBase {
    ThreadPool& tp;
    R receiver;

    ManualOperationState(ThreadPool& pool, R r) : tp(pool), receiver(r) {
    }

    void Run() override {
      receiver.SetValue({});
    }

    void Start() {
      tp.Submit(this);
    }
  };

  struct Sender {
    ThreadPool& pool;

    using ValueType = Unit;

    //    template <typename R>
    //    void Submit(R receiver) {
    //      pool.Submit([receiver]() mutable {
    //        receiver.SetValue({});
    //      });
    //    }

    template <typename R>
    auto Connect(R receiver) {
      return ManualOperationState{pool, receiver};
    }
  };

  struct Scheduler {
    ThreadPool& pool;

    auto Schedule() {
      return Sender{pool};
    }
  };

  Scheduler GetScheduler() {
    return Scheduler{*this};
  }

  void RunAll() {
    while (head_ != nullptr) {
      auto current = head_;
      head_ = head_->next;
      current->Run();
    }
  }

 private:
  void Submit(TaskBase* task) {
    task->next = head_;
    head_ = task;
  }

 private:
  TaskBase* head_ = nullptr;
};

//////////////////////////////////////////////////////////////////////

// Then combinator

namespace detail {

template <Sender S, typename F>
struct ThenSender {
  using T = typename S::ValueType;
  using U = typename magic::FunctorTraits<F>::ReturnType;
  using ValueType = U;

  S sender;
  F next;

  template <ReceiverOf<ValueType> R>
  void Connect(R u_receiver) {
    auto t_receiver = AsReceiver([next = next, u_receiver](T t_value) mutable {
      U u_value = next(t_value);
      u_receiver.SetValue(u_value);
    });
    sender.Submit(t_receiver);
  }
};

}  // namespace detail

template <typename S, typename F>
auto Then(S sender, F continuation) {
  return detail::ThenSender<S, F>{sender, continuation};
}

template <typename S, typename F>
auto operator|(S sender, F func) {
  return Then(sender, func);
}

//////////////////////////////////////////////////////////////////////

// Coroutine integration

// Receiver - Coroutine
// Sender - Awaiter
// Submit - co_await

namespace detail {
//
// template <Sender S>
// struct SenderAwaiter {
//  using T = typename S::ValueType;
//
//  S sender;
//  std::optional<T> result{};
//
//  bool await_ready() {  // NOLINT
//    return false;
//  }
//
//  void await_suspend(std::coroutine_handle<> handle) {  // NOLINT
//    sender.Submit(AsReceiver([this, handle](T value) mutable {
//      result.emplace(value);
//      handle.resume();
//    }));
//  }
//
//  T await_resume() {  // NOLINT
//    return result.value();
//  }
//};
//

struct ThreadPoolAwaiter {
  ThreadPool::Sender sender;

  struct Receiver {
    std::coroutine_handle<> handle;

    void SetValue(Unit) {
      handle.resume();
    }

    void SetError(std::exception_ptr) {
      std::abort();
    }

    void SetDone() {
      std::abort();
    }
  };

  using OperationState = ThreadPool::ManualOperationState<Receiver>;

  std::optional<OperationState> op_state_ = std::nullopt;

  bool await_ready() {  // NOLINT
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) {  // NOLINT
    op_state_.emplace(sender.Connect(Receiver{handle}));
    op_state_->Start();
  }

  void await_resume() {  // NOLINT
  }
};
}  // namespace detail

// template <Sender S>
// auto operator co_await(S sender) {
//   return detail::SenderAwaiter<S>{sender};
// }

auto operator co_await(ThreadPool::Sender sender) {
  return detail::ThreadPoolAwaiter{sender};
}

template <typename Scheduler>
void Coro(Scheduler& scheduler) {
  co_await scheduler.Schedule();
//  auto value = co_await Just(17);
  fmt::println("Hello! value=");
}

//////////////////////////////////////////////////////////////////////

// Generic operations

// template <Sender S, typename R>
// void Submit(S& sender, R receiver) {
//   sender.Submit(receiver);
// }

template <typename S, typename R>
auto Connect(S& sender, R receiver) {
  return sender.Connect(receiver);
}

template <typename Op>
auto Start(Op& operation) {
  operation.Start();
}

//////////////////////////////////////////////////////////////////////

struct IInvokable {
  virtual ~IInvokable() = default;
  virtual void Invoke() = 0;
};

template <typename F>
class Function {
  template <typename T>
  class FunctionStorage : public IInvokable {
   public:
    FunctionStorage(T f) : func_(f) {
    }

    void Invoke() override {
      func_();
    }

   private:
    T func_;
  };

 public:
  Function(F func) {
    function_ = new FunctionStorage<F>{func};
  }

  ~Function() {
    delete function_;
  }

  auto operator()() {
    function_->Invoke();
  }

 private:
  IInvokable* function_;
};

//////////////////////////////////////////////////////////////////////

int main() {
  Function f([]() {
    fmt::println("Function works...");
  });

  f();

  // Receiver
  {
    auto receiver = AsReceiver([](int value) {
      fmt::println("Received value: {}", value);
    });
    receiver.SetValue(42);
  }

  // Just
  {
      //    auto sender = Just(17);
      //    auto receiver = AsReceiver([](int value) {
      //      fmt::println("value = {}", value);
      //    });
      // Just chain
      // Submit(s1, receiver) -> s1.Submit(receiver) -> receiver.SetValue(42) -> functor(42);
      //    Submit(sender, receiver);

  }

  // NewThread
  //  {
  //    fmt::println("Main ThreadId: {}", std::this_thread::get_id());
  //
  //    auto sender = NewThread();
  //    auto receiver = AsReceiver([](Unit) {
  //      fmt::println("Current ThreadId: {}", std::this_thread::get_id());
  //    });
  //    Submit(sender, receiver);
  //  }

  // Thread pool
  {
      //    ThreadPool pool;
      //    auto scheduler = pool.GetScheduler();
      //    auto sender = scheduler.Schedule();
      //    auto receiver = AsReceiver([](Unit) {
      //      fmt::println("Current ThreadId: {}", std::this_thread::get_id());
      //    });
      //
      //    Submit(sender, receiver);
  }

  // Then
  {
      //    ThreadPool pool;
      //    auto scheduler = pool.GetScheduler();
      //
      //    auto s0 = scheduler.Schedule();
      //    auto s1 = Then(s0, [](Unit) -> int {
      //      return 42;
      //    });
      //
      //    auto s2 = Then(s1, [](int value) {
      //      return value + 1;
      //    });
      //
      //    auto receiver = AsReceiver([](int result) {
      //      fmt::println("\nthread: {}, result: {}", std::this_thread::get_id(), result);
      //    });
      //
      //    // Just chain
      //    // Submit(s1, receiver) -> s1.Submit(receiver) -> receiver.SetValue(42) -> functor(42);
      //
      //    // Then chain
      //    // Submit(s1, receiver) -> s1.Submit(receiver) -> receiver.SetValue(42) -> functor(42);
      //    Submit(s2, receiver);
  }

  {
    auto pool = ThreadPool();
    auto scheduler = pool.GetScheduler();

    auto sender = scheduler.Schedule();
    auto receiver = AsReceiver([](Unit) {
      fmt::println("Hello from Receiver");
    });

    // task - impl of ITask
    auto task = Connect(sender, receiver);
    Start(task);

    pool.RunAll();
  }

  // Coro
  {
    ThreadPool pool;
    auto scheduler = pool.GetScheduler();
    Coro(scheduler);

    pool.RunAll();
  }
  return 0;
}
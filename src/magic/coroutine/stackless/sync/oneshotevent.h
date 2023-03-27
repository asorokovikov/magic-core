#pragma once

#include <wheels/intrusive/forward_list.hpp>

#include <atomic>
#include <coroutine>
#include <cassert>

namespace magic {

class OneShotEvent final {
  using State = uint64_t;
  using CoroutinHandle = std::coroutine_handle<>;

  struct States {
    enum _ {
      NoWaiters = 0,
      Signaled = 1,
    };
  };

  class [[nodiscard]] EventAwaiter : public wheels::IntrusiveForwardListNode<EventAwaiter> {
   public:
    EventAwaiter(OneShotEvent& event) : event_(event) {
    }

    bool await_ready() {
      return event_.IsReady();
    }

    bool await_suspend(CoroutinHandle handle) {
      handle_ = handle;
      return event_.TryEnqueue(this);
    }

    void await_resume() {
    }

    void Resume() const {
      handle_.resume();
    }

   private:
    OneShotEvent& event_;
    CoroutinHandle handle_;
  };

  using Node = wheels::IntrusiveForwardListNode<EventAwaiter>;
  using WaiterList = wheels::IntrusiveForwardList<EventAwaiter>;

 public:

  // ~ Public Interface

  auto WaitAsync() {
    return EventAwaiter(*this);
  }

  // One-shot
  void Fire() {
    FireImpl();
  }

  bool IsReady() const {
    return state_.load() == States::Signaled;
  }

 private:
  // Returns true if awaiter has been added to waiter queue. false - if the event was signaled.
  bool TryEnqueue(Node* node) {
    while (true) {
      auto state = state_.load();
      if (state == States::Signaled) {
        return false;
      } else {
        if (state == States::NoWaiters) {
          node->ResetNext();
        } else {
          auto next = (Node*)state;
          node->SetNext(next);
        }
        if (state_.compare_exchange_strong(state, (State)node)) {
          return true;
        }
      }
    }
  }

  void FireImpl() {
    State state = States::NoWaiters;
    if (state_.compare_exchange_strong(state, States::Signaled)) {
      return;
    }
    if (state == States::Signaled) {
      return;
    }
    auto head = (Node*)state_.exchange(States::Signaled);

    auto waiters = CreateWaiterList(head);
    while (waiters.HasItems()) {
      auto next = waiters.PopFront();
      next->Resume();
    }
  }

  WaiterList CreateWaiterList(Node* head) {
    assert(head);
    WaiterList waiters;
    auto curr = head;
    while (curr) {
      auto next = curr->Next();
      waiters.PushBack(curr);
      curr = next;
    }
    return waiters;
  }

 private:
  std::atomic<State> state_ = States::NoWaiters;
};

}  // namespace magic
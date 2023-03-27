#pragma once

#include <magic/fibers/api.h>
#include <magic/common/intrusive/forward_list.h>

#include <atomic>
#include <cassert>

namespace magic::fibers {

//////////////////////////////////////////////////////////////////////

class OneShotEvent final {
  using State = uint64_t;

  struct States {
    enum _ {
      NoWaiters = 0,
      Signaled = 1,
    };
  };

  class EventAwaiter : public IntrusiveForwardListNode<EventAwaiter>, public IMaybeSuspendAwaiter {
   public:
    EventAwaiter(OneShotEvent& event) : event_(event) {
    }

    bool AwaitSuspend(FiberHandle handle) override {
      handle_ = handle;
      if (event_.TryEnqueue(this)) {
        // added to waiter list -> suspend
        return false;
      }
      // the event has already been signaled
      return true;
    }

    void Schedule() {
      handle_.Schedule();
    }

   private:
    OneShotEvent& event_;
    FiberHandle handle_;
  };

  //////////////////////////////////////////////////////////////////////

 public:
  // ~ Public Interface

  void WaitAsync() {
    if (IsReady()) { // fast path
      return;
    }
    auto awaiter = EventAwaiter(*this);
    self::Suspend(awaiter);
  }

  void Fire() {
    FireImpl();
  }

  bool IsReady() const {
    return state_.load() == States::Signaled;
  }

  //////////////////////////////////////////////////////////////////////

 private:
  using Node = wheels::IntrusiveForwardListNode<EventAwaiter>;
  using WaiterList = wheels::IntrusiveForwardList<EventAwaiter>;
  using AtomicState = std::atomic<State>;

  // Returns:
  // true - awaiter has been added to the waiter list.
  // false - if the event was signaled.
  bool TryEnqueue(Node* node) {
    while (true) {
      auto state = state_.load();
      if (state == States::Signaled) {
        return false;
      } else {
        if (state == States::NoWaiters) {
          node->ResetNext();
        } else {
          node->SetNext((Node*)state);
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
      next->Schedule();
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
  AtomicState state_ = States::NoWaiters;
};

}  // namespace magic::fibers
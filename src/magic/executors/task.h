#pragma once

#include <wheels/intrusive/forward_list.hpp>

namespace magic {

struct ITask {
  virtual ~ITask() = default;

  virtual void Run() noexcept = 0;
  virtual void Discard() noexcept = 0;
};

struct TaskNode : public ITask, public wheels::IntrusiveForwardListNode<TaskNode> {};

}  // namespace magic
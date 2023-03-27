#include <magic/executors/strand.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

Strand::Strand(IExecutor& executor) : executor_(executor) {
}

void Strand::Execute(TaskNode* task) {
  tasks_.Put(task);
  if (counter_.fetch_add(1) == 0) {
    RunNextBatch();
  }
}

void Strand::RunNextBatch() {
  executor_.Execute(this);
}

void Strand::Run() noexcept {
  auto completed = 0ul;
  auto items = tasks_.TakeAll();

  while (items.HasItems()) {
    auto task = items.PopFront();
    task->Run();
    ++completed;
  }

  if (counter_.fetch_sub(completed) > completed) {
    RunNextBatch();
  }
}

void Strand::Discard() noexcept {
  auto items = tasks_.TakeAll();
  while (items.HasItems()) {
    auto task = items.PopFront();
    task->Discard();
  }
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic
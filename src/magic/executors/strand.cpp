#include <magic/executors/strand.h>

namespace magic {

Strand::Strand(IExecutor& executor) : impl_(New<detail::StrandImpl>(executor)) {
}

void Strand::Execute(TaskNode* task) {
  impl_->Execute(task);
}

void detail::StrandImpl::Execute(TaskNode* task) {
  tasks_.Put(task);
  if (counter_.fetch_add(1) == 0) {
    RunNextBatch();
  }
}

void detail::StrandImpl::RunNextBatch() {
  AddRef();
  executor_.Execute(this);
}

void detail::StrandImpl::Run() noexcept {
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

  ReleaseRef();
}

void detail::StrandImpl::Discard() noexcept {
  auto items = tasks_.TakeAll();
  while (items.HasItems()) {
    auto task = items.PopFront();
    task->Discard();
  }
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic
#include <magic/executors/thread_pool.h>
#include <magic/concurrency/local/ptr.h>

#include <wheels/core/assert.hpp>
#include <wheels/core/exception.hpp>
#include <wheels/logging/logging.hpp>

#include <optional>

namespace magic {

//////////////////////////////////////////////////////////////////////

static ThreadLocalPtr<ThreadPool> this_thread_pool;

//////////////////////////////////////////////////////////////////////

ThreadPool* ThreadPool::Current() {
  return this_thread_pool;
}

ThreadPool::ThreadPool(size_t threads) {
  StartWorkerThreads(threads);
}

ThreadPool::~ThreadPool() {
  WHEELS_VERIFY(workers_.empty(), "Most likely, you forgot to call ThreadPool::Stop()");
}

void ThreadPool::Execute(TaskNode* task) {
  counter_.Add();
  if (!tasks_.Put(task)) {
    task->Discard();
    counter_.Done();
  }
}

void ThreadPool::WaitIdle() {
  counter_.WaitZero();
}

void ThreadPool::Stop() {
  tasks_.Close([this](TaskNode* task) {
    task->Discard();
    counter_.Done();
  });

  for (auto& worker : workers_) {
    worker.join();
  }

  workers_.clear();
}

//////////////////////////////////////////////////////////////////////

void ThreadPool::StartWorkerThreads(size_t count) {
  for (size_t index = 0; index < count; ++index) {
    workers_.emplace_back([this]() {
      this_thread_pool.Exchange(this);
      WorkerRoutine();
    });
  }
}

void ThreadPool::WorkerRoutine() {
  while (auto task = tasks_.Take()) {
    task->Run();
    counter_.Done();
  }
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic
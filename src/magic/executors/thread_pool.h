#pragma once

#include <magic/concurrency/intrusive/blocking_queue.h>
#include <magic/concurrency/atomic_counter.h>
#include <magic/executors/executor.h>
#include <magic/executors/task.h>

#include <thread>
#include <vector>

namespace magic {

//////////////////////////////////////////////////////////////////////

// Thread pool for independent CPU-bound tasks
// Fixed pool of worker threads + shared unbounded blocking queue

class ThreadPool final : public IExecutor {
  using Queue = MPMCBlockingQueue<TaskNode>;
  using Workers = std::vector<std::thread>;

 public:
  explicit ThreadPool(size_t threads);
  ~ThreadPool();

  // Non-copyable
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // ~ Public Interface

  // IExecutor
  void Execute(TaskNode* task) override;

  // Waits until outstanding work count has reached zero
  void WaitIdle();

  // Stops the worker threads as soon as possible
  // Pending tasks will be discarded
  void Stop();

  // Locates the current thread pool from worker thread
  static ThreadPool* Current();

  //////////////////////////////////////////////////////////////////////

 private:
  void StartWorkerThreads(size_t workers);
  void WorkerRoutine();

 private:
  AtomicCounter counter_;
  Queue tasks_;
  Workers workers_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
#pragma once

#include <magic/executors/executor.h>
#include <magic/executors/detail/default_task.h>

namespace magic {

/*
 * Usage:
 * Execute(thread_pool, []() {
 *   fmt::println("Running in thread pool");
 * });
 */

template <typename F>
void Execute(IExecutor& executor, F&& task) {
  executor.Execute(CreateTask(std::forward<F>(task)));
}

}  // namespace name
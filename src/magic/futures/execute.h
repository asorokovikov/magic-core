#pragma once

#include <magic/executors/execute.h>
#include <magic/futures/core/future.h>

namespace magic::futures {

// Usage:
// auto f = futures::Execute(pool, []() -> int {
//   return 42;  // <-- Computation runs in provided executor
// });

template <typename F>
auto Execute(IExecutor& executor, F func) {
  using T = std::invoke_result_t<F>;

  auto [f, p] = MakeContractVia<T>(executor);

  magic::Execute(executor, [p = std::move(p), f = std::move(func)]() mutable {
    std::move(p).Set(make_result::Invoke(f));
  });

  return std::move(f);
}

}  // namespace magic
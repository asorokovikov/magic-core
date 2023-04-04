#pragma once

#include <magic/common/result.h>

#include <magic/executors/task.h>

#include <optional>

namespace magic {

template <typename T>
struct ICallback {
  virtual ~ICallback() = default;
  virtual void Invoke(Result<T> result) noexcept = 0;
};

//template <typename T>
//using Callback = fu2::unique_function<void(Result<T>)>;

template <typename T>
class CallbackBase : public ICallback<T>, public TaskNode {
  using ICallback<T>::Invoke;

 public:

  void SetResult(Result<T> result) {
    result_.emplace(std::move(result));
  }

  void Run() noexcept override {
    Invoke(std::move(*result_));
  }

  void Discard() noexcept override {

  }

 private:
  std::optional<Result<T>> result_;
};

}  // namespace magic
#pragma once

#include <magic/futures/core/detail/shared_state.h>

#include <magic/common/result.h>
#include <magic/common/result/error.h>
#include <magic/common/result/make.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

template <typename T>
class Promise final : public detail::HoldState<T> {
  template <typename U>
  friend auto MakeContractVia(IExecutor&);

  using detail::HoldState<T>::ReleaseState;

 public:
  // ~ Public Interface

  void Set(Result<T> result) && {
    ReleaseState()->SetResult(std::move(result));
  }

  void SetError(Error error) && {
    ReleaseState()->SetResult(Fail(error));
  }

  void SetValue(T value) && {
    ReleaseState()->SetResult(Ok(std::move(value)));
  }

 private:
  explicit Promise(detail::StateRef<T> state) : detail::HoldState<T>(std::move(state)) {
  }
};

}  // namespace magic
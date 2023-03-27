#include <magic/common/result/make.h>

#include <cassert>

namespace magic::make_result {

//////////////////////////////////////////////////////////////////////

Status Ok() {
  return Status::Ok();
}

detail::Failure CurrentException() {
  return detail::Failure(std::current_exception());
}

detail::Failure Fail(std::error_code error) {
  assert(error);
  return detail::Failure{error};
}

detail::Failure Fail(Error error) {
  assert(error.HasError());
  return detail::Failure(std::move(error));
}

Status ToStatus(std::error_code error) {
  if (error) {
    return Fail(error);
  } else {
    return Ok();
  }
}

detail::Failure NotSupported() {
  return Fail(std::make_error_code(std::errc::not_supported));
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic::make_result
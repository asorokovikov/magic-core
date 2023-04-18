#include <magic/net/core/response.h>
#include <magic/net/core/format.h>

namespace magic {

std::string HttpResponse::ToString() const {
  return fmt::format("<- GET {}\n-> {} {}\n\n{}\n{}", url_, status_, metrics_, header_, content_);
}

}  // namespace magic
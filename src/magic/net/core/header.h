#pragma once

#include <magic/common/dictionary.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

class HttpHeader final {
 public:
  static HttpHeader Parse(std::string_view s);

  std::string_view operator[](std::string_view value) {
    const auto it = dictionary_.find(value);
    if (it != dictionary_.end()) {
      return it->second;
    }
    return {};
  }

  auto begin() {
    return dictionary_.begin();
  }

  auto end() {
    return dictionary_.end();
  }

 private:
  explicit HttpHeader(Dictionary dictionary) : dictionary_(std::move(dictionary)) {
  }

 private:
  Dictionary dictionary_;
};

}  // namespace magic

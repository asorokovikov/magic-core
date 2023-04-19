#pragma once

#include <cstdlib>
#include <optional>
#include <string_view>

#include <magic/common/convert.h>

namespace magic {

struct Environment {
  template <typename T>
  static std::optional<T> Get(std::string_view name) {
    if (const auto value = std::getenv(name.data())) {
      return ConvertTo<T>(value);
    }

    return std::nullopt;
  }
};

}  // namespace magic
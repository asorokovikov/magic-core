#pragma once

#include <fmt/core.h>
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

  static std::string GetString(std::string_view name) {
    if (const auto value = std::getenv(name.data())) {
      return *ConvertTo<std::string>(value);
    }
    throw std::runtime_error{fmt::format("failed to find environment variable with name: {}", name)};
  }
};

}  // namespace magic
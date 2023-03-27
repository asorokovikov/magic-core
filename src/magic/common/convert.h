#pragma once

#include <optional>
#include <string_view>
#include <sstream>

namespace magic {

//////////////////////////////////////////////////////////////////////

template <typename T>
std::optional<T> ConvertTo(const std::string_view s) {
  std::istringstream ss(s.data());
  T result;
  ss >> result;
  if (ss.fail()) {
    return std::nullopt;
  }

  return result;
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic
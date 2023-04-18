#pragma once

#include <optional>
#include <string_view>
#include <sstream>
#include <charconv>

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

template <>
std::optional<int> ConvertTo<int>(const std::string_view s) {
  int value;
  std::from_chars(s.data(), s.data() + s.size(), value);

  return value;
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic
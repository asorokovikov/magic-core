#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

struct StringHash {
  using hash_type = std::hash<std::string_view>;
  using is_transparent = void;

  std::size_t operator()(const char* str) const {
    return hash_type{}(str);
  }
  std::size_t operator()(std::string_view str) const {
    return hash_type{}(str);
  }
  std::size_t operator()(std::string const& str) const {
    return hash_type{}(str);
  }
};

}  // namespace detail

using Dictionary = std::unordered_map<std::string, std::string, detail::StringHash, std::equal_to<>>;

}  // namespace magic
#pragma once

namespace magic {

struct Unit {
  constexpr bool operator==(const Unit& /*other*/) const {
    return true;
  }
  constexpr bool operator!=(const Unit& /*other*/) const {
    return false;
  }
};

}  // namespace magic
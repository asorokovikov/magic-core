#pragma once

#include <cstdint>
#include <random>
#include <string>

namespace magic {

//////////////////////////////////////////////////////////////////////

class DataGenerator final {
 public:
  DataGenerator(size_t bytes) : bytes_left_(bytes) {
  }

  bool HasNext() const {
    return bytes_left_ > 0;
  }

  size_t NextChunk(char* buffer, size_t limit) {
    auto bytes = std::min(bytes_left_, limit);
    GenerateData(buffer, bytes);
    bytes_left_ -= bytes;
    return bytes;
  }

 private:
  void GenerateData(char* buffer, size_t bytes) {
    for (size_t index = 0; index < bytes; ++index) {
      buffer[index] = 'A' + twister_() % 26;
    }
  }

 private:
  size_t bytes_left_;
  std::mt19937 twister_{42};
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
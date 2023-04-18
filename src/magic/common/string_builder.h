#pragma once

#include <sstream>

namespace magic {

//////////////////////////////////////////////////////////////////////

class StringBuilder final {
 public:
  void Append(std::string_view s) {
    buffer_ << s;
    size_ += s.size();
  }

  void Append(const char* data, size_t size) {
    buffer_.write(data, size);
    size_ += size;
  }

  std::string ToString() const {
    return buffer_.str();
  }

  template <typename T>
  StringBuilder& operator<<(const T& next) {
    buffer_ << next;
    return *this;
  }

  operator std::string() const {
    return buffer_.str();
  }

 private:
  std::ostringstream buffer_;
  size_t size_ = 0;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
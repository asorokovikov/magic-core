#pragma once

#include <charconv>
#include <sstream>
#include <string_view>

namespace magic {

//////////////////////////////////////////////////////////////////////

class StringReader final {
 public:
  StringReader(std::string_view s) : source_(s) {
  }

  // ~ Public Interface

  char NextChar() const {
    return source_[0];
  }

  bool Next(char ch) const {
    return NextChar() == ch;
  }

  bool NextDigit() const {
    return isdigit(NextChar()) != 0;
  }

  bool NextAlpha() const {
    return isalnum(NextChar()) != 0;
  }

  bool NextSpace() const {
    return Next(' ');
  }

  bool IsEmpty() const {
    return source_.empty();
  }

  bool HasNext() const {
    return !source_.empty();
  }

  size_t Length() const {
    return source_.length();
  }

  StringReader& Skip(size_t count) {
    source_.remove_prefix(count);
    return *this;
  }

  StringReader& SkipOne() {
    return Skip(1);
  }

  StringReader& SkipSpaces() {
    while (HasNext() && isspace(NextChar()) != 0) {
      SkipOne();
    }
    return *this;
  }

  StringReader& SkipUntil(char ch) {
    auto offset = source_.find_first_of(ch);
    if (offset == std::string_view::npos) {
      offset = source_.length();
    }
    source_.remove_prefix(offset);
    return *this;
  }

  StringReader& SkipAfter(char ch) {
    auto offset = source_.find_first_of(ch);
    if (offset == std::string_view::npos) {
      offset = source_.length();
    } else {
      offset = std::min(offset + 1, source_.length());
    }
    source_.remove_prefix(offset);
    return *this;
  }

  StringReader& SkipLine() {
    SkipAfter('\n');
    if (Next('\r')) {
      SkipOne();
    }
    return *this;
  }

  char ReadOne() {
    auto ch = NextChar();
    SkipOne();
    return ch;
  }

  std::string_view ReadUntil(char ch) {
    if (IsEmpty() || NextChar() == ch) {
      return {};
    }

    const auto offset = source_.find_first_of(ch);
    if (offset == std::string_view::npos) {
      source_.remove_prefix(source_.length());
      return {};
    }

    auto result = source_.substr(0, offset);
    source_.remove_prefix(offset);

    return result;
  }

  int ReadInt() {
    int value;
    const auto result = std::from_chars(source_.data(), source_.data() + source_.size(), value);
    const auto offset = std::distance(source_.data(), result.ptr);
    source_.remove_prefix(offset);

    return value;
  }

  StringReader& SkipDigits() {
    while (HasNext() && NextDigit()) {
      SkipOne();
    }
    return *this;
  }

  double ReadDouble() {
    auto string = source_;

    SkipDigits();
    if (Next('.') || Next(',')) {
      SkipOne().SkipDigits();
    }

    return std::stod(string.data());
  }

 private:
  std::string_view source_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic
#pragma once

#include <fmt/core.h>

#include <magic/common/string_builder.h>

#include <magic/net/http/status.h>
#include <magic/net/http/response.h>
#include <magic/net/http/header.h>

//////////////////////////////////////////////////////////////////////

template <>
struct fmt::formatter<magic::HttpStatus> {
  static auto parse(format_parse_context& ctx) {  // NOLINT
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(magic::HttpStatus status, FormatContext& ctx) const {  // NOLINT
    return fmt::format_to(ctx.out(), "{}", status.ToString());
  }
};

//////////////////////////////////////////////////////////////////////

template <>
struct fmt::formatter<magic::HttpResponse> {
  static auto parse(format_parse_context& ctx) {  // NOLINT
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(magic::HttpResponse response, FormatContext& ctx) const {  // NOLINT
    return fmt::format_to(ctx.out(), "{}", response.ToString());
  }
};

//////////////////////////////////////////////////////////////////////

template <>
struct fmt::formatter<magic::HttpRequestMetrics> {
  static auto parse(format_parse_context& ctx) {  // NOLINT
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(magic::HttpRequestMetrics m, FormatContext& ctx) const {  // NOLINT
    return fmt::format_to(ctx.out(), "[{} Bytes] (~{}ms)", m.DownloadBytes, m.Duration.Milleseconds());
  }
};

//////////////////////////////////////////////////////////////////////

template <>
struct fmt::formatter<magic::HttpHeader> {
  static auto parse(format_parse_context& ctx) {  // NOLINT
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(magic::HttpHeader header, FormatContext& ctx) const {  // NOLINT
    auto builder = magic::StringBuilder();
    for (auto&& [key, value] : header) {
      builder << key << ": " << value << '\n';
    }
    return fmt::format_to(ctx.out(), "{}", builder.ToString());
  }
};
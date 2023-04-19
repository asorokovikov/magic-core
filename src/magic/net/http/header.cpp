#include <magic/common/string_reader.h>
#include <magic/net/http/header.h>

namespace magic {

HttpHeader HttpHeader::Parse(std::string_view s) {
  auto dictionary = Dictionary();
  auto reader = StringReader(s);

  reader.SkipUntil("HTTP").SkipLine();

  while (reader.SkipSpaces().HasNext()) {
    auto key = reader.ReadUntil(':');
    auto value = reader.SkipOne().SkipSpaces().ReadToEndLine();
    dictionary[std::string(key)] = value;
  }

  return HttpHeader(std::move(dictionary));
}

}  // namespace magic

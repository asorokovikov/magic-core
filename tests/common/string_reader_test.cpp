#include <gtest/gtest.h>
#include "../test_helper.h"

#include <magic/common/string_reader.h>

using namespace std::string_literals;

TEST(StringReader, Empty) {
  auto source = std::string();
  auto reader = StringReader(source);
  ASSERT_TRUE(reader.IsEmpty());
  ASSERT_FALSE(reader.HasNext());
}

TEST(StringReader, Next) {
  auto source = std::string("123");
  auto reader = StringReader(source);

  ASSERT_TRUE(reader.HasNext());
  ASSERT_TRUE(reader.Next('1'));
  ASSERT_TRUE(reader.NextDigit());
  ASSERT_FALSE(reader.NextSpace());

  source = std::string(" ");
  reader = StringReader(source);
  ASSERT_TRUE(reader.NextSpace());

  source = std::string("az");
  reader = StringReader(source);
  ASSERT_TRUE(reader.NextAlpha());
  ASSERT_TRUE(reader.SkipOne().NextAlpha());
}

TEST(StringReader, Length) {
  auto source = std::string("123");
  auto reader = StringReader(source);

  ASSERT_TRUE(reader.HasNext());
  ASSERT_TRUE(reader.Next('1'));
  ASSERT_EQ(reader.Length(), source.size());
}

TEST(StringReader, Copy) {
  auto source = std::string("123");
  auto reader = StringReader(source);
  auto copy = reader;

  ASSERT_TRUE(reader.SkipOne().Next('2'));
  ASSERT_TRUE(copy.Next('1'));
}

TEST(StringReader, Skip) {
  auto source = std::string("12345");
  auto reader = StringReader(source);

  ASSERT_EQ(reader.NextChar(), '1');
  ASSERT_EQ(reader.Length(), source.size());

  ASSERT_EQ(reader.SkipOne().NextChar(), '2');
  ASSERT_EQ(reader.Length(), source.size() - 1);

  ASSERT_EQ(reader.Skip(2).NextChar(), '4');
  ASSERT_EQ(reader.Length(), 2);
}

TEST(StringReader, SkipSpaces) {
  auto source = std::string("     1 2345");
  auto reader = StringReader(source);

  ASSERT_EQ(reader.NextChar(), ' ');
  ASSERT_EQ(reader.Length(), source.size());

  ASSERT_EQ(reader.SkipSpaces().NextChar(), '1');
  ASSERT_EQ(reader.SkipOne().SkipSpaces().NextChar(), '2');
}

TEST(StringReader, SkipDigits) {
  auto source = std::string("123,456.789");
  auto reader = StringReader(source);

  ASSERT_EQ(reader.SkipDigits().NextChar(), ',');
  ASSERT_EQ(reader.SkipOne().SkipDigits().NextChar(), '.');
  ASSERT_TRUE(reader.SkipOne().SkipDigits().IsEmpty());
}

TEST(StringReader, SkipUntil) {
  auto source = std::string("12345");
  auto reader = StringReader(source);

  ASSERT_TRUE(reader.SkipUntil('1').Next('1'));
  ASSERT_TRUE(reader.SkipUntil('3').Next('3'));
  ASSERT_TRUE(reader.SkipUntil('5').Next('5'));
  ASSERT_TRUE(reader.SkipOne().SkipUntil('1').IsEmpty());

  auto empty_source = std::string();
  reader = StringReader(empty_source);
  ASSERT_TRUE(reader.IsEmpty());
  ASSERT_TRUE(reader.SkipUntil('1').IsEmpty());
}

TEST(StringReader, SkipAfter) {
  auto source = std::string("12345");
  auto reader = StringReader(source);

  ASSERT_TRUE(reader.SkipAfter('1').Next('2'));
  ASSERT_TRUE(reader.SkipAfter('3').Next('4'));
  ASSERT_TRUE(reader.SkipAfter('5').IsEmpty());

  auto empty_source = std::string();
  reader = StringReader(empty_source);
  ASSERT_TRUE(reader.IsEmpty());
  ASSERT_TRUE(reader.SkipAfter('1').IsEmpty());
}

TEST(StringReader, SkipLine) {
  auto source = std::string("123\n456\n\r789");
  auto reader = StringReader(source);

  ASSERT_EQ(reader.SkipLine().NextChar(), '4');
  ASSERT_EQ(reader.SkipLine().NextChar(), '7');
  ASSERT_TRUE(reader.SkipLine().IsEmpty());
}

TEST(StringReader, ReadOne) {
  auto source = std::string("123");
  auto reader = StringReader(source);

  ASSERT_EQ(reader.ReadOne(), '1');
  ASSERT_EQ(reader.Length(), source.size() - 1);

  ASSERT_EQ(reader.ReadOne(), '2');
  ASSERT_EQ(reader.Length(), source.size() - 2);

  ASSERT_EQ(reader.ReadOne(), '3');
  ASSERT_EQ(reader.Length(), 0);
}

TEST(StringReader, ReadUntil) {
  const auto empty = std::string{};
  auto reader = StringReader(empty);

  ASSERT_TRUE(reader.ReadUntil('7').empty());

  const auto source = "123"s;
  reader = StringReader(source);

  ASSERT_TRUE(reader.ReadUntil('1').empty());
  ASSERT_EQ(reader.ReadUntil('3'), "12");
}

TEST(StringReader, ReadToEndLine) {
  const auto empty = std::string{};
  auto reader = StringReader(empty);

  ASSERT_TRUE(reader.ReadToEndLine().empty());

  const auto source = "123\n456\n\r789\n\r"s;
  reader = StringReader(source);
  ASSERT_EQ(reader.ReadToEndLine(), "123");
  ASSERT_EQ(reader.ReadToEndLine(), "456");
  ASSERT_EQ(reader.ReadToEndLine(), "789");
  ASSERT_TRUE(reader.ReadToEndLine().empty());
  ASSERT_TRUE(reader.IsEmpty());
}

TEST(StringReader, ReadInt) {
  auto source = std::string("42 55");
  auto reader = StringReader(source);

  ASSERT_EQ(reader.ReadInt(), 42);
  ASSERT_EQ(reader.SkipSpaces().ReadInt(), 55);
}

TEST(StringReader, ReadDouble) {
  const auto source = std::string("42.1 55.42 13");
  auto reader = StringReader(source);

  auto actual = reader.ReadDouble();
  auto expected = 42.1;
  ASSERT_DOUBLE_EQ(actual, expected);

  actual = reader.SkipSpaces().ReadDouble();
  expected = 55.42;
  ASSERT_DOUBLE_EQ(actual, expected);

  actual = reader.SkipSpaces().ReadDouble();
  expected = 13.0;
  ASSERT_DOUBLE_EQ(actual, expected);
}

TEST(StringReader, ParseHttpHeader) {
  const auto source = R"(
    HTTP/1.1 200 OK
    X-Powered-By: Express
    Content-Type: application/json; charset=utf-8
    Content-Length: 388
    ETag: W/"184-btA3qP8tg9Wrq1KC5bpgvj92E8w"
    Date: Mon, 17 Apr 2023 18:32:32 GMT
    Connection: keep-alive
    Keep-Alive: timeout=5
  )";

  auto reader = StringReader(source);

  ASSERT_EQ(reader.SkipUntil("HTTP").ReadUntilSpace(), "HTTP/1.1");
  ASSERT_EQ(reader.SkipSpaces().ReadToEndLine(), "200 OK");
  ASSERT_EQ(reader.SkipSpaces().ReadUntil(':'), "X-Powered-By");
  ASSERT_EQ(reader.SkipOne().SkipSpaces().ReadToEndLine(), "Express");
  ASSERT_EQ(reader.SkipSpaces().ReadUntil(':'), "Content-Type");
  ASSERT_EQ(reader.SkipOne().SkipSpaces().ReadToEndLine(), "application/json; charset=utf-8");
}
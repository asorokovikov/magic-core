#include <gtest/gtest.h>

#include <magic/filesystem/file.h>
#include <magic/common/random.h>
#include <magic/common/stopwatch.h>

#include <fmt/core.h>

using namespace magic;

//////////////////////////////////////////////////////////////////////

TEST(File, MissingFile) {
  auto lines = File::ReadAllLines("1");
  ASSERT_TRUE(lines.HasError());
}

TEST(File, WriteAllText) {
  File::WriteAllText("1.txt", "Hello").ThrowIfError();

  auto lines = File::ReadAllLines("1.txt");
  ASSERT_TRUE(lines.IsOk());
  ASSERT_EQ(lines.ValueOrThrow().size(), 1);
  ASSERT_EQ(lines.ValueOrThrow()[0], "Hello");
}

TEST(File, WriteAllLines) {
  auto actual = std::vector<std::string>{ "1", "2", "3"};

  File::WriteAllLines("WriteAllLines.txt", actual).ThrowIfError();
  auto lines = File::ReadAllLines("WriteAllLines.txt");
  ASSERT_TRUE(lines.IsOk());
  ASSERT_EQ(lines.ValueOrThrow().size(), 3);
  ASSERT_EQ(lines.ValueOrThrow(), actual);
}
#include <gtest/gtest.h>
#include "../test_helper.h"

#include <magic/common/environment.h>

using namespace std::string_literals;

TEST(Common, Convert) {
  auto value = ConvertTo<int>("42");
  ASSERT_EQ(*value, 42);
}

TEST(Common, Environment) {
  ASSERT_ANY_THROW(Environment::GetString("Random"));
  ASSERT_EQ(Environment::Get<int>("TEST"), std::nullopt);
}
#include <gtest/gtest.h>

#include <magic/common/result.h>
#include <magic/common/result/make.h>
#include <magic/common/result/error.h>

#include <chrono>
#include <thread>

using namespace magic;

//////////////////////////////////////////////////////////////////////

static std::error_code TimedOut() {
  return std::make_error_code(std::errc::timed_out);
}

static std::exception_ptr RuntimeError() {
  try {
    throw std::runtime_error("Test");
  } catch (...) {
    return std::current_exception();
  }
}

//////////////////////////////////////////////////////////////////////

class TestClass final {
 public:
  TestClass(std::string message) : message_(std::move(message)) {
    ++object_count_;
  }

  TestClass(const TestClass& that) : message_(that.message_) {
    ++object_count_;
  }

  TestClass& operator=(const TestClass& that) {
    message_ = that.message_;
    ++object_count_;
    return *this;
  }

  TestClass(TestClass&& that) noexcept : message_{std::move(that.message_)} {
    ++object_count_;
  }

  TestClass& operator=(TestClass&& that) noexcept {
    std::swap(message_, that.message_);
    ++object_count_;
    return *this;
  }

  ~TestClass() {
    --object_count_;
  }

  const std::string& Message() const {
    return message_;
  }

  static int ObjectCount() {
    return object_count_;
  }

 private:
  TestClass();

 private:
  std::string message_;
  static int object_count_;
};

int TestClass::object_count_ = 0;

//////////////////////////////////////////////////////////////////////

auto MakeVector(size_t size) {
  std::vector<int> numbers;
  numbers.reserve(size);

  for (size_t index = 0; index < size; ++index) {
    numbers.push_back(index);
  }
  // return Result<std::vector<int>>::Ok(std::move(numbers));
  return Ok(std::move(numbers));
}

auto MakeError() -> Result<std::string> {
  return Fail(TimedOut());
}

auto MakeOkStatus() {
  return Ok();
}

//////////////////////////////////////////////////////////////////////

TEST(Result, Ok) {
  static const std::string kMessage = "Test";
  auto result = Result<TestClass>::Ok(kMessage);
  result.ThrowIfError();
  ASSERT_TRUE(result.IsOk());
  ASSERT_FALSE(result.HasError());

  ASSERT_EQ(result.ValueUnsafe().Message(), kMessage);
  ASSERT_EQ((*result).Message(), kMessage);
  ASSERT_EQ(result->Message(), kMessage);
}

TEST(Result, ObjectCount) {
  {
    auto result = Result<TestClass>::Ok("Test");
    ASSERT_EQ(TestClass::ObjectCount(), 1);

    TestClass object = Result<TestClass>::Ok("Hello");
    ASSERT_EQ(object.Message(), "Hello");
    ASSERT_EQ(TestClass::ObjectCount(), 2);
  }
  ASSERT_EQ(TestClass::ObjectCount(), 0);
  {
    auto result = Result<TestClass>::Fail(TimedOut());
    ASSERT_EQ(TestClass::ObjectCount(), 0);
  }
  ASSERT_EQ(TestClass::ObjectCount(), 0);
}

TEST(Result, CopyCtor) {
  static const std::string kMessage = "Test";
  auto result = Result<TestClass>::Ok(TestClass(kMessage));
  ASSERT_TRUE(result.IsOk());
  ASSERT_EQ(result->Message(), kMessage);
}

TEST(Result, Error) {
  auto result = Result<TestClass>::Fail(TimedOut());
  ASSERT_FALSE(result.IsOk());
  ASSERT_TRUE(result.HasError());

  auto error_code = result.ErrorCode();
  ASSERT_EQ(error_code.value(), (int)std::errc::timed_out);
  ASSERT_THROW(result.ThrowIfError(), std::system_error);
}

TEST(Result, Ignore) {
  MakeVector(5).Ignore();
  MakeError().Ignore();
  MakeOkStatus().Ignore();
}

TEST(Result, MatchErrorCode) {
  Status result = Fail(TimedOut());
  ASSERT_TRUE(result.MatchErrorCode((int)std::errc::timed_out));
}

TEST(Result, Move) {
  static const std::string kMessage = "Test";
  auto a = Ok(TestClass(kMessage));
  auto b = std::move(a);
  ASSERT_EQ(b.ValueOrThrow().Message(), kMessage);
}

TEST(Result, Copy) {
  static const std::string kMessage = "Test";
  auto a = Ok(TestClass(kMessage));
  auto b = a;
  ASSERT_EQ(a.ValueOrThrow().Message(), kMessage);
  ASSERT_EQ(b.ValueOrThrow().Message(), kMessage);
}

TEST(Result, AccessMethods) {
  static const std::string kMessage = "Test";
  auto result = Ok(TestClass(kMessage));
  ASSERT_EQ(result->Message(), kMessage);

  const TestClass& test = *result;
  ASSERT_EQ(test.Message(), kMessage);

  TestClass second = std::move(*result);
  ASSERT_EQ(second.Message(), kMessage);

  ASSERT_EQ(result.ValueOrThrow().Message(), "");
}

TEST(Result, AutomaticallyUnwrapRvalue) {
  std::vector<int> numbers = MakeVector(3);
  ASSERT_EQ(numbers.size(), 3u);

  auto result1 = MakeVector(4);
  numbers = std::move(result1);
  ASSERT_EQ(numbers.size(), 4);

  std::string str;
  ASSERT_THROW(str = MakeError(), std::system_error);
}

TEST(Result, MakeOkResult) {
  auto ok = make_result::Ok();
  ASSERT_TRUE(ok.IsOk());

  const size_t answer = 42;
  Result<size_t> result = make_result::Ok(answer);
}

TEST(Result, MakeErrorResult) {
  Result<std::string> response = make_result::Fail(TimedOut());
  ASSERT_FALSE(response.IsOk());

  Result<std::vector<std::string>> lines = make_result::PropagateError(response);
  ASSERT_FALSE(lines.IsOk());
}

TEST(Result, MakeResultStatus) {
  auto result = make_result::ToStatus(TimedOut());
  ASSERT_FALSE(result.IsOk());
  ASSERT_TRUE(result.HasError());
}

Result<int> IntResult(int value) {
  return make_result::Ok(value);
}

/*
SIMPLE_TEST(Consistency) {
  auto result = IntResult(0);
  if (!result) {
    FAIL_TEST("???");
  }

  if (!IntResult(0)) {
    FAIL();
  }
}
*/

TEST(Result, JustStatus) {
  auto answer = Result<int>::Ok(42);
  auto ok = make_result::JustStatus(answer);
  ASSERT_TRUE(ok.IsOk());

  auto response = Result<std::string>::Fail(TimedOut());
  auto fail = make_result::JustStatus(response);
  ASSERT_FALSE(fail.IsOk());
}

TEST(Result, Exceptions) {
  auto bad = []() -> Result<std::string> {
    try {
      throw std::runtime_error("Bad");
      return make_result::Ok<std::string>("Good");
    } catch (...) {
      return make_result::CurrentException();
    }
  };

  auto result = bad();

  ASSERT_TRUE(result.HasError());

  auto error = result.Error();
  ASSERT_TRUE(error.HasException());
  ASSERT_FALSE(error.HasErrorCode());

  ASSERT_THROW(result.ThrowIfError(), std::runtime_error);
}

// TEST(Result, Invoke) {
//   {
//     auto good = []() -> int {
//       return 42;
//     };
//
//     auto result = make_result::Invoke(good);
//     ASSERT_TRUE(result.HasValue());
//     ASSERT_EQ(*result, 42);
//   }
//
//   {
//     auto bad = []() -> int {
//       throw std::runtime_error("just test");
//     };
//
//     auto result = make_result::Invoke(bad);
//     ASSERT_TRUE(result.HasError());
//   }
// }

// TEST(Result, InvokeWithArguments) {
//   auto sum = [](int x, int y) { return x + y; };
//   auto result = make_result::Invoke(sum, 1, 2);
//   ASSERT_EQ(*result, 3);
// }

// TEST(Result, InvokeVoid) {
//   bool done = false;
//   auto work = [&]() {
//     done = true;
//   };
//   Status status = make_result::Invoke(work);
//   ASSERT_TRUE(status.IsOk());
//   ASSERT_TRUE(done);
// }

TEST(Result, Throw) {
  Result<int> result = make_result::Throw<std::runtime_error>("Test error");

  ASSERT_TRUE(result.HasError());
  ASSERT_THROW(result.ThrowIfError(), std::runtime_error);
}

struct MoveOnly {
  MoveOnly(int d) : data(d) {
  }

  MoveOnly(MoveOnly&& that) = default;
  MoveOnly& operator=(MoveOnly&& that) = default;

  MoveOnly(const MoveOnly& that) = delete;
  MoveOnly& operator=(const MoveOnly& that) = delete;

  int data;
};

TEST(Result, MoveOnly) {
  {
    auto r = Result<MoveOnly>::Ok(42);
    auto v = std::move(r).ValueOrThrow();
    ASSERT_EQ(v.data, 42);
  }

  {
    //    MoveOnly v = Result<MoveOnly>::Ok(17).ExpectValueOr("Fail");
    //    ASSERT_EQ(v.data, 17);
  }
}

// TEST(Result, InvokeMoveOnlyArguments) {
//   auto foo = [](MoveOnly mo) {
//     return MoveOnly(mo.data + 1);
//   };
//
//   auto result = make_result::Invoke(foo, MoveOnly(3));
//   ASSERT_EQ(result->data, 4);
// }

TEST(Result, ConstResultValue) {
  const Result<int> result = make_result::Ok(42);
  ASSERT_EQ(result.ValueOrThrow(), 42);
}

TEST(Result, NotImplemented) {
  Result<int> result = make_result::NotImplemented();

  ASSERT_TRUE(result.HasError());
  ASSERT_TRUE(result.MatchErrorCode((int)std::errc::not_supported));
}

//////////////////////////////////////////////////////////////////////


TEST(Result, Error_Empty) {
  Error error;

  ASSERT_FALSE(error.HasError());
  ASSERT_FALSE(error.HasException());
  ASSERT_FALSE(error.HasErrorCode());
}

TEST(Result, Error_ErrorCode) {
  std::error_code error_code = std::make_error_code(std::errc::timed_out);
  Error error(error_code);

  ASSERT_TRUE(error.HasError());
  ASSERT_TRUE(error.HasErrorCode());
  ASSERT_EQ(error.ErrorCode().value(), (int)std::errc::timed_out);

  ASSERT_FALSE(error.HasException());
  ASSERT_THROW(error.ThrowIfError(), std::system_error);
}

TEST(Result, Error_Exception) {
  Error error(RuntimeError());

  ASSERT_TRUE(error.HasError());
  ASSERT_FALSE(error.HasErrorCode());
  ASSERT_TRUE(error.HasException());
  ASSERT_THROW(error.ThrowIfError(), std::runtime_error);
}

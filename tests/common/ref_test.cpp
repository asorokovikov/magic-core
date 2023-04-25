#include <wheels/test/test_framework.hpp>

#include <fmt/core.h>

#include <magic/common/refer/ref_counted.h>

using namespace magic;

//////////////////////////////////////////////////////////////////////

class Robot : public RefCounted<Robot> {
 public:
  Robot(std::string name) : name_(std::move(name)) {
    fmt::println("I am alive!");
  }

  const std::string& Name() const {
    return name_;
  }

  void Talk() {
    fmt::println("My name is {}", name_);
  }

  ~Robot() {
    fmt::println("Robot {} destroyed", name_);
  }

  Ref<Robot> Me() {
    return RefFromThis();
  }

 private:
  std::string name_;
};

//////////////////////////////////////////////////////////////////////

struct X : virtual IManaged {
  ~X() {
    std::cout << "~X" << std::endl;
  }
};

struct Y : X, RefCounted<Y> {
  ~Y() {
    std::cout << "~Y" << std::endl;
  }
};

//////////////////////////////////////////////////////////////////////

TEST_SUITE(Ref) {
  SIMPLE_TEST(JustWorks) {
    Ref<Robot> robot = New<Robot>("David");

    ASSERT_TRUE(robot.IsValid());

    robot->Talk();

    ASSERT_EQ((*robot).Name(), "David");
  }

  SIMPLE_TEST(FromRawPointer) {
    Robot* raw_ptr = new Robot("Wall-e");
    Ref<Robot> robot{raw_ptr, {}};

    ASSERT_TRUE(robot.IsValid());
    ASSERT_EQ(robot->GetRefCount(), 1);
  }

  SIMPLE_TEST(Invalid) {
    Ref<Robot> robot{};
    ASSERT_FALSE(robot.IsValid());
    ASSERT_EQ(robot.Get(), nullptr);
  }

  SIMPLE_TEST(New) {
    auto ref = New<Robot>("Mike");
    ASSERT_TRUE(ref.IsValid());
    ASSERT_EQ(ref->Name(), "Mike");
  }

  SIMPLE_TEST(CopyCtor) {
    Ref<Robot> r1 = New<Robot>("David");
    Ref<Robot> r2(r1);

    ASSERT_TRUE(r1.IsValid());
    ASSERT_TRUE(r2.IsValid());

    ASSERT_EQ(r1->Name(), "David");
    ASSERT_EQ(r2->Name(), "David");

    ASSERT_EQ(r1->GetRefCount(), 2);
  }

  SIMPLE_TEST(MoveCtor) {
    Ref<Robot> r1 = New<Robot>("Jake");
    Ref<Robot> r2(std::move(r1));

    ASSERT_FALSE(r1.IsValid());
    ASSERT_TRUE(r2.IsValid());

    ASSERT_EQ(r2->Name(), "Jake");
    ASSERT_EQ(r2->GetRefCount(), 1);
  }

  SIMPLE_TEST(Assigment) {
    Ref<Robot> r1;

    ASSERT_FALSE(r1.IsValid());

    {
      Ref<Robot> r2 = New<Robot>("Bob");
      r1 = r2;
    }

    ASSERT_TRUE(r1.IsValid());
    ASSERT_EQ(r1->Name(), "Bob");
  }

  SIMPLE_TEST(Convert) {
    Ref<Y> ry = New<Y>();
    Ref<X> rx{ry};
  }

  SIMPLE_TEST(MoveConvert) {
    Ref<Y> ry = New<Y>();
    Ref<X> rx{std::move(ry)};

    ASSERT_TRUE(!ry.IsValid());
  }

  SIMPLE_TEST(Release) {
    Ref<Robot> r = New<Robot>("Jake");

    Robot* raw_ptr = r.Release();

    ASSERT_FALSE(r.IsValid());

    ASSERT_EQ(raw_ptr->Name(), "Jake");
    raw_ptr->ReleaseRef();
  }

  SIMPLE_TEST(Adopt) {
    auto* raw_ptr = new Robot("Bob");

    ASSERT_EQ(raw_ptr->GetRefCount(), 1);
    auto ref = Adopt(raw_ptr);
    ASSERT_EQ(ref->GetRefCount(), 1);
  }

  SIMPLE_TEST(RefFromThis) {
    auto ref1 = New<Robot>("Mary");
    auto ref2 = ref1->RefFromThis();

    ASSERT_EQ(ref1->GetRefCount(), 2);
    ASSERT_EQ(ref2->Name(), "Mary");
  }
}

RUN_ALL_TESTS()
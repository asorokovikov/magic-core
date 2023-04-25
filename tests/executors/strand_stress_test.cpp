#include <gtest/gtest.h>

#include <magic/executors/execute.h>
#include <magic/executors/strand.h>
#include <magic/executors/manual.h>
#include <magic/executors/thread_pool.h>

#include <magic/common/stopwatch.h>

using namespace magic;

//////////////////////////////////////////////////////////////////////

bool KeepRunning(magic::Duration duration = 10s) {
  static Stopwatch sw;
  return sw.Elapsed() < duration;
}

//////////////////////////////////////////////////////////////////////

class Robot {
 public:
  explicit Robot(IExecutor& e) : strand_(e) {
  }

  void Cmd() {
    Execute(strand_, [this]() {
      Step();
    });
  }

  size_t Steps() const {
    return steps_;
  }

 private:
  void Step() {
    ++steps_;
  }

 private:
  Strand strand_;
  size_t steps_{0};
};

//////////////////////////////////////////////////////////////////////

void Robots(size_t strands, size_t load, magic::Duration duration) {
  ThreadPool pool{4};

  std::deque<Robot> robots;
  for (size_t i = 0; i < strands; ++i) {
    robots.emplace_back(pool);
  }

  ThreadPool clients{strands};

  size_t iters = 0;
  magic::Stopwatch sw;
  while (KeepRunning(duration)) {
    ++iters;

    for (auto& robot : robots) {
      Execute(clients, [&robot, load]() {
        for (size_t j = 0; j < load; ++j) {
          robot.Cmd();
        }
      });
    }

    clients.WaitIdle();
    pool.WaitIdle();
  }

  for (auto& robot : robots) {
    ASSERT_EQ(robot.Steps(), iters * load);
  }

  pool.Stop();
  clients.Stop();
}

//////////////////////////////////////////////////////////////////////

void MissingTasks(magic::Duration duration) {
  ThreadPool pool{4};

  size_t iter = 0;

  while (KeepRunning(duration)) {
    Strand strand(pool);

    size_t todo = 2 + (iter++) % 5;

    size_t done = 0;

    for (size_t i = 0; i < todo; ++i) {
      Execute(strand, [&done]() {
        ++done;
      });
    }

    pool.WaitIdle();

    ASSERT_EQ(done, todo);
  }

  pool.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST(StrandStress, Robots_1) {
  Robots(30, 30, 5s);
}

TEST(StrandStress, Robots_2) {
  Robots(50, 20, 5s);
}

TEST(StrandStress, MissingTasks) {
  MissingTasks(5s);
}
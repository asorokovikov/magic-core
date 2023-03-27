#include <gtest/gtest.h>

#include <magic/coroutine/coroutine.h>
#include <magic/fibers/core/stack.h>

#include <fmt/core.h>
#include <chrono>
#include <thread>

using namespace magic;

//////////////////////////////////////////////////////////////////////

void PrintAllocatorMetrics(const AllocatorMetrics& metrics) {
  fmt::println("Allocator metrics");
  fmt::println("Total allocate: {} ({} KB)", metrics.total_allocate,
               metrics.total_allocate_bytes / (1024));
  fmt::println("AllocateNew: {} times", metrics.allocate_new_count);
  fmt::println("Reused: {} times", metrics.total_allocate - metrics.allocate_new_count);
  fmt::println("Release: {} times", metrics.release_count);
}

//////////////////////////////////////////////////////////////////////

TEST(Coroutine, JustWorks) {
    Coroutine co([] {
        Coroutine::Suspend();
    });

    ASSERT_FALSE(co.IsCompleted());
    co.Resume();
    ASSERT_FALSE(co.IsCompleted());
    co.Resume();
    ASSERT_TRUE(co.IsCompleted());

    PrintAllocatorMetrics(GetAllocatorMetrics());
}

TEST(Coroutine, Interleaving) {
    size_t step = 0;

    Coroutine a([&] {
        ASSERT_EQ(step, 0);
        step = 1;
        Coroutine::Suspend();
        ASSERT_EQ(step, 2);
        step = 3;
    });

    Coroutine b([&] {
        ASSERT_EQ(step, 1);
        step = 2;
        Coroutine::Suspend();
        ASSERT_EQ(step, 3);
        step = 4;
    });

    a.Resume();
    b.Resume();

    ASSERT_EQ(step, 2);

    a.Resume();
    b.Resume();

    ASSERT_EQ(step, 4);

    ASSERT_TRUE(a.IsCompleted());
    ASSERT_TRUE(b.IsCompleted());

    PrintAllocatorMetrics(GetAllocatorMetrics());
}

//////////////////////////////////////////////////////////////////////

struct Threads {
    template <typename Task>
    void Execute(Task task) {
        std::thread t([task = std::move(task)]() mutable {
            task();
        });
        t.join();
    }
};

TEST(Coroutine, Threads) {
    size_t steps = 0;

    Coroutine co([&] {
        std::cout << "Step" << std::endl;
        ++steps;
        Coroutine::Suspend();
        std::cout << "Step" << std::endl;
        ++steps;
        Coroutine::Suspend();
        std::cout << "Step" << std::endl;
        ++steps;
    });

    auto resume = [&] {
        co.Resume();
    };

    // Simulate fiber running in thread pool
    Threads threads;
    threads.Execute(resume);
    threads.Execute(resume);
    threads.Execute(resume);

    ASSERT_EQ(steps, 3);

    PrintAllocatorMetrics(GetAllocatorMetrics());
}

//////////////////////////////////////////////////////////////////////

struct TreeNode;
using TreeNodePtr = std::shared_ptr<TreeNode>;

struct TreeNode {
    TreeNodePtr left;
    TreeNodePtr right;
    std::string data;

    TreeNode(std::string data, TreeNodePtr left, TreeNodePtr right)
            : left(std::move(left)), right(std::move(right)), data(std::move(data)) {
    }

    static TreeNodePtr CreateFork(std::string data, TreeNodePtr left, TreeNodePtr right) {
        return std::make_shared<TreeNode>(std::move(data), std::move(left), std::move(right));
    }

    static TreeNodePtr CreateLeaf(std::string data) {
        return std::make_shared<TreeNode>(std::move(data), nullptr, nullptr);
    }
};

class TreeIterator {
public:
    explicit TreeIterator(TreeNodePtr root)
            : walker_([this, root] {
        TreeWalk(root);
    }) {
    }

    bool MoveNext() {
        walker_.Resume();
        return !walker_.IsCompleted();
    }

    auto Data() const {
        return data_;
    }

private:
    void TreeWalk(TreeNodePtr node) {
        if (node->left) {
            TreeWalk(node->left);
        }

        data_ = node->data;
        Coroutine::Suspend();

        if (node->right) {
            TreeWalk(node->right);
        }
    }

private:
    Coroutine walker_;
    std::string_view data_;
};

TEST(Coroutine, TreeWalk) {
    auto root = TreeNode::CreateFork(
            "B", TreeNode::CreateLeaf("A"),
            TreeNode::CreateFork(
                    "F", TreeNode::CreateFork("D", TreeNode::CreateLeaf("C"), TreeNode::CreateLeaf("E")),
                    TreeNode::CreateLeaf("G")));

    std::string traversal;

    TreeIterator it(root);
    while (it.MoveNext()) {
        traversal += it.Data();
    }

    ASSERT_EQ(traversal, "ABCDEFG");

    PrintAllocatorMetrics(GetAllocatorMetrics());
}

//////////////////////////////////////////////////////////////////////

TEST(Coroutine, Pipeline) {
    const size_t kSteps = 123;

    size_t counter = 0;

    Coroutine a([&] {
        Coroutine b([&] {
            for (size_t index = 0; index < kSteps; ++index) {
                ++counter;
                Coroutine::Suspend();
            }
        });

        while (not b.IsCompleted()) {
            b.Resume();
            Coroutine::Suspend();
        }
    });

    while (not a.IsCompleted()) {
        a.Resume();
    }

    ASSERT_EQ(counter, kSteps);

    PrintAllocatorMetrics(GetAllocatorMetrics());
}

//////////////////////////////////////////////////////////////////////

struct MyException {};

TEST(Coroutine, Exception) {
    Coroutine co([&] {
        Coroutine::Suspend();
        throw MyException{};
        Coroutine::Suspend();
    });

    ASSERT_FALSE(co.IsCompleted());
    co.Resume();
    ASSERT_THROW(co.Resume(), MyException);
    ASSERT_TRUE(co.IsCompleted());


    PrintAllocatorMetrics(GetAllocatorMetrics());
}

TEST(Coroutine, NestedException1) {
    Coroutine a([] {
        Coroutine b([] {
            throw MyException{};
        });
        ASSERT_THROW(b.Resume(), MyException);
    });

    a.Resume();
    ASSERT_TRUE(a.IsCompleted());

    PrintAllocatorMetrics(GetAllocatorMetrics());
}

TEST(Coroutine, NestedException2) {
    Coroutine a([] {
        Coroutine b([] {
            throw MyException{};
        });
        b.Resume();
    });

    ASSERT_THROW(a.Resume(), MyException);
    ASSERT_TRUE(a.IsCompleted());

    PrintAllocatorMetrics(GetAllocatorMetrics());
}

TEST(Coroutine, ExceptionsHard) {
    int score = 0;

    Coroutine a([&]() {
        Coroutine b([]() {
            throw 1;
        });
        try {
            b.Resume();
        } catch (int) {
            ++score;
            // Context switch during stack unwinding
            Coroutine::Suspend();
            throw;
        }
    });

    a.Resume();

    std::thread t([&]() {
        try {
            a.Resume();
        } catch (int) {
            ++score;
        }
    });
    t.join();

    ASSERT_EQ(score, 2);

    PrintAllocatorMetrics(GetAllocatorMetrics());
}


TEST(Coroutine, Leak) {
    auto shared_ptr = std::make_shared<int>(42);
    std::weak_ptr<int> weak_ptr = shared_ptr;

    {
        auto routine = [ptr = std::move(shared_ptr)]() {};
        Coroutine co(routine);
        co.Resume();
    }

    ASSERT_FALSE(weak_ptr.lock());

    PrintAllocatorMetrics(GetAllocatorMetrics());
}

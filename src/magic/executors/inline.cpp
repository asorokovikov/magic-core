#include <magic/executors/inline.h>

namespace magic {

class InlineExecutor final : public IExecutor {
 public:
  void Execute(TaskNode* task) override {
    task->Run();
  }
};

IExecutor& GetInlineExecutor() {
  static InlineExecutor executor;
  return executor;
}

}
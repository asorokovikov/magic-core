#include <fmt/core.h>

#include <magic/coroutine/coroutine.h>
#include <magic/coroutine/processor.h>

using namespace magic;


void ProcessorExample() {

}

int main() {
  fmt::println("Coroutine example");

  Coroutine co([]{
    fmt::println("Hello from Coroutine");
    Coroutine::Suspend();
    fmt::println("Hello from Coroutine again");
  });

  fmt::println("Try to resume coroutine");
  co.Resume();
  co.Resume();

  return 0;
}
#include <fmt/core.h>
#include <fmt/std.h>

#include <magic/fibers/api.h>
#include <magic/common/stopwatch.h>
#include <magic/common/meta.h>

#include <magic/futures/core/future.h>

using namespace magic;

//////////////////////////////////////////////////////////////////////

void Application() {
  //  Http::GetString("google.com").Then([](auto response) {
  //    fmt::println(response);
  //  });

  auto [f, p] = MakeContract<std::string>();

}

//  auto result = Http.GetString("google.com", 60s);
//  Http.GetString("https://jsonplaceholder.typicode.com/todos/1")
//      .Then(response => printf(response));

int main() {
  RunScheduler(4, []() {
    Application();
  });

  return 0;
}
#include <magic/common/random.h>
#include <magic/common/generators/random_generator.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

static thread_local RandomGenerator generator;

//////////////////////////////////////////////////////////////////////

uint32_t Random::Next() {
  return generator.Next();
}

uint32_t magic::Random::Next(uint32_t max_value) {
  return generator.Next(max_value);
}

uint32_t Random::Next(uint32_t min_value, uint32_t max_value) {
  return generator.Next(min_value, max_value);
}

uint64_t Random::NextInt64() {
  return generator.NextInt64();
}

}  // namespace magic

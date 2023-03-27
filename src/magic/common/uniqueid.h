#pragma once
#include <atomic>

namespace magic {

//////////////////////////////////////////////////////////////////////

class UniqueIdGenerator final {
public:
    size_t Next() {
        return last_id_.fetch_add(1);
    }

private:
    std::atomic<size_t> last_id_ = 0;
};

//////////////////////////////////////////////////////////////////////

} // namespace name
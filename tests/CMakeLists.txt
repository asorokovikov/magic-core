include(GoogleTest)

# common

add_test_executable(shared_ptr_test common/shared_ptr_test.cpp)
add_test_executable(result_test common/result.cpp)
add_test_executable(string_reader_test common/string_reader_test.cpp)

# coroutine

add_test_executable(coroutine_test coroutine/coroutine_test.cpp)
add_test_executable(processor_test coroutine/processor_test.cpp)
add_test_executable(generator_test coroutine/generator_test.cpp)
add_test_executable(stackless_mutex_test coroutine/stackless/sync/stackless_mutex_test.cpp)
add_test_executable(stackless_waitgroup_test coroutine/stackless/sync/stackless_waitgroup_test.cpp)

# fibers

add_test_executable(fibers_test fibers/fibers_test.cpp)
add_test_executable(mutex_test fibers/mutex_test.cpp)
add_test_executable(condvar_test fibers/condvar_test.cpp)
add_test_executable(wait_group_test fibers/wait_group_test.cpp)

# executors

add_test_executable(thread_pool_test executors/thread_pool_test.cpp)
add_test_executable(manual_test executors/manual_test.cpp)
add_test_executable(strand_test executors/strand_test.cpp)
add_test_executable(strand_stress_test executors/strand_stress_test.cpp)

# concurrency

add_test_executable(lock_free_stack_test concurrency/lockfree/lock_free_stack_test.cpp)
add_test_executable(lock_free_queue_test concurrency/lockfree/lock_free_queue_test.cpp)
add_test_executable(stamped_ptr_test concurrency/lockfree/stamped_ptr_test.cpp)
add_test_executable(lock_free_intrusive_stack_test concurrency/lockfree/lock_free_intrusive_stack_test.cpp)
add_test_executable(lock_free_intrusive_queue_test concurrency/lockfree/lock_free_intrusive_queue_test.cpp)

# filesystem

add_test_executable(file_test filesystem/file_test.cpp)

# futures

add_test_executable(futures_test futures/futures_test.cpp)

# net

add_test_executable(net_test net/net_test.cpp)
add_test_executable(http_header_test net/http_header_test.cpp)
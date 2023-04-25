cmake_minimum_required(VERSION 3.14)

project(magic)

include(cmake/Logging.cmake)
include(cmake/Ccache.cmake)
include(cmake/CheckCompiler.cmake)
include(cmake/CompileOptions.cmake)
include(cmake/Sanitize.cmake)
include(cmake/Platform.cmake)
include(cmake/Dependencies.cmake)
include(cmake/Helpers.cmake)
include(cmake/SetupLinker.cmake)

option(MAGIC_TESTS "Enable tests" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

add_subdirectory(src)
add_subdirectory(examples)

if (MAGIC_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
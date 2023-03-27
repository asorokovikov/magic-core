
function(add_test_executable BINARY_NAME)
    set(BINARY_SOURCES ${ARGN})

    if (ARGV1)
        add_executable(${BINARY_NAME} ${BINARY_SOURCES})
    else ()
        add_executable(${BINARY_NAME} ${BINARY_NAME}.cpp)
    endif ()
    target_link_libraries(${BINARY_NAME} GTest::gtest_main ${PROJECT_NAME})
#    add_test(${BINARY_NAME} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BINARY_NAME})
    gtest_discover_tests(${BINARY_NAME})
endfunction()

# Example

function(add_example DIR_NAME)
    set(TARGET_NAME "${DIR_NAME}_example")
    file(GLOB_RECURSE TARGET_CXX_SOURCES ${DIR_NAME}/*.cpp)
    add_executable(${TARGET_NAME} ${TARGET_CXX_SOURCES})
    target_link_libraries(${TARGET_NAME} pthread ${PROJECT_NAME})
endfunction()
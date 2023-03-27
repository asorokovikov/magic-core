get_filename_component(LIB_INCLUDE_PATH "." ABSOLUTE)
get_filename_component(LIB_PATH "." ABSOLUTE)

file(GLOB_RECURSE LIB_CXX_SOURCES ${LIB_PATH}/*.cpp)
file(GLOB_RECURSE LIB_ASM_SOURCES ${LIB_PATH}/*.S)
file(GLOB_RECURSE LIB_HEADERS ${LIB_PATH}/*.h)

enable_language(ASM)

set(LIB_LIST "wheels;fmt;function2;sure;asio;twist")

add_library(${PROJECT_NAME} STATIC ${LIB_CXX_SOURCES} ${LIB_ASM_SOURCES} ${LIB_HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${LIB_INCLUDE_PATH})
target_link_libraries(${PROJECT_NAME} PUBLIC pthread ${LIB_LIST})
add_dependencies(${PROJECT_NAME} ${LIB_LIST})
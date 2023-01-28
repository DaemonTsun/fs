
cmake_minimum_required(VERSION 3.10)

set(DEMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DEMO_BIN "${CMAKE_CURRENT_BINARY_DIR}")
set(DEMO_SRC "${DEMO_DIR}/src")
file(GLOB_RECURSE DEMO_HEADERS "${DEMO_SRC}/*.h" "${DEMO_SRC}/*.hpp")
file(GLOB_RECURSE DEMO_SOURCES "${DEMO_SRC}/*.c" "${DEMO_SRC}/*.cpp")

add_executable(filesystem_watcher_demo)
target_sources(filesystem_watcher_demo PUBLIC ${DEMO_SOURCES})
target_include_directories(filesystem_watcher_demo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../src")

target_link_libraries(filesystem_watcher_demo fs shl pthread)

# run
add_custom_target("run_filesystem_watcher_demo" COMMAND "${DEMO_BIN}/filesystem_watcher_demo")

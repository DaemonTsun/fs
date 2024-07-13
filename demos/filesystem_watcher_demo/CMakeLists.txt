cmake_minimum_required(VERSION 3.10)

set(DEMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DEMO_BIN "${CMAKE_CURRENT_BINARY_DIR}")
set(DEMO_SRC "${DEMO_DIR}/src")
find_sources(DEMO_SOURCES "${DEMO_SRC}")
find_headers(DEMO_HEADERS "${DEMO_SRC}")


add_executable(filesystem_watcher_demo)
set_property(TARGET filesystem_watcher_demo PROPERTY CXX_STANDARD 20)
target_compile_options(filesystem_watcher_demo PRIVATE ${fs_COMPILE_FLAGS})
target_compile_definitions(filesystem_watcher_demo PRIVATE -DUNICODE=1)
target_sources(filesystem_watcher_demo PRIVATE ${DEMO_SOURCES})
target_include_directories(filesystem_watcher_demo PRIVATE "${fs_SOURCES_DIR}" ${fs_INCLUDE_DIRECTORIES})

target_link_libraries(filesystem_watcher_demo ${fs_TARGET})

# run
add_custom_target("run_filesystem_watcher_demo" COMMAND "${DEMO_BIN}/filesystem_watcher_demo")

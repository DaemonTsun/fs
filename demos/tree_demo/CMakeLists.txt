cmake_minimum_required(VERSION 3.10)

set(DEMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DEMO_BIN "${CMAKE_CURRENT_BINARY_DIR}")
set(DEMO_SRC "${DEMO_DIR}/src")
find_sources(DEMO_SOURCES "${DEMO_SRC}")
find_headers(DEMO_HEADERS "${DEMO_SRC}")


add_executable(tree_demo)
set_property(TARGET tree_demo PROPERTY CXX_STANDARD 20)
target_compile_options(tree_demo PRIVATE ${fs_COMPILE_FLAGS})
target_link_options(tree_demo PRIVATE ${fs_LINK_FLAGS})
target_compile_definitions(tree_demo PRIVATE -DUNICODE=1)
target_sources(tree_demo PRIVATE ${DEMO_SOURCES})
target_include_directories(tree_demo PRIVATE "${fs_SOURCES_DIR}" ${fs_INCLUDE_DIRECTORIES})

target_link_libraries(tree_demo ${fs_TARGET})

# run
add_custom_target("run_tree_demo" COMMAND "${DEMO_BIN}/tree_demo")

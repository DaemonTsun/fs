cmake_minimum_required(VERSION 3.10)

set(DEMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DEMO_BIN "${CMAKE_CURRENT_BINARY_DIR}")
set(DEMO_SRC "${DEMO_DIR}/src")
find_sources(DEMO_SOURCES "${DEMO_SRC}")
find_headers(DEMO_HEADERS "${DEMO_SRC}")


add_executable(iterator_demo)
target_sources(iterator_demo PUBLIC ${DEMO_SOURCES})
target_include_directories(iterator_demo PRIVATE "${fs_SOURCES_DIR}" "${shl_SOURCES_DIR}")

target_link_libraries(iterator_demo ${fs_TARGET} ${shl_TARGET} pthread)

# run
add_custom_target("run_iterator_demo" COMMAND "${DEMO_BIN}/iterator_demo")

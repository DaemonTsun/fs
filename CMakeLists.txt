cmake_minimum_required(VERSION 3.10)
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/better-cmake/cmake/betterConfig.cmake")
    execute_process(COMMAND git submodule update --init "${CMAKE_CURRENT_SOURCE_DIR}/ext/better-cmake" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}") 
endif()
find_package(better REQUIRED PATHS ext/better-cmake/cmake)

project(fs)
project_author(fs "DaemonTsun")

set(TEST_SOURCE_DIR ${ROOT}/tests)

# library
add_lib(fs STATIC
    VERSION 0.7
    SOURCES_DIR "${ROOT}/src/"
    GENERATE_TARGET_HEADER "${ROOT}/src/fs/fs_info.hpp"
    CPP_VERSION 17
    CPP_WARNINGS ALL SANE FATAL
    LIBRARIES
        @Linux pthread

    EXT
        LIB shl 0.8.0 "${ROOT}/ext/shl" INCLUDE LINK GIT_SUBMODULE

    TESTS "${TEST_SOURCE_DIR}"
    )

exit_if_included()

add_subdirectory("${CMAKE_SOURCE_DIR}/demos")

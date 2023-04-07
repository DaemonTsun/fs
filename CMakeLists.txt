cmake_minimum_required(VERSION 3.10)
find_package(better REQUIRED PATHS ext/better-cmake/cmake)

project(fs)
project_author(fs "DaemonTsun")

set(TEST_SOURCE_DIR ${ROOT}/tests)

# library
add_lib(fs STATIC
    VERSION 0.6.1
    SOURCES_DIR "${ROOT}/src/"
    GENERATE_TARGET_HEADER "${ROOT}/src/fs/fs_info.hpp"
    CPP_VERSION 20
    CPP_WARNINGS ALL SANE
    LIBRARIES pthread

    EXT
    LIB shl 0.6.1 "${ROOT}/ext/shl" INCLUDE LINK GIT_SUBMODULE

    TESTS "${TEST_SOURCE_DIR}"
    )

exit_if_included()

add_subdirectory("${CMAKE_SOURCE_DIR}/demos")

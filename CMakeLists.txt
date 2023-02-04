cmake_minimum_required(VERSION 3.10)
find_package(better REQUIRED NO_DEFAULT_PATH PATHS ext/better-cmake/cmake)

project(fs VERSION 0.5.2)
get_version_target(fs_TARGET ${PROJECT_NAME})

if(TARGET ${fs_TARGET})
    return()
endif()

#settings
set(CMAKE_CXX_STANDARD 20)

set(fs_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
find_sources(fs_SOURCES "${fs_SOURCES_DIR}")
find_headers(fs_HEADERS "${fs_SOURCES_DIR}")

add_subdirectory(ext/shl)

add_library(${fs_TARGET} SHARED)
target_sources(${fs_TARGET} PUBLIC ${fs_HEADERS} ${fs_SOURCES})
message("${shl_SOURCES_DIR}")
target_include_directories(${fs_TARGET} PRIVATE "${fs_SOURCES_DIR}" "${shl_SOURCES_DIR}")
set_property(TARGET ${fs_TARGET} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${fs_TARGET} PRIVATE ${shl_TARGET} pthread)

if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    export_library_variables(fs)
    return()
endif()

# install
install_library(TARGET ${fs_TARGET} HEADERS ${fs_HEADERS})

# tests
set(TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

find_package(t1 QUIET)
if(NOT DEFINED t1_DIR OR t1_DIR)
    add_test_directory("${TEST_SOURCE_DIR}" INCLUDE_DIRS "${fs_SOURCES_DIR}" "${shl_SOURCES_DIR}"
                                            LIBRARIES ${fs_TARGET} pthread)
    register_tests()
endif()

add_subdirectory("${CMAKE_SOURCE_DIR}/demos")

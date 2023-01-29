cmake_minimum_required(VERSION 3.10)
if(TARGET fs)
    return()
endif()

add_compile_options (-fdiagnostics-color=always)

#settings
set(FS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")
file(GLOB_RECURSE FS_SOURCES "${FS_SRC_DIR}/*.c" "${FS_SRC_DIR}/*.cpp")
file(GLOB_RECURSE FS_HEADERS "${FS_SRC_DIR}/*.h" "${FS_SRC_DIR}/*.hpp")

project(fs)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(ext/shl)

add_library(fs SHARED)
target_include_directories(fs PUBLIC "${FS_SRC_DIR}" "${SHL_SRC_DIR}")
target_sources(fs PUBLIC ${FS_HEADERS} ${FS_SOURCES})
set_property(TARGET fs PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(fs shl pthread)

if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # included file
    set(FS_SRC_DIR "${FS_SRC_DIR}" PARENT_SCOPE)
    set(FS_SOURCES ${FS_SOURCES} PARENT_SCOPE)
    set(FS_HEADERS ${FS_HEADERS} PARENT_SCOPE)
else()
    # install
    install(FILES ${FS_HEADERS} DESTINATION /usr/local/include/)
    install(FILES ${FS_HEADERS} DESTINATION "include/${PROJECT_NAME}")
    install(TARGETS "${PROJECT_NAME}"
            RUNTIME DESTINATION "bin"
            LIBRARY DESTINATION "lib"
            ARCHIVE DESTINATION "lib/${PROJECT_NAME}")

    # tests
    set(TEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

    find_package(t1 QUIET)
    if(NOT DEFINED t1_DIR OR t1_DIR)
        add_test_directory("${TEST_SRC_DIR}" INCLUDE_DIRS "${FS_SRC_DIR}" LIBRARIES fs)
        register_tests()
    endif()

    add_subdirectory("${CMAKE_SOURCE_DIR}/demos")
endif()

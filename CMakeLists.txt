cmake_minimum_required(VERSION 3.28)
project(SimpleFS C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

option(ENABLE_ASAN "Enable AddressSanitizer for test targets" OFF)

add_executable(SimpleFS
        disk_emu.c
        sfs_test.c
        sfs_api.c
        sfs_util.c)

# Unit tests via CTest
enable_testing()

# separate test executable for utils
add_executable(sfs_unittests_utils
        tests/test_utils.c
        disk_emu.c
        sfs_util.c
        sfs_api.c)

# separate test executable for sfs api
add_executable(sfs_unittests_api
        tests/test_sfs_api.c
        disk_emu.c
        sfs_api.c
        sfs_util.c)

# If ASAN is enabled, compile and link test targets with sanitizers
if(ENABLE_ASAN)
    message(STATUS "ENABLE_ASAN=ON: adding AddressSanitizer flags to test targets")
    # compile options
    target_compile_options(sfs_unittests_utils PRIVATE -g -O0 -fsanitize=address -fno-omit-frame-pointer)
    target_compile_options(sfs_unittests_api   PRIVATE -g -O0 -fsanitize=address -fno-omit-frame-pointer)
    # link options
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.13")
        target_link_options(sfs_unittests_utils PRIVATE -fsanitize=address)
        target_link_options(sfs_unittests_api   PRIVATE -fsanitize=address)
    else()
        target_link_libraries(sfs_unittests_utils PRIVATE -fsanitize=address)
        target_link_libraries(sfs_unittests_api   PRIVATE -fsanitize=address)
    endif()
endif()

add_test(NAME sfs_unittests_utils COMMAND sfs_unittests_utils)
add_test(NAME sfs_unittests_api COMMAND sfs_unittests_api)

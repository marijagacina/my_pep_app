# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(TEST_SOURCES
    test.cpp
)

include_directories(
    ${ZEPHYR_BASE}/tests/include
    ../src
)

project(my_pep_app_tests)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})


find_package(GTest REQUIRED)
include_directories(/usr/src/googletest/googlemock/include)

add_executable(tests test.cpp)

target_link_libraries(tests ${GTEST_LIBRARIES} gmock pthread)

cmake_minimum_required(VERSION 3.11)
project(GarlicModelTests)

find_package(GTest REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(TEST_SOURCES main.cpp)

add_executable(GarlicModelTests ${TEST_SOURCES})
target_link_libraries(GarlicModelTests Threads::Threads ${GTEST_BOTH_LIBRARIES})

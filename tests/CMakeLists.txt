cmake_minimum_required(VERSION 3.11)
project(GarlicModelTests)

find_package(GTest REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(TEST_SOURCES test_json.cpp test_default_values.cpp)

add_executable(GarlicModelTests ${TEST_SOURCES})
target_link_libraries(GarlicModelTests GarlicModel Threads::Threads ${GTEST_BOTH_LIBRARIES})

file(COPY data DESTINATION ${CMAKE_BINARY_DIR}/bin)

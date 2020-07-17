cmake_minimum_required(VERSION 3.11)
project(GarlicModelTests)

find_package(GTest REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
enable_testing()

set(TEST_SOURCES test_json.cpp test_basic.cpp test_yaml.cpp test_protocol.h)

add_executable(GarlicModelTests ${TEST_SOURCES})
target_link_libraries(GarlicModelTests GarlicModel yaml Threads::Threads ${GTEST_BOTH_LIBRARIES})


file(COPY data DESTINATION ${CMAKE_BINARY_DIR}/bin)

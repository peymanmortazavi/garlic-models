cmake_minimum_required(VERSION 3.11)
project(GarlicModelTests)

find_package(GTest REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

option(GARLIC_BUILD_PERFORMANCE_TESTS "Build performance tests." OFF)
set(GARLIC_BUILD_PERFORMANCE_TESTS OFF)

option(GARLIC_BUILD_LAB "Build lab tests." OFF)
set(GARLIC_BUILD_LAB OFF)

enable_testing()

set(TEST_SOURCES 
    all_tests.cpp
    test_json.cpp
    test_clove.cpp
    test_yaml.cpp
    test_models.cpp
    test_module.cpp
    test_constraints.cpp
    test_helpers.cpp
    test_utility.cpp)

add_executable(GarlicModelTests ${TEST_SOURCES})
target_link_libraries(GarlicModelTests GarlicModel yaml-cpp yaml Threads::Threads ${GTEST_BOTH_LIBRARIES})

IF(GARLIC_BUILD_PERFORMANCE_TESTS)
    add_executable(PerformanceTests performance.cpp test_utility.cpp)
    target_link_libraries(PerformanceTests GarlicModel yaml-cpp yaml Threads::Threads ${GTEST_BOTH_LIBRARIES} benchmark)
ENDIF()

IF(GARLIC_BUILD_LAB)
    add_executable(Lab lab.cpp)
    target_link_libraries(Lab GarlicModel)
ENDIF()

file(COPY data DESTINATION ${CMAKE_BINARY_DIR}/bin)

cmake_minimum_required(VERSION 3.11)
project(GarlicModelTests)

find_package(GTest REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

option(GARLIC_BUILD_PERFORMANCE_TESTS "Build performance tests." ON)

option(GARLIC_TEST_PROVIDERS "Build tests for the built-in adapters." ON)

option(GARLIC_BUILD_LAB "Build lab tests." ON)

enable_testing()

set(TEST_SOURCES 
    all_tests.cpp
    test_clove.cpp
    test_models.cpp
    test_module.cpp
    test_module_parsing.cpp
    test_encoding.cpp
    test_constraints.cpp
    test_containers.cpp
    test_helpers.cpp
    test_utility.cpp)

add_executable(GarlicModelTests ${TEST_SOURCES})
target_link_libraries(GarlicModelTests GarlicModel yaml-cpp yaml Threads::Threads ${GTEST_BOTH_LIBRARIES})

IF(GARLIC_TEST_PROVIDERS)
    add_subdirectory(adapters)
ENDIF()

IF(GARLIC_BUILD_PERFORMANCE_TESTS)
    add_executable(PerformanceTests performance.cpp test_utility.cpp)
    target_link_libraries(PerformanceTests GarlicModel yaml-cpp yaml Threads::Threads ${GTEST_BOTH_LIBRARIES} benchmark)
ENDIF()

IF(GARLIC_BUILD_LAB)
    add_executable(Lab lab.cpp)
    target_link_libraries(Lab GarlicModel)
ENDIF()

file(COPY data DESTINATION ${CMAKE_BINARY_DIR}/bin)

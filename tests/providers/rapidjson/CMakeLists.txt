find_package(GTest REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(RapidJsonTests test_json.cpp)
target_link_libraries(RapidJsonTests GarlicModel Threads::Threads ${GTEST_BOTH_LIBRARIES})

add_test(RapidJsonTests RapidJsonTests)
set_tests_properties(RapidJsonTests PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

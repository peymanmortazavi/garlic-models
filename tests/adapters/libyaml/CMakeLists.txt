find_package(GTest REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(LibYamlTests test_yaml.cpp)
target_link_libraries(LibYamlTests GarlicModel yaml Threads::Threads ${GTEST_BOTH_LIBRARIES})

add_test(LibYamlTests LibYamlTests)
set_tests_properties(LibYamlTests PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

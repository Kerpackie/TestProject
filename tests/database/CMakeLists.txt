add_executable(database_test database_test.cpp)

target_link_libraries(database_test
  PRIVATE
    database
    GTest::gtest_main
)

# Register each gtest case as an individual CTest test.
gtest_discover_tests(database_test)

add_executable(core_test core_test.cpp)

target_link_libraries(core_test
  PRIVATE
    core
    GTest::gtest_main
)

# Register each gtest case as an individual CTest test.
gtest_discover_tests(core_test)

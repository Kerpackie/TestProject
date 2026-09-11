# Make gtest_discover_tests() available to test subdirectories.
include(GoogleTest)

add_subdirectory(core)
add_subdirectory(database)

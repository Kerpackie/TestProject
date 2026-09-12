cmake_minimum_required(VERSION 3.20)

project(ApplicationStarterTemplate
  VERSION 0.1.0
  DESCRIPTION "C++ desktop app starter template with Dear ImGui and SQLite"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch external dependencies before applying strict warning flags so that
# third-party code (GoogleTest) is not built with -Werror.
include(cmake/dependencies.cmake)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_subdirectory(libs/core)
add_subdirectory(libs/database)
add_subdirectory(libs/views)
add_subdirectory(app)
add_subdirectory(tests)

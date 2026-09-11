# External dependencies, fetched at configure time.
# Keep this list minimal; add entries only when a real need appears.

include(FetchContent)

# GoogleTest — pinned to a specific release for reproducible builds.
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0
  GIT_SHALLOW TRUE
)

# Build gtest into our tree; never install it.
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# SQLiteCpp — pinned to a specific release for reproducible builds.
FetchContent_Declare(
  SQLiteCpp
  GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
  GIT_TAG 3.3.3
  GIT_SHALLOW TRUE
)

set(SQLITECPP_INTERNAL_SQLITE ON CACHE BOOL "" FORCE)
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SQLiteCpp)

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

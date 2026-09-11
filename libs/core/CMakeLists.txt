add_library(core STATIC
  src/core.cpp
)

# Public interface: only include/ is exposed to consumers.
# Private headers (src/detail.h) are included by filename from within src/.
target_include_directories(core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

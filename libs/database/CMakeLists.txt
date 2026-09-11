add_library(database STATIC
  src/database.cpp
  src/database_factory.cpp
  src/transaction.cpp
)

# Public interface: only include/ is exposed to consumers.
# Private headers (src/detail.h) are included by filename from within src/.
target_include_directories(database
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(database
  PRIVATE
    SQLiteCpp
)

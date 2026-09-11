add_library(database STATIC
  src/database.cpp
  src/database_factory.cpp
  src/transaction.cpp
  src/repository/sqlite_user_repository.cpp
  src/service/user_service.cpp
  src/adapter/sqlite_session.cpp
  src/adapter/postgres_adapter.cpp
  src/adapter/mariadb_adapter.cpp
  src/concurrency/connection_pool.cpp
  src/concurrency/retry_policy.cpp
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

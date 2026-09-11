#include "database/adapter/sqlite_session.h"

#include <SQLiteCpp/Exception.h>
#include "database/error/database_error.h"

namespace database::adapter {

SqliteTransaction::SqliteTransaction(Connection& connection)
    : tx_(connection) {}

void SqliteTransaction::commit() {
  try {
    tx_.commit();
  } catch (const SQLite::Exception& e) {
    if (e.getErrorCode() == 19 || e.getExtendedErrorCode() == 2067) {  // SQLITE_CONSTRAINT / SQLITE_CONSTRAINT_UNIQUE
      throw error::ConflictException(e.what(), error::EngineType::SQLite);
    }
    throw error::DatabaseException(e.what(), error::EngineType::SQLite);
  }
}

bool SqliteTransaction::is_committed() const noexcept {
  return tx_.is_committed();
}

SqliteSession::SqliteSession(DatabaseConfig config)
    : connection_(config) {}

std::unique_ptr<session::ITransaction> SqliteSession::begin_transaction() {
  return std::make_unique<SqliteTransaction>(connection_);
}

void SqliteSession::execute(std::string_view sql) {
  try {
    connection_.execute(std::string(sql));
  } catch (const SQLite::Exception& e) {
    if (e.getErrorCode() == 19 || e.getExtendedErrorCode() == 2067 || std::string(e.what()).find("UNIQUE") != std::string::npos) {
      throw error::ConflictException(e.what(), error::EngineType::SQLite);
    }
    throw error::DatabaseException(e.what(), error::EngineType::SQLite);
  }
}

int SqliteSession::execute_scalar_int(std::string_view sql) {
  try {
    return connection_.execute_scalar_int(std::string(sql));
  } catch (const SQLite::Exception& e) {
    throw error::DatabaseException(e.what(), error::EngineType::SQLite);
  }
}

bool SqliteSession::is_open() const noexcept {
  return connection_.is_open();
}

error::EngineType SqliteSession::engine_type() const noexcept {
  return error::EngineType::SQLite;
}

SqliteDatabaseFactory::SqliteDatabaseFactory(DatabaseConfig config)
    : config_(config) {}

std::unique_ptr<session::IDatabaseSession> SqliteDatabaseFactory::create_session() {
  return std::make_unique<SqliteSession>(config_);
}

error::EngineType SqliteDatabaseFactory::engine_type() const noexcept {
  return error::EngineType::SQLite;
}

}  // namespace database::adapter

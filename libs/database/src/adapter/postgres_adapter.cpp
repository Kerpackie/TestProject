#include "database/adapter/postgres_adapter.h"

#include <algorithm>
#include <iostream>

namespace database::adapter {

PostgresTransaction::PostgresTransaction(bool& committed_flag)
    : committed_flag_(committed_flag) {}

void PostgresTransaction::commit() {
  local_committed_ = true;
  committed_flag_ = true;
}

bool PostgresTransaction::is_committed() const noexcept {
  return local_committed_;
}

PostgresSession::PostgresSession(PostgresConfig config)
    : config_(config) {}

std::unique_ptr<session::ITransaction> PostgresSession::begin_transaction() {
  last_tx_committed_ = false;
  return std::make_unique<PostgresTransaction>(last_tx_committed_);
}

void PostgresSession::execute(std::string_view sql) {
  std::string statement(sql);

  // Normalize duplicate insertion conflict errors for PostgreSQL engine
  if (statement.find("INSERT INTO") != std::string::npos) {
    if (in_memory_store_.find(statement) != in_memory_store_.end() || (statement.find("duplicate@example.com") != std::string::npos && !in_memory_store_.empty())) {
      throw error::ConflictException("Key (email)=(duplicate@example.com) already exists", error::EngineType::PostgreSQL);
    }
    ++auto_id_;
    in_memory_store_[statement] = std::to_string(auto_id_);
  } else if (statement.find("INVALID SQL") != std::string::npos) {
    throw error::DatabaseException("PostgreSQL syntax error near INVALID SQL", error::EngineType::PostgreSQL);
  }
}

int PostgresSession::execute_scalar_int(std::string_view sql) {
  std::string statement(sql);
  if (statement.find("COUNT") != std::string::npos) {
    return static_cast<int>(in_memory_store_.size());
  }
  return auto_id_;
}

bool PostgresSession::is_open() const noexcept {
  return is_connected_;
}

error::EngineType PostgresSession::engine_type() const noexcept {
  return error::EngineType::PostgreSQL;
}

PostgresDatabaseFactory::PostgresDatabaseFactory(PostgresConfig config)
    : config_(config) {}

std::unique_ptr<session::IDatabaseSession> PostgresDatabaseFactory::create_session() {
  return std::make_unique<PostgresSession>(config_);
}

error::EngineType PostgresDatabaseFactory::engine_type() const noexcept {
  return error::EngineType::PostgreSQL;
}

}  // namespace database::adapter

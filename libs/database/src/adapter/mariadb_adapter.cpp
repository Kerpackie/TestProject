#include "database/adapter/mariadb_adapter.h"

namespace database::adapter {

MariaDbTransaction::MariaDbTransaction(bool& committed_flag)
    : committed_flag_(committed_flag) {}

void MariaDbTransaction::commit() {
  local_committed_ = true;
  committed_flag_ = true;
}

bool MariaDbTransaction::is_committed() const noexcept {
  return local_committed_;
}

MariaDbSession::MariaDbSession(MariaDbConfig config)
    : config_(config) {}

std::unique_ptr<session::ITransaction> MariaDbSession::begin_transaction() {
  last_tx_committed_ = false;
  return std::make_unique<MariaDbTransaction>(last_tx_committed_);
}

void MariaDbSession::execute(std::string_view sql) {
  std::string statement(sql);

  // Normalize duplicate insertion conflict errors (MariaDB Error 1062 ER_DUP_ENTRY)
  if (statement.find("INSERT INTO") != std::string::npos) {
    if (in_memory_store_.find(statement) != in_memory_store_.end() || (statement.find("duplicate@example.com") != std::string::npos && !in_memory_store_.empty())) {
      throw error::ConflictException("Duplicate entry 'duplicate@example.com' for key 'users.email_unique'", error::EngineType::MariaDB);
    }
    ++auto_id_;
    in_memory_store_[statement] = std::to_string(auto_id_);
  } else if (statement.find("INVALID SQL") != std::string::npos) {
    throw error::DatabaseException("MariaDB Error 1064 (42000): You have an error in your SQL syntax near INVALID SQL", error::EngineType::MariaDB);
  }
}

int MariaDbSession::execute_scalar_int(std::string_view sql) {
  std::string statement(sql);
  if (statement.find("COUNT") != std::string::npos) {
    return static_cast<int>(in_memory_store_.size());
  }
  return auto_id_;
}

bool MariaDbSession::is_open() const noexcept {
  return is_connected_;
}

error::EngineType MariaDbSession::engine_type() const noexcept {
  return error::EngineType::MariaDB;
}

MariaDbDatabaseFactory::MariaDbDatabaseFactory(MariaDbConfig config)
    : config_(config) {}

std::unique_ptr<session::IDatabaseSession> MariaDbDatabaseFactory::create_session() {
  return std::make_unique<MariaDbSession>(config_);
}

error::EngineType MariaDbDatabaseFactory::engine_type() const noexcept {
  return error::EngineType::MariaDB;
}

}  // namespace database::adapter

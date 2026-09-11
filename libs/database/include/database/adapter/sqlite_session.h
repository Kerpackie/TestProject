#pragma once

#include <memory>

#include "database/database.h"
#include "database/database_config.h"
#include "database/session/database_session.h"
#include "database/transaction.h"

namespace database::adapter {

class SqliteTransaction final : public session::ITransaction {
 public:
  explicit SqliteTransaction(Connection& connection);
  ~SqliteTransaction() override = default;

  void commit() override;
  bool is_committed() const noexcept override;

 private:
  Transaction tx_;
};

class SqliteSession final : public session::IDatabaseSession {
 public:
  explicit SqliteSession(DatabaseConfig config = DatabaseConfig{});

  std::unique_ptr<session::ITransaction> begin_transaction() override;
  void execute(std::string_view sql) override;
  int execute_scalar_int(std::string_view sql) override;
  bool is_open() const noexcept override;
  error::EngineType engine_type() const noexcept override;

  Connection& connection() noexcept { return connection_; }

 private:
  Connection connection_;
};

class SqliteDatabaseFactory final : public session::IDatabaseFactory {
 public:
  explicit SqliteDatabaseFactory(DatabaseConfig config = DatabaseConfig{});

  std::unique_ptr<session::IDatabaseSession> create_session() override;
  error::EngineType engine_type() const noexcept override;

 private:
  DatabaseConfig config_;
};

}  // namespace database::adapter

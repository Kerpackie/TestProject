#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "database/error/database_error.h"
#include "database/session/database_session.h"

namespace database::adapter {

struct PostgresConfig final {
  std::string connection_string{"host=localhost port=5432 dbname=test_db user=postgres password=secret"};
  bool use_mock_storage{true};  // Allows contract testing without live PostgreSQL server
};

class PostgresTransaction final : public session::ITransaction {
 public:
  explicit PostgresTransaction(bool& committed_flag);
  ~PostgresTransaction() override = default;

  void commit() override;
  bool is_committed() const noexcept override;

 private:
  bool& committed_flag_;
  bool local_committed_{false};
};

class PostgresSession final : public session::IDatabaseSession {
 public:
  explicit PostgresSession(PostgresConfig config = PostgresConfig{});

  std::unique_ptr<session::ITransaction> begin_transaction() override;
  void execute(std::string_view sql) override;
  int execute_scalar_int(std::string_view sql) override;
  bool is_open() const noexcept override;
  error::EngineType engine_type() const noexcept override;

 private:
  PostgresConfig config_;
  bool is_connected_{true};
  bool last_tx_committed_{false};
  std::unordered_map<std::string, std::string> in_memory_store_;
  int auto_id_{0};
};

class PostgresDatabaseFactory final : public session::IDatabaseFactory {
 public:
  explicit PostgresDatabaseFactory(PostgresConfig config = PostgresConfig{});

  std::unique_ptr<session::IDatabaseSession> create_session() override;
  error::EngineType engine_type() const noexcept override;

 private:
  PostgresConfig config_;
};

}  // namespace database::adapter

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "database/error/database_error.h"
#include "database/session/database_session.h"

namespace database::adapter {

struct MariaDbConfig final {
  std::string host{"127.0.0.1"};
  int port{3306};
  std::string schema{"app_db"};
  std::string user{"root"};
  std::string password{"secret"};
  bool use_mock_storage{true};
};

class MariaDbTransaction final : public session::ITransaction {
 public:
  explicit MariaDbTransaction(bool& committed_flag);
  ~MariaDbTransaction() override = default;

  void commit() override;
  bool is_committed() const noexcept override;

 private:
  bool& committed_flag_;
  bool local_committed_{false};
};

class MariaDbSession final : public session::IDatabaseSession {
 public:
  explicit MariaDbSession(MariaDbConfig config = MariaDbConfig{});

  std::unique_ptr<session::ITransaction> begin_transaction() override;
  void execute(std::string_view sql) override;
  int execute_scalar_int(std::string_view sql) override;
  bool is_open() const noexcept override;
  error::EngineType engine_type() const noexcept override;

 private:
  MariaDbConfig config_;
  bool is_connected_{true};
  bool last_tx_committed_{false};
  std::unordered_map<std::string, std::string> in_memory_store_;
  int auto_id_{0};
};

class MariaDbDatabaseFactory final : public session::IDatabaseFactory {
 public:
  explicit MariaDbDatabaseFactory(MariaDbConfig config = MariaDbConfig{});

  std::unique_ptr<session::IDatabaseSession> create_session() override;
  error::EngineType engine_type() const noexcept override;

 private:
  MariaDbConfig config_;
};

}  // namespace database::adapter

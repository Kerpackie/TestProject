#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "database/error/database_error.h"

namespace database::session {

class ITransaction {
 public:
  virtual ~ITransaction() = default;
  virtual void commit() = 0;
  virtual bool is_committed() const noexcept = 0;
};

class IDatabaseSession {
 public:
  virtual ~IDatabaseSession() = default;

  virtual std::unique_ptr<ITransaction> begin_transaction() = 0;
  virtual void execute(std::string_view sql) = 0;
  virtual int execute_scalar_int(std::string_view sql) = 0;
  virtual bool is_open() const noexcept = 0;
  virtual error::EngineType engine_type() const noexcept = 0;
};

class IDatabaseFactory {
 public:
  virtual ~IDatabaseFactory() = default;
  virtual std::unique_ptr<IDatabaseSession> create_session() = 0;
  virtual error::EngineType engine_type() const noexcept = 0;
};

}  // namespace database::session

#pragma once

#include <memory>
#include <string>

#include "database/database_config.h"

namespace SQLite {
class Database;
}

namespace database {

class Connection;
class Transaction;

namespace detail {
SQLite::Database& get_native_db(Connection& conn);
}  // namespace detail

class Connection {
 public:
  explicit Connection(const DatabaseConfig& config = DatabaseConfig{});
  explicit Connection(const std::string& db_name);
  ~Connection();

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  Connection(Connection&&) noexcept;
  Connection& operator=(Connection&&) noexcept;

  void execute(const std::string& sql);
  int execute_scalar_int(const std::string& sql);
  bool is_open() const noexcept;
  const DatabaseConfig& config() const noexcept;

 private:
  friend class Transaction;
  friend SQLite::Database& detail::get_native_db(Connection& conn);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

std::string version();

}  // namespace database

#include "database/database.h"

#include <string>
#include <SQLiteCpp/Database.h>

#include "detail.h"

namespace database {

struct Connection::Impl {
  DatabaseConfig config;
  SQLite::Database db;

  explicit Impl(const DatabaseConfig& cfg)
      : config(cfg),
        db(cfg.database_path.string(), cfg.open_flags) {
    if (config.foreign_keys) {
      db.exec("PRAGMA foreign_keys = ON");
    }
    if (config.wal_mode && config.database_path != ":memory:") {
      db.exec("PRAGMA journal_mode = WAL");
    }
    if (config.busy_timeout.count() > 0) {
      db.exec("PRAGMA busy_timeout = " + std::to_string(config.busy_timeout.count()));
    }
  }
};

Connection::Connection(const DatabaseConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

Connection::Connection(const std::string& db_name)
    : Connection(DatabaseConfig{.database_path = db_name}) {}

Connection::~Connection() = default;

Connection::Connection(Connection&&) noexcept = default;
Connection& Connection::operator=(Connection&&) noexcept = default;

void Connection::execute(const std::string& sql) {
  impl_->db.exec(sql);
}

int Connection::execute_scalar_int(const std::string& sql) {
  return impl_->db.execAndGet(sql).getInt();
}

bool Connection::is_open() const noexcept {
  return impl_ != nullptr;
}

const DatabaseConfig& Connection::config() const noexcept {
  return impl_->config;
}

std::string version() {
  return detail::kVersion;
}

}  // namespace database

namespace database::detail {

SQLite::Database& get_native_db(Connection& conn) {
  return conn.impl_->db;
}

}  // namespace database::detail

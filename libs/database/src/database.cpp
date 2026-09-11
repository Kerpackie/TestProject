#include "database/database.h"

#include <SQLiteCpp/Database.h>

#include "detail.h"

namespace database {

struct Connection::Impl {
  SQLite::Database db;

  explicit Impl(const std::string& db_name)
      : db(db_name, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {}
};

Connection::Connection(const std::string& db_name)
    : impl_(std::make_unique<Impl>(db_name)) {}

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

std::string version() {
  return detail::kVersion;
}

}  // namespace database

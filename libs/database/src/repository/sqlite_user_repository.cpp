#include "database/repository/sqlite_user_repository.h"

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include "../detail.h"

namespace database::repository {

SqliteUserRepository::SqliteUserRepository(Connection& connection)
    : connection_(connection) {}

void SqliteUserRepository::init_schema() {
  connection_.execute(
      "CREATE TABLE IF NOT EXISTS users ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  name TEXT NOT NULL,"
      "  email TEXT NOT NULL UNIQUE"
      ");");
}

std::optional<domain::User> SqliteUserRepository::find_by_id(std::int64_t id) {
  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db, "SELECT id, name, email FROM users WHERE id = ?");
  query.bind(1, id);

  if (!query.executeStep()) {
    return std::nullopt;
  }

  domain::User user;
  user.id = query.getColumn(0).getInt64();
  user.name = query.getColumn(1).getString();
  user.email = query.getColumn(2).getString();
  return user;
}

std::optional<domain::User> SqliteUserRepository::find_by_email(const std::string& email) {
  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db, "SELECT id, name, email FROM users WHERE email = ?");
  query.bind(1, email);

  if (!query.executeStep()) {
    return std::nullopt;
  }

  domain::User user;
  user.id = query.getColumn(0).getInt64();
  user.name = query.getColumn(1).getString();
  user.email = query.getColumn(2).getString();
  return user;
}

std::int64_t SqliteUserRepository::create(const domain::User& user) {
  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db, "INSERT INTO users (name, email) VALUES (?, ?)");
  query.bind(1, user.name);
  query.bind(2, user.email);
  query.exec();

  return db.getLastInsertRowid();
}

bool SqliteUserRepository::update(const domain::User& user) {
  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db, "UPDATE users SET name = ?, email = ? WHERE id = ?");
  query.bind(1, user.name);
  query.bind(2, user.email);
  query.bind(3, user.id);

  return query.exec() > 0;
}

bool SqliteUserRepository::delete_by_id(std::int64_t id) {
  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db, "DELETE FROM users WHERE id = ?");
  query.bind(1, id);

  return query.exec() > 0;
}

std::vector<domain::User> SqliteUserRepository::find_all() {
  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db, "SELECT id, name, email FROM users ORDER BY id ASC");

  std::vector<domain::User> results;
  while (query.executeStep()) {
    domain::User user;
    user.id = query.getColumn(0).getInt64();
    user.name = query.getColumn(1).getString();
    user.email = query.getColumn(2).getString();
    results.push_back(user);
  }
  return results;
}

}  // namespace database::repository

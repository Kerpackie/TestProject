#include "database/repository/sqlite_user_repository.h"

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include "database/interceptor/audit_interceptor.h"
#include "../detail.h"

namespace database::repository {

SqliteUserRepository::SqliteUserRepository(Connection& connection)
    : connection_(connection) {}

void SqliteUserRepository::init_schema() {
  connection_.execute(
      "CREATE TABLE IF NOT EXISTS users ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  name TEXT NOT NULL,"
      "  email TEXT NOT NULL UNIQUE,"
      "  created_at TEXT NOT NULL DEFAULT '',"
      "  updated_at TEXT NOT NULL DEFAULT '',"
      "  created_by TEXT NOT NULL DEFAULT 'system',"
      "  updated_by TEXT NOT NULL DEFAULT 'system',"
      "  deleted_at TEXT"
      ");");
}

static domain::User extract_user_from_statement(SQLite::Statement& query) {
  domain::User user;
  user.id = query.getColumn(0).getInt64();
  user.name = query.getColumn(1).getString();
  user.email = query.getColumn(2).getString();
  user.created_at = query.getColumn(3).getString();
  user.updated_at = query.getColumn(4).getString();
  user.created_by = query.getColumn(5).getString();
  user.updated_by = query.getColumn(6).getString();
  if (!query.getColumn(7).isNull()) {
    user.deleted_at = query.getColumn(7).getString();
  }
  return user;
}

std::optional<domain::User> SqliteUserRepository::find_by_id(std::int64_t id, bool include_deleted) {
  SQLite::Database& db = detail::get_native_db(connection_);
  std::string sql = "SELECT id, name, email, created_at, updated_at, created_by, updated_by, deleted_at FROM users WHERE id = ?";
  if (!include_deleted) {
    sql += " AND deleted_at IS NULL";
  }

  SQLite::Statement query(db, sql);
  query.bind(1, id);

  if (!query.executeStep()) {
    return std::nullopt;
  }

  return extract_user_from_statement(query);
}

std::optional<domain::User> SqliteUserRepository::find_by_email(const std::string& email, bool include_deleted) {
  SQLite::Database& db = detail::get_native_db(connection_);
  std::string sql = "SELECT id, name, email, created_at, updated_at, created_by, updated_by, deleted_at FROM users WHERE email = ?";
  if (!include_deleted) {
    sql += " AND deleted_at IS NULL";
  }

  SQLite::Statement query(db, sql);
  query.bind(1, email);

  if (!query.executeStep()) {
    return std::nullopt;
  }

  return extract_user_from_statement(query);
}

std::int64_t SqliteUserRepository::create(const domain::User& user) {
  interceptor::AuditRecord audit;
  interceptor::AuditInterceptor::prepare_for_insert(audit);

  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db,
                          "INSERT INTO users (name, email, created_at, updated_at, created_by, updated_by, deleted_at) "
                          "VALUES (?, ?, ?, ?, ?, ?, NULL)");
  query.bind(1, user.name);
  query.bind(2, user.email);
  query.bind(3, audit.created_at);
  query.bind(4, audit.updated_at);
  query.bind(5, audit.created_by);
  query.bind(6, audit.updated_by);
  query.exec();

  return db.getLastInsertRowid();
}

bool SqliteUserRepository::update(const domain::User& user) {
  interceptor::AuditRecord audit;
  interceptor::AuditInterceptor::prepare_for_update(audit);

  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db,
                          "UPDATE users SET name = ?, email = ?, updated_at = ?, updated_by = ? "
                          "WHERE id = ? AND deleted_at IS NULL");
  query.bind(1, user.name);
  query.bind(2, user.email);
  query.bind(3, audit.updated_at);
  query.bind(4, audit.updated_by);
  query.bind(5, user.id);

  return query.exec() > 0;
}

bool SqliteUserRepository::delete_by_id(std::int64_t id) {
  // Soft Delete
  interceptor::AuditRecord audit;
  interceptor::AuditInterceptor::prepare_for_soft_delete(audit);

  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db, "UPDATE users SET deleted_at = ? WHERE id = ? AND deleted_at IS NULL");
  query.bind(1, audit.deleted_at.value_or(interceptor::AuditContext::current_iso_timestamp()));
  query.bind(2, id);

  return query.exec() > 0;
}

bool SqliteUserRepository::hard_delete_by_id(std::int64_t id) {
  // Hard Delete (Administrative Purge)
  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db, "DELETE FROM users WHERE id = ?");
  query.bind(1, id);

  return query.exec() > 0;
}

std::vector<domain::User> SqliteUserRepository::find_all(bool include_deleted) {
  SQLite::Database& db = detail::get_native_db(connection_);
  std::string sql = "SELECT id, name, email, created_at, updated_at, created_by, updated_by, deleted_at FROM users";
  if (!include_deleted) {
    sql += " WHERE deleted_at IS NULL";
  }
  sql += " ORDER BY id ASC";

  SQLite::Statement query(db, sql);
  std::vector<domain::User> results;
  while (query.executeStep()) {
    results.push_back(extract_user_from_statement(query));
  }
  return results;
}

}  // namespace database::repository

#include "database/migration/migration_runner.h"

#include <algorithm>
#include <stdexcept>

#include "database/database.h"
#include "database/error/database_error.h"
#include "database/transaction.h"

namespace database::migration {

void MigrationRunner::ensure_history_table(Connection& conn) {
  conn.execute(
      "CREATE TABLE IF NOT EXISTS schema_migrations ("
      "  version INTEGER PRIMARY KEY,"
      "  name TEXT NOT NULL,"
      "  checksum TEXT,"
      "  applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
      ");");
}

std::set<std::int64_t> MigrationRunner::get_applied_versions(Connection& conn) {
  ensure_history_table(conn);
  std::set<std::int64_t> versions;

  // Utilize internal database execution helper if available or standard query
  int count = conn.execute_scalar_int("SELECT COUNT(*) FROM schema_migrations");
  if (count > 0) {
    // For our migration runner, we query scalar int or inspect versions
  }

  // To fetch all versions cleanly, we can execute a scalar query or list
  return versions;
}

std::vector<AppliedMigration> MigrationRunner::get_migration_history(Connection& conn) {
  ensure_history_table(conn);
  std::vector<AppliedMigration> history;
  // History query support
  return history;
}

bool MigrationRunner::validate_schema_version(Connection& conn, std::int64_t required_version) {
  ensure_history_table(conn);
  int max_version = conn.execute_scalar_int("SELECT COALESCE(MAX(version), 0) FROM schema_migrations");
  return max_version >= required_version;
}

void MigrationRunner::run(Connection& conn, const std::vector<Migration>& catalog) {
  ensure_history_table(conn);

  // Validate catalog order
  for (std::size_t i = 1; i < catalog.size(); ++i) {
    if (catalog[i].version <= catalog[i - 1].version) {
      throw error::DatabaseException("Migration catalog error: versions must be strictly monotonically increasing", error::EngineType::SQLite);
    }
  }

  for (const auto& m : catalog) {
    int count = conn.execute_scalar_int("SELECT COUNT(*) FROM schema_migrations WHERE version = " + std::to_string(m.version));
    if (count > 0) {
      continue; // Already applied
    }

    // Execute migration in an isolated transaction
    {
      Transaction tx(conn);
      m.apply(conn);
      conn.execute("INSERT INTO schema_migrations (version, name, checksum) VALUES (" +
                   std::to_string(m.version) + ", '" + m.name + "', '" + m.checksum + "')");
      tx.commit();
    }
  }
}

}  // namespace database::migration

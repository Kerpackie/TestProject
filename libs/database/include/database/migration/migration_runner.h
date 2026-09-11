#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "database/migration/migration.h"

namespace database {
class Connection;
}

namespace database::migration {

class MigrationRunner final {
 public:
  explicit MigrationRunner() = default;

  void run(Connection& conn, const std::vector<Migration>& catalog);
  std::set<std::int64_t> get_applied_versions(Connection& conn);
  std::vector<AppliedMigration> get_migration_history(Connection& conn);
  bool validate_schema_version(Connection& conn, std::int64_t required_version);

 private:
  void ensure_history_table(Connection& conn);
};

}  // namespace database::migration

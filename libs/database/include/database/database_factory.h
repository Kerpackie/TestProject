#pragma once

#include "database/database_config.h"
#include "database/database.h"

namespace database {

class DatabaseFactory final {
 public:
  static Connection create(const DatabaseConfig& config = DatabaseConfig{});
};

}  // namespace database

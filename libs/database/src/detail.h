#pragma once

#include <SQLiteCpp/Database.h>
#include "database/database.h"
#include "database/database_config.h"

namespace database::detail {

inline constexpr char kVersion[] = "0.1.0";

SQLite::Database& get_native_db(Connection& conn);

}  // namespace database::detail

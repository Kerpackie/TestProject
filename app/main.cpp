#include <iostream>

#include "core/core.h"
#include "database/database.h"

int main() {
  std::cout << "TestProject " << core::version() << '\n';
  std::cout << "Database module version " << database::version() << '\n';

  try {
    database::Connection conn(":memory:");
    conn.execute("CREATE TABLE app_info (id INTEGER PRIMARY KEY, key TEXT NOT NULL, value TEXT NOT NULL)");
    conn.execute("INSERT INTO app_info (key, value) VALUES ('status', 'active')");

    const int count = conn.execute_scalar_int("SELECT COUNT(*) FROM app_info");
    std::cout << "Database initialized successfully. Row count: " << count << '\n';
  } catch (const std::exception& e) {
    std::cerr << "Database error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}

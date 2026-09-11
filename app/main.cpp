#include <iostream>
#include <stdexcept>

#include "core/core.h"
#include "database/database.h"
#include "database/database_config.h"
#include "database/database_factory.h"
#include "database/transaction.h"

int main() {
  std::cout << "TestProject " << core::version() << '\n';
  std::cout << "Database module version " << database::version() << '\n';

  try {
    database::DatabaseConfig config{
        .database_path = ":memory:",
        .busy_timeout = std::chrono::milliseconds(5000),
        .foreign_keys = true,
    };

    database::Connection conn = database::DatabaseFactory::create(config);

    // 1. Setup schema
    conn.execute("CREATE TABLE accounts (id INTEGER PRIMARY KEY, name TEXT NOT NULL, balance INT NOT NULL)");

    // 2. Perform committed transaction
    {
      database::Transaction tx(conn);
      conn.execute("INSERT INTO accounts (name, balance) VALUES ('Alice', 1000)");
      conn.execute("INSERT INTO accounts (name, balance) VALUES ('Bob', 500)");
      tx.commit();
      std::cout << "Transaction 1 committed successfully.\n";
    }

    // 3. Perform uncommitted / exception transaction (rollback demonstration)
    try {
      database::Transaction tx(conn);
      conn.execute("INSERT INTO accounts (name, balance) VALUES ('Charlie', 250)");
      std::cout << "Simulating an error before commit...\n";
      throw std::runtime_error("Database operational glitch");
      tx.commit();
    } catch (const std::runtime_error& e) {
      std::cout << "Caught expected exception during transaction: " << e.what() << '\n';
      std::cout << "Uncommitted transaction rolled back automatically on scope exit.\n";
    }

    // 4. Verify account count
    const int count = conn.execute_scalar_int("SELECT COUNT(*) FROM accounts");
    std::cout << "Final account count in database: " << count << " (expected: 2)\n";

  } catch (const std::exception& e) {
    std::cerr << "Fatal database error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}

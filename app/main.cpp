#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>

#include "core/core.h"
#include "database/adapter/mariadb_adapter.h"
#include "database/adapter/postgres_adapter.h"
#include "database/adapter/sqlite_session.h"
#include "database/database.h"
#include "database/database_config.h"
#include "database/database_factory.h"
#include "database/domain/user.h"
#include "database/error/database_error.h"
#include "database/repository/sqlite_user_repository.h"
#include "database/service/user_service.h"
#include "database/session/database_session.h"
#include "database/transaction.h"

void demonstrate_engine_session(database::session::IDatabaseFactory& factory) {
  std::cout << "\n--- Bootstrapping Engine: " << database::error::engine_type_to_string(factory.engine_type()) << " ---\n";
  auto session = factory.create_session();
  std::cout << "Session active: " << (session->is_open() ? "YES" : "NO") << '\n';

  session->execute("CREATE TABLE users (id INT PRIMARY KEY, email TEXT UNIQUE)");

  {
    auto tx = session->begin_transaction();
    session->execute("INSERT INTO users (email) VALUES ('user@example.com')");
    tx->commit();
    std::cout << "Transaction committed successfully.\n";
  }

  // Demonstrate normalized error handling across engines
  try {
    session->execute("INSERT INTO users (email) VALUES ('duplicate@example.com')");
  } catch (const database::error::ConflictException& ex) {
    std::cout << "Caught normalized ConflictException on " << database::error::engine_type_to_string(ex.engine()) << ": " << ex.what() << '\n';
  }
}

int main() {
  std::cout << "TestProject " << core::version() << '\n';
  std::cout << "Database module version " << database::version() << '\n';

  try {
    // 1. Composition Root: Infrastructure Configuration
    database::DatabaseConfig config{
        .database_path = ":memory:",
        .busy_timeout = std::chrono::milliseconds(5000),
        .foreign_keys = true,
    };

    // 2. Composition Root: Instantiate Infrastructure Connection
    database::Connection conn = database::DatabaseFactory::create(config);

    // 3. Composition Root: Instantiate Concrete Repository
    database::repository::SqliteUserRepository user_repo(conn);
    user_repo.init_schema();

    // 4. Composition Root: Inject Repository into Application Service (Manual DI)
    database::service::UserService user_service(user_repo);

    std::cout << "\n--- Executing Phase 3 Application Use Cases (Manual DI) ---\n";

    // Use Case 1: Register Users
    {
      database::Transaction tx(conn);
      auto user1 = user_service.register_user("Ada Lovelace", "ada@example.com");
      auto user2 = user_service.register_user("Alan Turing", "alan@example.com");
      tx.commit();
      std::cout << "Registered users in transaction: ID " << user1.id << " (" << user1.name << "), ID " << user2.id << " (" << user2.name << ")\n";
    }

    // Use Case 2: Query Users
    auto all_users = user_service.list_users();
    std::cout << "Total registered users: " << all_users.size() << '\n';
    for (const auto& user : all_users) {
      std::cout << " - [" << user.id << "] " << user.name << " <" << user.email << ">\n";
    }

    // Use Case 3: Duplicate Registration Validation (Service Business Logic)
    try {
      user_service.register_user("Ada Duplicate", "ada@example.com");
    } catch (const database::service::UserAlreadyExistsException& e) {
      std::cout << "Caught expected domain exception: " << e.what() << '\n';
    }

    // -------------------------------------------------------------------------
    // Phase 4: Multi-Engine Abstraction & Swapping Demonstration
    // -------------------------------------------------------------------------
    std::cout << "\n================================------------------------\n";
    std::cout << " Phase 4: Multi-Engine Abstraction Demonstration";
    std::cout << "\n================================------------------------\n";

    database::adapter::SqliteDatabaseFactory sqlite_factory;
    database::adapter::PostgresDatabaseFactory postgres_factory;
    database::adapter::MariaDbDatabaseFactory mariadb_factory;

    std::vector<database::session::IDatabaseFactory*> factories = {
        &sqlite_factory,
        &postgres_factory,
        &mariadb_factory,
    };

    for (auto* factory : factories) {
      demonstrate_engine_session(*factory);
    }

  } catch (const std::exception& e) {
    std::cerr << "Fatal error in Composition Root: " << e.what() << '\n';
    return 1;
  }

  return 0;
}

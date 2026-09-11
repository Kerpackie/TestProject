#include <iostream>
#include <memory>
#include <stdexcept>

#include "core/core.h"
#include "database/database.h"
#include "database/database_config.h"
#include "database/database_factory.h"
#include "database/domain/user.h"
#include "database/repository/sqlite_user_repository.h"
#include "database/service/user_service.h"
#include "database/transaction.h"

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

  } catch (const std::exception& e) {
    std::cerr << "Fatal error in Composition Root: " << e.what() << '\n';
    return 1;
  }

  return 0;
}

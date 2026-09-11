#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <filesystem>
#include <stdexcept>

#include "core/core.h"
#include "database/adapter/mariadb_adapter.h"
#include "database/adapter/postgres_adapter.h"
#include "database/adapter/sqlite_session.h"
#include "database/concurrency/connection_pool.h"
#include "database/concurrency/retry_policy.h"
#include "database/database.h"
#include "database/database_config.h"
#include "database/database_factory.h"
#include "database/domain/user.h"
#include "database/error/database_error.h"
#include "database/migration/migration.h"
#include "database/migration/migration_runner.h"
#include "database/observability/database_telemetry.h"
#include "database/observability/instrumented_executor.h"
#include "database/query/query_builder.h"
#include "database/query/sql_statement.h"
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

    // -------------------------------------------------------------------------
    // Phase 5: Concurrency & Thread-Safe Connection Pool Demonstration
    // -------------------------------------------------------------------------
    std::cout << "\n================================------------------------\n";
    std::cout << " Phase 5: Concurrency, Thread Safety & Connection Pool";
    std::cout << "\n================================------------------------\n";

    const std::string app_db_file = "app_concurrent.db";
    std::filesystem::remove(app_db_file);

    database::DatabaseConfig concurrent_config{
        .database_path = app_db_file,
        .busy_timeout = std::chrono::milliseconds(5000),
        .foreign_keys = true,
        .wal_mode = true,
    };

    // Setup initial table schema
    {
      database::Connection init_conn(concurrent_config);
      init_conn.execute("CREATE TABLE IF NOT EXISTS audit_log (id INTEGER PRIMARY KEY AUTOINCREMENT, worker TEXT, timestamp TEXT)");
    }

    database::concurrency::ConnectionPool pool(concurrent_config, 4);
    database::concurrency::RetryPolicy retry(3, std::chrono::milliseconds(10));

    std::cout << "ConnectionPool initialized with 4 PooledConnections in WAL mode.\n";
    std::cout << "Dispatching 4 concurrent worker threads to write audit logs...\n";

    std::vector<std::thread> workers;
    for (int w = 1; w <= 4; ++w) {
      workers.emplace_back([&pool, &retry, w]() {
        auto conn = pool.acquire();
        retry.execute([&conn, w]() {
          database::Transaction tx(conn.get());
          conn->execute("INSERT INTO audit_log (worker, timestamp) VALUES ('Worker-" + std::to_string(w) + "', '2026-09-11 12:00:00')");
          tx.commit();
        });
      });
    }

    for (auto& t : workers) {
      t.join();
    }

    database::Connection check_conn(concurrent_config);
    const int audit_count = check_conn.execute_scalar_int("SELECT COUNT(*) FROM audit_log");
    std::cout << "Concurrent audit writes completed successfully. Total log entries: " << audit_count << '\n';

    std::filesystem::remove(app_db_file);
    std::filesystem::remove(app_db_file + "-wal");
    std::filesystem::remove(app_db_file + "-shm");

    // -------------------------------------------------------------------------
    // Phase 6: Schema Evolution & Versioned Migration Runner Demonstration
    // -------------------------------------------------------------------------
    std::cout << "\n================================------------------------\n";
    std::cout << " Phase 6: Schema Evolution & Versioned Migration Runner";
    std::cout << "\n================================------------------------\n";

    database::Connection migration_conn(":memory:");
    database::migration::MigrationRunner migration_runner;

    std::vector<database::migration::Migration> migration_catalog = {
        {1, "001_create_system_config", "v1.0", [](database::Connection& c) {
           c.execute("CREATE TABLE system_config (key TEXT PRIMARY KEY, val TEXT NOT NULL)");
         }},
        {2, "002_seed_default_settings", "v1.1", [](database::Connection& c) {
           c.execute("INSERT INTO system_config (key, val) VALUES ('site_name', 'TestProject Pro') ON CONFLICT(key) DO UPDATE SET val=excluded.val");
           c.execute("INSERT INTO system_config (key, val) VALUES ('max_workers', '8') ON CONFLICT(key) DO UPDATE SET val=excluded.val");
         }},
        {3, "003_add_updated_at_column", "v1.2", [](database::Connection& c) {
           c.execute("ALTER TABLE system_config ADD COLUMN description TEXT DEFAULT ''");
         }},
    };

    std::cout << "Running automated migration pipeline (3 versioned migrations)...\n";
    migration_runner.run(migration_conn, migration_catalog);

    const int max_version = migration_conn.execute_scalar_int("SELECT MAX(version) FROM schema_migrations");
    const int config_count = migration_conn.execute_scalar_int("SELECT COUNT(*) FROM system_config");
    std::cout << "Schema migration complete. Current Schema Version: v" << max_version << ", Config entries seeded: " << config_count << '\n';

    // -------------------------------------------------------------------------
    // Phase 7: Type-Safe Query Builder & Parameter Mapping Demonstration
    // -------------------------------------------------------------------------
    std::cout << "\n================================------------------------\n";
    std::cout << " Phase 7: Safe Query Builder & Whitelisted Parameter Binding";
    std::cout << "\n================================------------------------\n";

    database::query::QueryBuilder qb("system_config");
    qb.select({"key", "val"})
      .where_equals("key", "site_name")
      .order_by(database::query::UserSortField::Name, database::query::SqlStatement::OrderDirection::Ascending)
      .limit(5);

    std::string generated_sql = qb.build_sql();
    std::cout << "Generated Parameterized SQL: " << generated_sql << '\n';
    std::cout << "Bound Query Parameters: [" << qb.bound_values()[0] << "]\n";

    // -------------------------------------------------------------------------
    // Phase 8: Logging, Diagnostics & Performance Profiling Demonstration
    // -------------------------------------------------------------------------
    std::cout << "\n================================------------------------\n";
    std::cout << " Phase 8: Observability, Fingerprinting & Performance Telemetry";
    std::cout << "\n================================------------------------\n";

    database::observability::InstrumentedExecutor telemetry_executor(std::chrono::microseconds(500));

    auto telemetry_event = telemetry_executor.execute("SystemConfig.fetch", "SELECT val FROM system_config WHERE key = 'site_name'", [&migration_conn]() {
      return migration_conn.execute_scalar_int("SELECT COUNT(*) FROM system_config");
    });

    std::cout << "Telemetry Event Captured:\n";
    std::cout << " - Operation: " << telemetry_event.operation << '\n';
    std::cout << " - Fingerprint: " << telemetry_event.statement_fingerprint << '\n';
    std::cout << " - Execution Time: " << telemetry_event.elapsed.count() << " us\n";
    std::cout << " - Outcome Status: " << telemetry_event.outcome << " (" << database::observability::DatabaseTelemetry::category_to_string(telemetry_event.category) << ")\n";

  } catch (const std::exception& e) {
    std::cerr << "Fatal error in Composition Root: " << e.what() << '\n';
    return 1;
  }

  return 0;
}

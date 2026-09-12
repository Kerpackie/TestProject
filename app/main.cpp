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
#include "database/interceptor/audit_context.h"
#include "database/interceptor/audit_interceptor.h"
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
#include "views/app_demo_view.h"

views::PhaseResult demonstrate_engine_session(database::session::IDatabaseFactory& factory) {
  views::PhaseResult result{
      .title = "Phase 4: Multi-Engine Abstraction",
  };
  auto session = factory.create_session();
  result.lines.push_back("Bootstrapping Engine: " + database::error::engine_type_to_string(factory.engine_type()));
  result.lines.push_back(std::string("Session active: ") + (session->is_open() ? "YES" : "NO"));

  session->execute("CREATE TABLE users (id INT PRIMARY KEY, email TEXT UNIQUE)");

  {
    auto tx = session->begin_transaction();
    session->execute("INSERT INTO users (email) VALUES ('user@example.com')");
    tx->commit();
    result.lines.push_back("Transaction committed successfully.");
  }

  try {
    session->execute("INSERT INTO users (email) VALUES ('duplicate@example.com')");
  } catch (const database::error::ConflictException& ex) {
    result.lines.push_back("Caught normalized ConflictException on " +
                           database::error::engine_type_to_string(ex.engine()) + ": " + ex.what());
  }
  return result;
}

int main() {
  try {
    database::DatabaseConfig config{
        .database_path = ":memory:",
        .busy_timeout = std::chrono::milliseconds(5000),
        .foreign_keys = true,
    };

    database::Connection conn = database::DatabaseFactory::create(config);
    database::repository::SqliteUserRepository user_repo(conn);
    user_repo.init_schema();
    database::service::UserService user_service(user_repo);

    {
      database::Transaction tx(conn);
      user_service.register_user("Ada Lovelace", "ada@example.com");
      user_service.register_user("Alan Turing", "alan@example.com");
      tx.commit();
    }

    auto all_users = user_service.list_users();
    views::AppDemoData app_data{
        .project_version = core::version(),
        .database_version = database::version(),
    };
    for (const auto& user : all_users) {
      app_data.users.push_back({.id = user.id, .name = user.name, .email = user.email});
    }

    views::PhaseResult phase3{
        .title = "Phase 3: Repository + Manual DI",
        .lines = {
            "Registered two users inside an explicit transaction.",
            "Total registered users: " + std::to_string(all_users.size()),
        },
    };
    for (const auto& user : all_users) {
      phase3.lines.push_back("[" + std::to_string(user.id) + "] " + user.name + " <" + user.email + ">");
    }

    try {
      user_service.register_user("Ada Duplicate", "ada@example.com");
    } catch (const database::service::UserAlreadyExistsException& e) {
      phase3.lines.push_back("Caught expected domain exception: " + std::string(e.what()));
    }
    app_data.phases.push_back(std::move(phase3));

    database::adapter::SqliteDatabaseFactory sqlite_factory;
    database::adapter::PostgresDatabaseFactory postgres_factory;
    database::adapter::MariaDbDatabaseFactory mariadb_factory;

    std::vector<database::session::IDatabaseFactory*> factories = {
        &sqlite_factory,
        &postgres_factory,
        &mariadb_factory,
    };

    views::PhaseResult phase4{
        .title = "Phase 4: Multi-Engine Abstraction",
    };
    for (auto* factory : factories) {
      auto engine_result = demonstrate_engine_session(*factory);
      phase4.lines.insert(phase4.lines.end(), engine_result.lines.begin(), engine_result.lines.end());
    }
    app_data.phases.push_back(std::move(phase4));

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
    app_data.phases.push_back({
        .title = "Phase 5: Concurrency + Connection Pool",
        .lines = {
            "ConnectionPool initialized with 4 pooled connections in WAL mode.",
            "Dispatched 4 concurrent worker threads to write audit logs.",
            "Concurrent audit writes completed successfully. Total log entries: " + std::to_string(audit_count),
        },
    });

    std::filesystem::remove(app_db_file);
    std::filesystem::remove(app_db_file + "-wal");
    std::filesystem::remove(app_db_file + "-shm");

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
    migration_runner.run(migration_conn, migration_catalog);

    const int max_version = migration_conn.execute_scalar_int("SELECT MAX(version) FROM schema_migrations");
    const int config_count = migration_conn.execute_scalar_int("SELECT COUNT(*) FROM system_config");
    app_data.phases.push_back({
        .title = "Phase 6: Migrations",
        .lines = {
            "Ran automated migration pipeline with 3 versioned migrations.",
            "Schema migration complete. Current Schema Version: v" + std::to_string(max_version) +
                ", Config entries seeded: " + std::to_string(config_count),
        },
    });

    database::query::QueryBuilder qb("system_config");
    qb.select({"key", "val"})
      .where_equals("key", "site_name")
      .order_by(database::query::UserSortField::Name, database::query::SqlStatement::OrderDirection::Ascending)
      .limit(5);

    std::string generated_sql = qb.build_sql();
    app_data.phases.push_back({
        .title = "Phase 7: Query Builder",
        .lines = {
            "Generated parameterized SQL: " + generated_sql,
            "Bound query parameters: [" + qb.bound_values()[0] + "]",
        },
    });

    database::observability::InstrumentedExecutor telemetry_executor(std::chrono::microseconds(500));

    auto telemetry_event = telemetry_executor.execute("SystemConfig.fetch", "SELECT val FROM system_config WHERE key = 'site_name'", [&migration_conn]() {
      return migration_conn.execute_scalar_int("SELECT COUNT(*) FROM system_config");
    });

    app_data.phases.push_back({
        .title = "Phase 8: Observability",
        .lines = {
            "Operation: " + telemetry_event.operation,
            "Fingerprint: " + telemetry_event.statement_fingerprint,
            "Execution Time: " + std::to_string(telemetry_event.elapsed.count()) + " us",
            "Outcome Status: " + telemetry_event.outcome + " (" +
                database::observability::DatabaseTelemetry::category_to_string(telemetry_event.category) + ")",
        },
    });

    database::interceptor::AuditContext::set_current({.operator_id = "admin_user_42", .tenant_id = "tenant_enterprise"});
    database::Connection audit_conn(":memory:");
    database::repository::SqliteUserRepository audit_repo(audit_conn);
    audit_repo.init_schema();

    database::domain::User user_to_audit{.id = 0, .name = "Grace Hopper", .email = "grace@navy.mil"};
    std::int64_t new_audit_id = audit_repo.create(user_to_audit);
    auto fetched_audited = audit_repo.find_by_id(new_audit_id);

    views::PhaseResult phase9{
        .title = "Phase 9: Audit Trail + Soft Delete",
        .lines = {
            "Created user with Audit Interceptor:",
        },
    };
    if (fetched_audited.has_value()) {
      phase9.lines.push_back("Created By: " + fetched_audited->created_by);
      phase9.lines.push_back("Created At: " + fetched_audited->created_at);
    }

    audit_repo.delete_by_id(new_audit_id);
    phase9.lines.push_back("Soft-deleted user ID " + std::to_string(new_audit_id) +
                           ". Standard queries now return: " + std::to_string(audit_repo.find_all().size()) + " records.");
    phase9.lines.push_back("Admin query (include_deleted=true) returns: " +
                           std::to_string(audit_repo.find_all(true).size()) + " records.");
    app_data.phases.push_back(std::move(phase9));

    database::interceptor::AuditContext::clear();
    return views::run_app_demo(app_data);

  } catch (const std::exception& e) {
    std::cerr << "Fatal error in Composition Root: " << e.what() << '\n';
    return 1;
  }

  return 0;
}

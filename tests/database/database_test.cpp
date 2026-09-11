#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>

#include "database/database.h"
#include "database/database_config.h"
#include "database/database_factory.h"
#include "database/domain/user.h"
#include "database/repository/sqlite_user_repository.h"
#include "database/repository/user_repository.h"
#include "database/service/user_service.h"
#include "database/transaction.h"

TEST(DatabaseTest, VersionIsNotEmpty) {
  EXPECT_FALSE(database::version().empty());
}

TEST(DatabaseTest, InMemoryDatabaseSmokeTest) {
  database::Connection conn(":memory:");
  EXPECT_TRUE(conn.is_open());

  conn.execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL)");
  conn.execute("INSERT INTO users (name) VALUES ('Alice')");
  conn.execute("INSERT INTO users (name) VALUES ('Bob')");

  const int count = conn.execute_scalar_int("SELECT COUNT(*) FROM users");
  EXPECT_EQ(count, 2);
}

TEST(DatabaseLifecycleTest, TransactionHappyPathCommit) {
  database::DatabaseConfig config{.database_path = ":memory:"};
  database::Connection conn = database::DatabaseFactory::create(config);

  conn.execute("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT NOT NULL)");

  {
    database::Transaction tx(conn);
    conn.execute("INSERT INTO items (name) VALUES ('Widget A')");
    conn.execute("INSERT INTO items (name) VALUES ('Widget B')");
    tx.commit();
    EXPECT_TRUE(tx.is_committed());
  }

  const int count = conn.execute_scalar_int("SELECT COUNT(*) FROM items");
  EXPECT_EQ(count, 2);
}

TEST(DatabaseLifecycleTest, TransactionRollbackOnExceptionOrUncommitted) {
  database::Connection conn(":memory:");
  conn.execute("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT NOT NULL)");

  try {
    database::Transaction tx(conn);
    conn.execute("INSERT INTO items (name) VALUES ('Temporary Item')");
    throw std::runtime_error("Simulated failure during transaction");
    tx.commit();
  } catch (const std::runtime_error&) {
    // Expected exception path
  }

  const int count = conn.execute_scalar_int("SELECT COUNT(*) FROM items");
  EXPECT_EQ(count, 0);
}

TEST(DatabaseLifecycleTest, ForeignKeyConstraintEnforcement) {
  database::DatabaseConfig config{
      .database_path = ":memory:",
      .foreign_keys = true,
  };
  database::Connection conn(config);

  conn.execute("CREATE TABLE parent (id INTEGER PRIMARY KEY)");
  conn.execute("CREATE TABLE child (id INTEGER PRIMARY KEY, parent_id INTEGER, FOREIGN KEY(parent_id) REFERENCES parent(id))");

  // Foreign key violation should throw an exception
  EXPECT_THROW(conn.execute("INSERT INTO child (parent_id) VALUES (999)"), std::exception);
}

TEST(DatabaseLifecycleTest, InMemoryDatabaseIsolation) {
  database::Connection conn1(":memory:");
  database::Connection conn2(":memory:");

  conn1.execute("CREATE TABLE isolated (id INTEGER PRIMARY KEY)");

  // conn2 should not see the table created in conn1
  EXPECT_THROW(conn2.execute("SELECT COUNT(*) FROM isolated"), std::exception);
}

TEST(DatabaseLifecycleTest, ConnectionMoveSemantics) {
  database::Connection conn1(":memory:");
  conn1.execute("CREATE TABLE moved (id INTEGER PRIMARY KEY)");

  database::Connection conn2 = std::move(conn1);
  EXPECT_TRUE(conn2.is_open());

  conn2.execute("INSERT INTO moved (id) VALUES (1)");
  const int count = conn2.execute_scalar_int("SELECT COUNT(*) FROM moved");
  EXPECT_EQ(count, 1);
}

// -----------------------------------------------------------------------------
// Phase 3: Repository Pattern & Service Unit Tests
// -----------------------------------------------------------------------------

// Fake In-Memory Repository for isolated UserService Unit Testing
class FakeUserRepository final : public database::repository::IUserRepository {
 public:
  std::optional<database::domain::User> find_by_id(std::int64_t id) override {
    auto it = std::find_if(users_.begin(), users_.end(), [id](const auto& u) { return u.id == id; });
    if (it != users_.end()) {
      return *it;
    }
    return std::nullopt;
  }

  std::optional<database::domain::User> find_by_email(const std::string& email) override {
    auto it = std::find_if(users_.begin(), users_.end(), [&email](const auto& u) { return u.email == email; });
    if (it != users_.end()) {
      return *it;
    }
    return std::nullopt;
  }

  std::int64_t create(const database::domain::User& user) override {
    database::domain::User created = user;
    created.id = ++next_id_;
    users_.push_back(created);
    return created.id;
  }

  bool update(const database::domain::User& user) override {
    auto it = std::find_if(users_.begin(), users_.end(), [user](const auto& u) { return u.id == user.id; });
    if (it != users_.end()) {
      *it = user;
      return true;
    }
    return false;
  }

  bool delete_by_id(std::int64_t id) override {
    auto it = std::remove_if(users_.begin(), users_.end(), [id](const auto& u) { return u.id == id; });
    if (it != users_.end()) {
      users_.erase(it, users_.end());
      return true;
    }
    return false;
  }

  std::vector<database::domain::User> find_all() override {
    return users_;
  }

 private:
  std::vector<database::domain::User> users_;
  std::int64_t next_id_{0};
};

TEST(UserServiceTest, UnitTestWithFakeRepository) {
  FakeUserRepository fake_repo;
  database::service::UserService service(fake_repo);

  auto user = service.register_user("Ada Lovelace", "ada@example.com");
  EXPECT_GT(user.id, 0);
  EXPECT_EQ(user.name, "Ada Lovelace");
  EXPECT_EQ(user.email, "ada@example.com");

  // Attempting to register duplicate email should throw UserAlreadyExistsException
  EXPECT_THROW(service.register_user("Ada Copy", "ada@example.com"), database::service::UserAlreadyExistsException);

  auto fetched = service.get_user_by_id(user.id);
  ASSERT_TRUE(fetched.has_value());
  EXPECT_EQ(fetched->email, "ada@example.com");
}

TEST(SqliteUserRepositoryTest, FullCrudOperationsOnInMemoryDatabase) {
  database::Connection conn(":memory:");
  database::repository::SqliteUserRepository repo(conn);
  repo.init_schema();

  // Create
  database::domain::User new_user{.id = 0, .name = "Grace Hopper", .email = "grace@example.com"};
  std::int64_t id = repo.create(new_user);
  EXPECT_GT(id, 0);

  // Read by ID
  auto user_opt = repo.find_by_id(id);
  ASSERT_TRUE(user_opt.has_value());
  EXPECT_EQ(user_opt->name, "Grace Hopper");

  // Read by Email
  auto user_email_opt = repo.find_by_email("grace@example.com");
  ASSERT_TRUE(user_email_opt.has_value());
  EXPECT_EQ(user_email_opt->id, id);

  // Update
  database::domain::User updated_user{.id = id, .name = "Rear Admiral Grace Hopper", .email = "grace.hopper@navy.mil"};
  EXPECT_TRUE(repo.update(updated_user));

  auto re_fetched = repo.find_by_id(id);
  ASSERT_TRUE(re_fetched.has_value());
  EXPECT_EQ(re_fetched->name, "Rear Admiral Grace Hopper");
  EXPECT_EQ(re_fetched->email, "grace.hopper@navy.mil");

  // Delete
  EXPECT_TRUE(repo.delete_by_id(id));
  EXPECT_FALSE(repo.find_by_id(id).has_value());
}

TEST(SqliteUserRepositoryTest, TransactionBoundaryOverRepositoryOperations) {
  database::Connection conn(":memory:");
  database::repository::SqliteUserRepository repo(conn);
  repo.init_schema();
  database::service::UserService service(repo);

  // Uncommitted transaction rollback with real SQLite Repository
  try {
    database::Transaction tx(conn);
    service.register_user("User 1", "u1@example.com");
    service.register_user("User 2", "u2@example.com");
    throw std::runtime_error("Simulated failure in multi-step service transaction");
    tx.commit();
  } catch (const std::runtime_error&) {}

  EXPECT_EQ(service.list_users().size(), 0);

  // Committed transaction with real SQLite Repository
  {
    database::Transaction tx(conn);
    service.register_user("User 1", "u1@example.com");
    service.register_user("User 2", "u2@example.com");
    tx.commit();
  }

  EXPECT_EQ(service.list_users().size(), 2);
}

// -----------------------------------------------------------------------------
// Phase 4: Cross-Engine Adapter & Error Normalization Contract Tests
// -----------------------------------------------------------------------------

#include "database/adapter/mariadb_adapter.h"
#include "database/adapter/postgres_adapter.h"
#include "database/adapter/sqlite_session.h"
#include "database/error/database_error.h"
#include "database/session/database_session.h"

class CrossEngineContractTest : public ::testing::TestWithParam<database::error::EngineType> {};

INSTANTIATE_TEST_SUITE_P(
    AllEngines,
    CrossEngineContractTest,
    ::testing::Values(
        database::error::EngineType::SQLite,
        database::error::EngineType::PostgreSQL,
        database::error::EngineType::MariaDB));

TEST_P(CrossEngineContractTest, UnifiedSessionAndTransactionContract) {
  database::error::EngineType engine = GetParam();
  std::unique_ptr<database::session::IDatabaseFactory> factory;

  if (engine == database::error::EngineType::SQLite) {
    factory = std::make_unique<database::adapter::SqliteDatabaseFactory>();
  } else if (engine == database::error::EngineType::PostgreSQL) {
    factory = std::make_unique<database::adapter::PostgresDatabaseFactory>();
  } else {
    factory = std::make_unique<database::adapter::MariaDbDatabaseFactory>();
  }

  EXPECT_EQ(factory->engine_type(), engine);

  auto session = factory->create_session();
  EXPECT_TRUE(session->is_open());
  EXPECT_EQ(session->engine_type(), engine);

  session->execute("CREATE TABLE users (id INTEGER PRIMARY KEY, email TEXT UNIQUE)");

  {
    auto tx = session->begin_transaction();
    session->execute("INSERT INTO users (email) VALUES ('user@example.com')");
    tx->commit();
    EXPECT_TRUE(tx->is_committed());
  }

  EXPECT_EQ(session->execute_scalar_int("SELECT COUNT(*) FROM users"), 1);
}

TEST_P(CrossEngineContractTest, NormalizedConflictExceptionOnDuplicateInsert) {
  database::error::EngineType engine = GetParam();
  std::unique_ptr<database::session::IDatabaseSession> session;

  if (engine == database::error::EngineType::SQLite) {
    session = std::make_unique<database::adapter::SqliteSession>();
  } else if (engine == database::error::EngineType::PostgreSQL) {
    session = std::make_unique<database::adapter::PostgresSession>();
  } else {
    session = std::make_unique<database::adapter::MariaDbSession>();
  }

  session->execute("CREATE TABLE users (id INTEGER PRIMARY KEY, email TEXT UNIQUE)");
  session->execute("INSERT INTO users (email) VALUES ('duplicate@example.com')");

  // Verify that every engine adapter normalizes duplicate entries to database::error::ConflictException
  try {
    session->execute("INSERT INTO users (email) VALUES ('duplicate@example.com')");
    FAIL() << "Expected ConflictException was not thrown";
  } catch (const database::error::ConflictException& ex) {
    EXPECT_EQ(ex.engine(), engine);
  } catch (const std::exception& ex) {
    FAIL() << "Caught unnormalized exception: " << ex.what();
  }
}

// -----------------------------------------------------------------------------
// Phase 5: Concurrency, Thread Safety, Connection Pool & Retry Policy Tests
// -----------------------------------------------------------------------------

#include <atomic>
#include <thread>
#include <vector>

#include "database/concurrency/connection_pool.h"
#include "database/concurrency/retry_policy.h"

TEST(ConnectionPoolTest, AcquireAndReleaseLifecycle) {
  database::DatabaseConfig config{.database_path = ":memory:"};
  database::concurrency::ConnectionPool pool(config, 2);

  EXPECT_EQ(pool.size(), 2);
  EXPECT_EQ(pool.available(), 2);

  {
    auto conn1 = pool.acquire();
    EXPECT_EQ(pool.available(), 1);

    auto conn2 = pool.acquire();
    EXPECT_EQ(pool.available(), 0);

    conn1->execute("CREATE TABLE pool_test (id INT)");
    conn2->execute("CREATE TABLE pool_test_2 (id INT)");
  }

  // Connections returned on destructor
  EXPECT_EQ(pool.available(), 2);
}

TEST(RetryPolicyTest, BoundedRetriesOnTransientErrors) {
  database::concurrency::RetryPolicy retry(3, std::chrono::milliseconds(1));
  database::concurrency::RetryStats stats;

  int call_count = 0;
  retry.execute([&call_count]() {
    call_count++;
    if (call_count < 3) {
      throw database::error::TransientException("Database lock busy", database::error::EngineType::SQLite);
    }
  }, &stats);

  EXPECT_EQ(call_count, 3);
  EXPECT_EQ(stats.retries, 2);
  EXPECT_TRUE(stats.succeeded);

  // Permanent constraint error should fail immediately without retrying
  int permanent_calls = 0;
  EXPECT_THROW(retry.execute([&permanent_calls]() {
    permanent_calls++;
    throw database::error::ConflictException("Unique violation", database::error::EngineType::SQLite);
  }), database::error::ConflictException);

  EXPECT_EQ(permanent_calls, 1);
}

TEST(ConcurrencyTest, MultiWorkerConcurrentReadsAndWrites) {
  const std::string db_file = "test_concurrent.db";
  std::filesystem::remove(db_file);

  database::DatabaseConfig config{
      .database_path = db_file,
      .busy_timeout = std::chrono::milliseconds(5000),
      .foreign_keys = true,
      .wal_mode = true,
  };

  // Setup schema
  {
    database::Connection setup_conn(config);
    setup_conn.execute("CREATE TABLE IF NOT EXISTS counter (id INT PRIMARY KEY, val INT)");
    setup_conn.execute("INSERT INTO counter VALUES (1, 0)");
  }

  database::concurrency::ConnectionPool pool(config, 4);
  database::concurrency::RetryPolicy retry(10, std::chrono::milliseconds(10));

  constexpr int num_workers = 8;
  constexpr int increments_per_worker = 10;
  std::atomic<int> successful_writes{0};

  std::vector<std::thread> workers;
  workers.reserve(num_workers);

  for (int w = 0; w < num_workers; ++w) {
    workers.emplace_back([&pool, &retry, &successful_writes]() {
      for (int i = 0; i < increments_per_worker; ++i) {
        auto conn = pool.acquire();
        retry.execute([&conn, &successful_writes]() {
          database::Transaction tx(conn.get());
          int current = conn->execute_scalar_int("SELECT val FROM counter WHERE id = 1");
          conn->execute("UPDATE counter SET val = " + std::to_string(current + 1) + " WHERE id = 1");
          tx.commit();
          successful_writes++;
        });
      }
    });
  }

  for (auto& t : workers) {
    t.join();
  }

  EXPECT_EQ(successful_writes.load(), num_workers * increments_per_worker);

  database::Connection check_conn(config);
  const int final_val = check_conn.execute_scalar_int("SELECT val FROM counter WHERE id = 1");
  EXPECT_EQ(final_val, num_workers * increments_per_worker);

  std::filesystem::remove(db_file);
  std::filesystem::remove(db_file + "-wal");
  std::filesystem::remove(db_file + "-shm");
}

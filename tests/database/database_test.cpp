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

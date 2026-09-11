#include <gtest/gtest.h>

#include <stdexcept>

#include "database/database.h"
#include "database/database_config.h"
#include "database/database_factory.h"
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

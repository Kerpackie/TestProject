#include <gtest/gtest.h>

#include "database/database.h"

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

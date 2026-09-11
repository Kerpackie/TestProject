# Video & Blog Script: Phase 4 — Enterprise Database Abstraction (PostgreSQL & MariaDB Adapters)

> **Target Audience**: C++ Developers, Systems Engineers, and Software Architects  
> **Format**: Video Tutorial / Technical Blog Post Blueprint  
> **Topic**: Scaling to Enterprise Relational Databases: Abstraction Ports, Engine Adapters, & Normalized Error Handling (Phase 4)

---

## 1. Executive Summary & Hook

### The "Lowest Common Denominator" Database Abstraction Trap
When developers attempt to support multiple database engines (SQLite, PostgreSQL, MariaDB) in C++, they often fall into one of two major design traps:
- **Leaking Engine Specifics**: Hardcoding SQLite-specific row IDs (`getLastInsertRowid()`) or PostgreSQL-specific return clauses (`RETURNING id`) directly into application interfaces.
- **Lowest-Common-Denominator Interfaces**: Stripping out transaction controls, prepared statements, and error codes in favor of a weak `execute(std::string sql)` string API.

### The Solution: Port-Adapter Architecture & Error Normalization
Phase 4 introduces a clean port-adapter boundary and a normalized error taxonomy:
1. **Normalized Exception Hierarchy (`DatabaseException`)**: Maps engine-specific error codes (e.g. SQLite error 19, PostgreSQL SQLSTATE 23505, MariaDB Error 1062) to standard domain exceptions like `ConflictException` and `NotFoundException`.
2. **Unified Session Port (`IDatabaseSession`, `ITransaction`)**: Abstracts transaction lifecycle and command execution behind engine-agnostic interfaces.
3. **Pluggable Engine Adapters**: Implements concrete adapters for `SQLite`, `PostgreSQL`, and `MariaDB`.
4. **Cross-Engine Contract Tests**: Runs identical parameterized GoogleTest suites against every engine adapter to guarantee behavioral parity.

---

## 2. Architecture & Component Flow

```mermaid
flowchart TD
    subgraph Composition Root / Application
        APP[Application / Service Layer]
    end

    subgraph Unified Application Ports [libs/database]
        PORT_SESS[IDatabaseSession Interface]
        PORT_TX[ITransaction Interface]
        ERR_NORM[Normalized Exceptions<br/>ConflictException, NotFoundException]
    end

    subgraph Pluggable Engine Adapters
        SQLITE_ADAPTER[SqliteSession Adapter]
        POSTGRES_ADAPTER[PostgresSession Adapter]
        MARIADB_ADAPTER[MariaDbSession Adapter]
    end

    subgraph Underlying Drivers
        SQLITE_DRIVER[SQLite3 Engine]
        PQXX_DRIVER[libpqxx Driver]
        MARIADB_DRIVER[MariaDB Connector/C++]
    end

    APP -->|1. Interacts via| PORT_SESS
    APP -->|2. Guards via| PORT_TX
    APP -->|3. Catches normalized| ERR_NORM

    SQLITE_ADAPTER ..|>|Implements| PORT_SESS
    POSTGRES_ADAPTER ..|>|Implements| PORT_SESS
    MARIADB_ADAPTER ..|>|Implements| PORT_SESS

    SQLITE_ADAPTER -->|Wraps| SQLITE_DRIVER
    POSTGRES_ADAPTER -->|Wraps| PQXX_DRIVER
    MARIADB_ADAPTER -->|Wraps| MARIADB_DRIVER

    style APP fill:#2d3748,stroke:#4a5568,color:#fff
    style PORT_SESS fill:#2b6cb0,stroke:#3182ce,color:#fff
    style PORT_TX fill:#2b6cb0,stroke:#3182ce,color:#fff
    style ERR_NORM fill:#744210,stroke:#975a16,color:#fff
    style SQLITE_ADAPTER fill:#2f855a,stroke:#38a169,color:#fff
    style POSTGRES_ADAPTER fill:#2f855a,stroke:#38a169,color:#fff
    style MARIADB_ADAPTER fill:#2f855a,stroke:#38a169,color:#fff
    style SQLITE_DRIVER fill:#1a202c,stroke:#2d3748,color:#fff
    style PQXX_DRIVER fill:#1a202c,stroke:#2d3748,color:#fff
    style MARIADB_DRIVER fill:#1a202c,stroke:#2d3748,color:#fff
```

### Key Architectural Invariants
- **Zero Engine Driver Contamination**: Application services never `#include` driver headers (`<SQLiteCpp/...>`, `<pqxx/pqxx>`, `<mariadb/...>`).
- **Normalized Error Taxonomy**: Regardless of whether SQLite, PostgreSQL, or MariaDB triggers a duplicate key violation, the application catches `database::error::ConflictException`.

---

## 3. Step-by-Step Implementation Guide

### Step 1: Normalized Error Taxonomy
**File**: `libs/database/include/database/error/database_error.h`

```cpp
#pragma once

#include <stdexcept>
#include <string>

namespace database::error {

enum class EngineType { SQLite, PostgreSQL, MariaDB };

class DatabaseException : public std::runtime_error {
 public:
  explicit DatabaseException(const std::string& message, EngineType engine)
      : std::runtime_error(message), engine_(engine) {}

  EngineType engine() const noexcept { return engine_; }

 private:
  EngineType engine_;
};

class ConflictException : public DatabaseException {
 public:
  explicit ConflictException(const std::string& message, EngineType engine)
      : DatabaseException("Conflict (Unique Constraint Violation): " + message, engine) {}
};

}  // namespace database::error
```

> **Voiceover / Blog Callout**:
> *"Notice how `ConflictException` records which `EngineType` raised the exception while providing a uniform interface for caller catch blocks."*

---

### Step 2: Unified Database Session Ports
**File**: `libs/database/include/database/session/database_session.h`

```cpp
#pragma once

#include <memory>
#include <string_view>
#include "database/error/database_error.h"

namespace database::session {

class ITransaction {
 public:
  virtual ~ITransaction() = default;
  virtual void commit() = 0;
  virtual bool is_committed() const noexcept = 0;
};

class IDatabaseSession {
 public:
  virtual ~IDatabaseSession() = default;

  virtual std::unique_ptr<ITransaction> begin_transaction() = 0;
  virtual void execute(std::string_view sql) = 0;
  virtual int execute_scalar_int(std::string_view sql) = 0;
  virtual bool is_open() const noexcept = 0;
  virtual error::EngineType engine_type() const noexcept = 0;
};

class IDatabaseFactory {
 public:
  virtual ~IDatabaseFactory() = default;
  virtual std::unique_ptr<IDatabaseSession> create_session() = 0;
  virtual error::EngineType engine_type() const noexcept = 0;
};

}  // namespace database::session
```

---

### Step 3: SQLite Adapter Error Normalization
**File**: `libs/database/src/adapter/sqlite_session.cpp`

```cpp
void SqliteSession::execute(std::string_view sql) {
  try {
    connection_.execute(std::string(sql));
  } catch (const SQLite::Exception& e) {
    if (e.getErrorCode() == 19 || e.getExtendedErrorCode() == 2067) { // SQLITE_CONSTRAINT_UNIQUE
      throw error::ConflictException(e.what(), error::EngineType::SQLite);
    }
    throw error::DatabaseException(e.what(), error::EngineType::SQLite);
  }
}
```

---

### Step 4: Cross-Engine Contract Testing with GoogleTest
**File**: `tests/database/database_test.cpp`

```cpp
class CrossEngineContractTest : public ::testing::TestWithParam<database::error::EngineType> {};

INSTANTIATE_TEST_SUITE_P(
    AllEngines,
    CrossEngineContractTest,
    ::testing::Values(
        database::error::EngineType::SQLite,
        database::error::EngineType::PostgreSQL,
        database::error::EngineType::MariaDB));

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
  }
}
```

> **Voiceover / Blog Callout**:
> *"GoogleTest's `TestWithParam` lets us run the exact same behavioral test suite against SQLite, PostgreSQL, and MariaDB adapters with zero code repetition!"*

---

### Step 5: Multi-Engine Swapping in Composition Root
**File**: `app/main.cpp`

```cpp
database::adapter::SqliteDatabaseFactory sqlite_factory;
database::adapter::PostgresDatabaseFactory postgres_factory;
database::adapter::MariaDbDatabaseFactory mariadb_factory;

std::vector<database::session::IDatabaseFactory*> factories = {
    &sqlite_factory,
    &postgres_factory,
    &mariadb_factory,
};

for (auto* factory : factories) {
  std::cout << "\n--- Bootstrapping Engine: " << database::error::engine_type_to_string(factory->engine_type()) << " ---\n";
  auto session = factory->create_session();
  
  session->execute("CREATE TABLE users (id INT PRIMARY KEY, email TEXT UNIQUE)");
  
  try {
    session->execute("INSERT INTO users (email) VALUES ('duplicate@example.com')");
    session->execute("INSERT INTO users (email) VALUES ('duplicate@example.com')");
  } catch (const database::error::ConflictException& ex) {
    std::cout << "Caught normalized ConflictException on " 
              << database::error::engine_type_to_string(ex.engine()) << ": " << ex.what() << '\n';
  }
}
```

---

## 4. Common Pitfalls & Anti-Patterns

| Anti-Pattern | Why it Fails | Phase 4 Best Practice |
|---|---|---|
| `#ifdef MARIADB` in Repositories | Pollutes domain code with preprocessor macros; impossible to test multiple engines in one binary. | Use clean virtual ports (`IDatabaseSession`) and pluggable engine adapters. |
| Catching raw driver exceptions in services | Tight coupling to `SQLite::Exception` or `pqxx::sql_error`. | Catch normalized exceptions (`ConflictException`, `NotFoundException`). |
| Pretending all SQL dialects are identical | Causes runtime SQL syntax errors on complex queries. | Keep portable CRUD operations portable, and isolate engine-specific features behind capabilities. |
| Skipping contract tests for secondary engines | Features work on SQLite but break silently when deployed to PostgreSQL or MariaDB. | Use GoogleTest `TestWithParam` to enforce identical contract test passes across all engines. |

---

## 5. Live Terminal Demo & Verification Cues

### Commands to Run On Screen / In Article
1. **Compile Application & Suite**:
   ```bash
   cmake --build build
   ```
2. **Execute Cross-Engine Contract Tests (17 Total Tests)**:
   ```bash
   ctest --test-dir build --output-on-failure
   ```
   *Expected Output*: `100% tests passed out of 17` (including 6 parameterized `CrossEngineContractTest` instances covering SQLite, PostgreSQL, and MariaDB).
3. **Execute Application Binary**:
   ```bash
   ./build/debug/app/app
   ```
   *Expected Output*:
   ```text
   ================================------------------------
    Phase 4: Multi-Engine Abstraction Demonstration
   ================================------------------------

   --- Bootstrapping Engine: SQLite ---
   Session active: YES
   Transaction committed successfully.

   --- Bootstrapping Engine: PostgreSQL ---
   Session active: YES
   Transaction committed successfully.
   Caught normalized ConflictException on PostgreSQL: Conflict (Unique Constraint Violation): Key (email)=(duplicate@example.com) already exists

   --- Bootstrapping Engine: MariaDB ---
   Session active: YES
   Transaction committed successfully.
   Caught normalized ConflictException on MariaDB: Conflict (Unique Constraint Violation): Duplicate entry 'duplicate@example.com' for key 'users.email_unique'
   ```

---

## 6. Teaser for Phase 5

Now that our database layer seamlessly supports multiple database backends under normalized error contracts, we are ready for **Phase 5: Concurrency Model & Connection Pooling**.

In Phase 5, we will cover:
- Thread-safe connection pools (`ConnectionPool`).
- Managing lock contention and SQLite WAL concurrency vs PostgreSQL worker pools.
- Multi-worker load testing under high thread contention.

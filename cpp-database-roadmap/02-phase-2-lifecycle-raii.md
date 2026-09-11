# Phase 2 — Production Database Lifecycle: RAII Connections, Transactions & Configuration

## 1. The Pain Point

A common first implementation creates `SQLite::Database` directly inside a repository method, executes SQL, and lets the object disappear. This feels simple, but it hides lifecycle policy:

- connection flags are duplicated;
- pragmas are inconsistently applied;
- transaction boundaries are accidental;
- a thrown exception can leave a future maintainer unsure whether work was committed;
- configuration is mixed into business logic;
- tests cannot easily substitute an in-memory database or temporary file.

Production code needs an explicit owner for configuration and connection creation, and a transaction object whose lifetime clearly defines commit/rollback semantics.

## 2. Objective & Architecture

Introduce four infrastructure concepts:

1. `DatabaseConfig`: immutable configuration value.
2. `DatabaseConnection`: RAII wrapper around one SQLiteCpp database handle.
3. `Transaction`: a scope guard that commits explicitly and rolls back on exceptional/uncommitted exit.
4. `DatabaseFactory` or `DatabaseService`: central policy point for creating configured connections.

```text
Application
   |
   v
DatabaseService ---> DatabaseConfig
   |
   +--> DatabaseConnection ---> SQLiteCpp::Database
   |
   +--> Transaction ------------> SQLiteCpp::Transaction
```

The connection service owns policy; repositories own transaction *intent* but should not know connection bootstrap details.

## 3. Technical Specifications

- Pass configuration by value or `const&`; do not store raw pointers to configuration.
- Make constructors establish valid invariants or throw.
- Do not expose a non-owning raw SQLite handle unless a lower layer specifically requires it.
- Use `std::filesystem::path` for local database paths.
- Treat transaction commit as an explicit success action.
- Prefer one obvious ownership chain over shared global state.
- Convert infrastructure exceptions only at boundaries where a more stable domain/application error is useful.

## 4. File Structure

```text
include/database/
├── DatabaseConfig.h
├── DatabaseConnection.h
├── DatabaseFactory.h
└── Transaction.h
src/database/
├── DatabaseConnection.cpp
├── DatabaseFactory.cpp
└── Transaction.cpp
tests/
└── database_lifecycle_test.cpp
```

## 5. Draft Code

### `DatabaseConfig.h`

```cpp
#pragma once

#include <chrono>
#include <filesystem>
#include <string>

struct DatabaseConfig final {
    // A value type is intentionally used here. Configuration should be easy
    // to copy into a service and easy to construct in tests.
    std::filesystem::path database_path{":memory:"};

    // SQLite connection behavior. Keep flags visible rather than burying them
    // in call sites so operational changes are auditable.
    int open_flags{};

    // Used later for contention handling. A finite timeout is preferable to
    // an unbounded wait in a production service.
    std::chrono::milliseconds busy_timeout{5000};

    bool foreign_keys{true};
};
```

### `DatabaseConnection.h`

```cpp
#pragma once

#include <SQLiteCpp/Database.h>
#include "DatabaseConfig.h"

class DatabaseConnection final {
public:
    explicit DatabaseConnection(const DatabaseConfig& config);

    // Non-copyable: duplicating the wrapper could accidentally suggest that
    // two objects own the same connection. SQLiteCpp already owns the handle.
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    DatabaseConnection(DatabaseConnection&&) noexcept = default;
    DatabaseConnection& operator=(DatabaseConnection&&) noexcept = default;

    SQLite::Database& native() noexcept { return database_; }
    const SQLite::Database& native() const noexcept { return database_; }

private:
    SQLite::Database database_;
};
```

### `DatabaseConnection.cpp`

```cpp
#include "database/DatabaseConnection.h"

#include <string>

DatabaseConnection::DatabaseConnection(const DatabaseConfig& config)
    : database_(config.database_path.string(), config.open_flags) {

    // PRAGMA configuration is connection-scoped in SQLite. Apply it immediately
    // after opening so every connection produced by this component is configured
    // consistently.
    if (config.foreign_keys) {
        database_.exec("PRAGMA foreign_keys = ON");
    }

    // Avoid string concatenation for numeric values when a direct SQLite API
    // exists. SQLiteCpp exposes execute helpers, so use SQL here only where
    // the setting is naturally expressed as PRAGMA.
    database_.exec(
        "PRAGMA busy_timeout = " +
        std::to_string(config.busy_timeout.count()));
}
```

SQLite documents both the `busy_timeout` API and the equivalent pragma as mechanisms for waiting on locks before returning `SQLITE_BUSY`. citeturn721891search0

### `Transaction.h`

```cpp
#pragma once

#include <SQLiteCpp/Transaction.h>

class Transaction final {
public:
    explicit Transaction(SQLite::Database& database)
        : tx_(database), committed_(false) {}

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    ~Transaction() noexcept {
        // Destructors must not throw. SQLiteCpp's transaction object handles
        // rollback semantics when it is destroyed without commit.
        // We intentionally do not call commit from a destructor because
        // destruction must never turn a partially completed operation into a
        // successful transaction.
    }

    void commit() {
        tx_.commit();
        committed_ = true;
    }

    SQLite::Transaction& native() noexcept { return tx_; }

private:
    SQLite::Transaction tx_;
    bool committed_;
};
```

> Teaching note: the explicit `committed_` bit documents intent even though SQLiteCpp's transaction object already owns the rollback-on-destruction behavior. Keep the commit operation visually close to the end of the successful unit of work.

## 6. Usage Pattern

```cpp
DatabaseConfig config{
    .database_path = ":memory:",
    .open_flags = SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE,
    .busy_timeout = std::chrono::milliseconds{5000},
    .foreign_keys = true,
};

DatabaseConnection connection(config);

Transaction tx(connection.native());

connection.native().exec("CREATE TABLE account(id INTEGER PRIMARY KEY, name TEXT NOT NULL)");
connection.native().exec("INSERT INTO account(name) VALUES('Ada')");

tx.commit();
```

## 7. Verification Criteria

Test these cases explicitly:

1. **Happy path** — committed changes are visible after scope exit.
2. **Exception path** — throw after an insert but before `commit()`; verify the insert is absent.
3. **Configuration invariant** — foreign-key violations fail when `foreign_keys = true`.
4. **Timeout invariant** — two connections contend for a lock and return after a bounded wait.
5. **In-memory isolation** — separate `:memory:` connections are separate databases unless a shared-cache URI is intentionally used.
6. **Move semantics** — moving a connection does not double-close the underlying handle.

## 8. Gate to Phase 3

The next phase must be able to construct repositories without knowing which concrete database wrapper owns the connection. Lifecycle is now an infrastructure concern rather than an application concern.

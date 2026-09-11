# Phase 4 — Scaling to Enterprise Databases: PostgreSQL / MariaDB Abstraction

## 1. The Pain Point

A naïve “database abstraction” often does one of two bad things:

1. It hard-codes SQLite concepts into an interface and makes other databases feel awkward.
2. It hides everything behind a lowest-common-denominator `execute(string)` API, throwing away transactions, prepared statements, native typing, and engine-specific capabilities.

PostgreSQL ships `libpq` as its primary C client interface, while libpqxx provides a modern C++ API on top. libpqxx models connections, transactions, and results as distinct concepts and follows RAII-style lifetime management. citeturn397109search0turn397109search4turn397109search5 MariaDB provides its own C++ connector with native connection functionality. citeturn721891search8

## 2. Objective & Architecture

Split the system into:

- **application ports**: what the application needs;
- **engine adapters**: SQLite, PostgreSQL, MariaDB;
- **engine capability model**: optional features such as returning clauses, JSON, bulk operations, or advisory locking.

```text
                         Application Port
                              ^
              +---------------+---------------+
              |               |               |
       SQLite Adapter   PostgreSQL Adapter  MariaDB Adapter
              |               |               |
         SQLiteCpp          libpqxx       MariaDB Connector/C++
```

The abstraction boundary should not promise identical SQL dialects. Instead, keep portable repository operations portable and isolate engine-specific operations in explicit capability interfaces.

## 3. Technical Specifications

- Define a narrow `IDatabaseSession`/transaction port only for behavior needed by application code.
- Use an adapter per engine rather than conditional compilation throughout repositories.
- Store engine-specific connection strings/configuration in engine-specific config types behind a stable application bootstrap interface.
- Normalize only errors that the application can act upon meaningfully (`not_found`, `conflict`, `transient_failure`, etc.). Preserve original diagnostics for logs.
- Keep migrations engine-aware when SQL cannot be made portable without becoming unreadable.

## 4. File Structure

```text
include/database/
├── IDatabaseFactory.h
├── ITransaction.h
└── DatabaseError.h
include/infrastructure/
├── sqlite/SqliteDatabaseFactory.h
├── postgres/PostgresDatabaseFactory.h
└── mariadb/MariaDbDatabaseFactory.h
src/infrastructure/
├── sqlite/
├── postgres/
└── mariadb/
tests/
└── cross_engine_contract_tests.cpp
```

## 5. Draft Code

### Stable application-facing session

```cpp
#pragma once

#include <memory>
#include <string_view>

class ITransaction {
public:
    virtual ~ITransaction() = default;

    // Commit is deliberately explicit. Destruction means “rollback/abort if
    // the underlying driver supports that semantics”, not success.
    virtual void commit() = 0;
};

class IDatabaseSession {
public:
    virtual ~IDatabaseSession() = default;

    virtual std::unique_ptr<ITransaction> begin_transaction() = 0;

    // Keep this method small. If the application grows a need for richer
    // semantics, introduce a dedicated port rather than turning this into a
    // generic universal database API.
    virtual long long execute_affected(std::string_view sql) = 0;
};
```

### Capability split

```cpp
class IReturningInsertSupport {
public:
    virtual ~IReturningInsertSupport() = default;
    virtual long long insert_and_return_id(/* domain inputs */) = 0;
};
```

This is preferable to forcing every engine to emulate every PostgreSQL/MariaDB/SQLite feature.

### PostgreSQL adapter sketch

```cpp
#include <pqxx/pqxx>

class PostgresTransaction final : public ITransaction {
public:
    explicit PostgresTransaction(pqxx::connection& connection)
        : tx_(connection) {}

    void commit() override {
        tx_.commit();
    }

private:
    // libpqxx transaction lifetime owns the transactional scope.
    pqxx::work tx_;
};
```

libpqxx documents `pqxx::connection` as the connection object and transactions such as `pqxx::work` as the objects used to execute SQL; it also states that transactions roll back when destroyed without commit. citeturn397109search4turn397109search5

### MariaDB adapter sketch

```cpp
// Pseudocode-shaped boundary: exact constructor details belong in the engine
// adapter and should follow the installed connector version's API.
class MariaDbDatabaseSession final : public IDatabaseSession {
public:
    // The connector-specific connection is owned here.
    std::unique_ptr<ITransaction> begin_transaction() override;
    long long execute_affected(std::string_view sql) override;
};
```

MariaDB's current C++ connector is an object-oriented native client library; pin the connector version and verify its CMake/import target in the environment used for the production build. citeturn721891search8

## 6. Cross-Engine Contract Testing

The repository contract should be expressed as reusable tests:

```text
Given an empty database
When the repository creates a user
Then the user can be loaded by id

Given two users
When one is deleted
Then only that user's lookup returns not_found

Given an invalid duplicate key
Then the adapter surfaces a normalized conflict error
```

Run the same behavioral suite against SQLite, PostgreSQL, and MariaDB in CI. Driver-specific tests remain separate.

## 7. Verification Criteria

Pass when:

1. Repositories depend on application ports, not engine headers.
2. Switching the composition root from SQLite to PostgreSQL does not change domain/service code.
3. A MariaDB adapter can be introduced without adding `#ifdef MARIADB` to every repository.
4. Native capabilities have explicit extension points.
5. Error normalization preserves enough source information for diagnostics.
6. The same repository contract tests run against every supported engine.

## 8. Gate to Phase 5

Once more than one engine exists, connection/thread ownership becomes an architectural issue. The next phase defines the concurrency model rather than allowing each adapter to invent its own.

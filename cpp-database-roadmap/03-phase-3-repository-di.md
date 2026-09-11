# Phase 3 — Architecture & Design Patterns: Repository Pattern + Manual DI

## 1. The Pain Point

A codebase quickly becomes unmaintainable when `main()`, services, and controllers all execute SQL directly. Typical failure modes include:

- SQL scattered across business operations;
- duplicated mapping logic;
- test code forced to boot a real database;
- repositories acquiring global connections;
- circular dependencies caused by service locators;
- “interfaces everywhere” with no clear boundary.

The answer is not to build a giant generic DAO abstraction. We need a small, stable port around the behavior the application actually requires.

## 2. Objective & Architecture

Introduce:

- domain entities/value types;
- repository interfaces owned by the application/domain side;
- infrastructure implementations using SQLiteCpp;
- application services consuming repository interfaces;
- manual dependency injection in the composition root.

```text
                 +-----------------------+
                 | Composition Root      |
                 | main / bootstrap       |
                 +-----------+-----------+
                             |
                    constructs concrete
                             |
       +---------------------+----------------------+
       |                                            |
Application Service                         SQLite Repository
       |                                            |
       v                                            v
IUserRepository <--------------------> SQLiteCpp / DB layer
```

## 3. Technical Specifications

- Interfaces should model use cases, not SQL syntax.
- Repository methods should communicate ownership and error behavior clearly.
- Prefer `std::optional<T>` for “not found” rather than sentinel objects.
- Return domain values, not `SQLite::Column` or wrapper-specific types.
- Inject dependencies through constructors.
- The composition root may depend on every concrete implementation; leaf modules should not depend upward.
- Avoid a global singleton database connection.

## 4. File Structure

```text
include/domain/
└── User.h
include/application/
├── UserService.h
└── IUserRepository.h
include/infrastructure/sqlite/
└── SqliteUserRepository.h
src/application/
└── UserService.cpp
src/infrastructure/sqlite/
└── SqliteUserRepository.cpp
src/main.cpp
tests/
├── user_service_test.cpp
└── sqlite_user_repository_test.cpp
```

## 5. Draft Code

### Domain model

```cpp
#pragma once

#include <cstdint>
#include <string>

struct User final {
    std::int64_t id{};
    std::string name;
};
```

### Repository port

```cpp
#pragma once

#include <cstdint>
#include <optional>
#include "domain/User.h"

class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    // The application asks for a user. It does not ask how rows are stored.
    virtual std::optional<User> find_by_id(std::int64_t id) = 0;

    // Create returns the new identity. The interface intentionally avoids
    // exposing SQLite's last-insert-rowid API.
    virtual std::int64_t create(const User& user) = 0;
};
```

### SQLite implementation

```cpp
#pragma once

#include <SQLiteCpp/Database.h>
#include "application/IUserRepository.h"

class SqliteUserRepository final : public IUserRepository {
public:
    explicit SqliteUserRepository(SQLite::Database& database)
        : database_(database) {}

    std::optional<User> find_by_id(std::int64_t id) override;
    std::int64_t create(const User& user) override;

private:
    // Non-owning reference: the composition root owns the connection.
    // This makes the lifetime relationship explicit and prevents accidental
    // shared_ptr cycles.
    SQLite::Database& database_;
};
```

### Repository implementation

```cpp
#include "infrastructure/sqlite/SqliteUserRepository.h"

std::optional<User> SqliteUserRepository::find_by_id(std::int64_t id) {
    SQLite::Statement query(
        database_,
        "SELECT id, name FROM user WHERE id = ?"
    );

    query.bind(1, id); // Always bind values; never interpolate them into SQL.

    if (!query.executeStep()) {
        return std::nullopt;
    }

    User user;
    user.id = query.getColumn(0).getInt64();
    user.name = query.getColumn(1).getString();
    return user;
}

std::int64_t SqliteUserRepository::create(const User& user) {
    // The repository owns the identity policy. We intentionally let SQLite
    // allocate the INTEGER PRIMARY KEY instead of inserting a caller-supplied
    // id and then separately asking for the generated id.
    SQLite::Statement insert(
        database_,
        "INSERT INTO user(name) VALUES(?)"
    );

    insert.bind(1, user.name);
    insert.exec();

    return database_.getLastInsertRowid();
}
```

### Manual dependency injection

```cpp
int main() {
    DatabaseConfig config{/* production values loaded here */};
    DatabaseConnection connection(config);

    // Schema/migrations would be run before repositories are exposed.
    SqliteUserRepository users(connection.native());
    UserService service(users);

    // Only the composition root knows that SQLiteCpp is being used.
    service.register_user(User{0, "Ada"});
}
```

## 6. Why Manual DI First?

Manual DI makes object ownership and dependency direction visible. A framework can automate construction later, but a tutorial should first teach the dependency graph itself.

Use `std::unique_ptr<IUserRepository>` when the composition root genuinely needs runtime polymorphic ownership:

```cpp
std::unique_ptr<IUserRepository> repo =
    std::make_unique<SqliteUserRepository>(connection.native());
```

Use `std::shared_ptr` only when multiple components truly share a lifetime. Ownership should not be added merely because an interface is involved.

## 7. Verification Criteria

1. Application code compiles without including `<SQLiteCpp/...>`.
2. `UserService` can be unit-tested with a fake repository.
3. SQLite integration tests exercise the real repository against `:memory:` or a temporary database.
4. No repository reaches for a global singleton.
5. No domain object contains SQLiteCpp types.
6. A transaction boundary can be introduced above a multi-repository operation without changing the domain API.

## 8. Gate to Phase 4

Before adding PostgreSQL/MariaDB, identify which behaviors are genuinely shared and which are engine-specific. The adapter interface should be driven by application behavior, not by copying SQLiteCpp's API into an abstract base class.

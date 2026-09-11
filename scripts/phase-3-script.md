# Video & Blog Script: Phase 3 — Architecture & Design Patterns: Repository Pattern + Manual DI

> **Target Audience**: C++ Developers, Systems Engineers, and Software Architects  
> **Format**: Video Tutorial / Technical Blog Post Blueprint  
> **Topic**: Clean Architecture in C++: Domain Models, Repository Ports, Application Services, & Manual Dependency Injection (Phase 3)

---

## 1. Executive Summary & Hook

### The "Scattered SQL & Tight Coupling" Problem in C++
In rapidly growing C++ applications, SQL statements often leak into UI controllers, web handlers, and business logic methods. This anti-pattern causes critical architecture problems:
- **Tight Infrastructure Coupling**: Application code directly invokes driver APIs (e.g. `SQLite::Statement`), making it impossible to swap database engines or run fast unit tests without a database handle.
- **Duplicated Data Mapping**: Row-to-struct mapping logic is copied across multiple endpoints, leading to bugs when database schemas evolve.
- **Service Locator & Global Singletons**: Reaching for global database singletons creates hidden dependencies and fragile lifetime management.

### The Solution: Clean Architecture Ports & Manual DI
Phase 3 decouples business logic from database drivers using the **Repository Pattern** and **Manual Dependency Injection**:
1. **Domain Model (`User`)**: Plain C++ value type representing business entities, completely free of database driver types.
2. **Repository Port (`IUserRepository`)**: Abstract interface defined by the application layer modeling business queries (`find_by_id`, `create`, `find_by_email`), returning `std::optional<User>`.
3. **Infrastructure Adapter (`SqliteUserRepository`)**: Concrete repository executing parameterized SQLite queries without exposing driver types in consumer headers.
4. **Application Service (`UserService`)**: Consumes `IUserRepository&` via constructor injection to enforce business rules (e.g., duplicate email prevention).
5. **Composition Root (`main()`)**: Explicitly wires together configuration, connection, repository, and service instances at startup.

---

## 2. Architecture & Component Flow

```mermaid
flowchart TD
    subgraph Composition Root [main.cpp]
        ROOT[main / Bootstrap]
    end

    subgraph Application Layer
        SERVICE[UserService]
        PORT[IUserRepository Interface]
    end

    subgraph Domain Layer
        ENTITY[User Value Struct]
    end

    subgraph Infrastructure Layer [libs/database]
        SQLITE_REPO[SqliteUserRepository]
        CONN[Connection Handle]
    end

    subgraph Driver Layer [PRIVATE]
        SQLITE_DRIVER[SQLiteCpp / sqlite3]
    end

    ROOT -->|1. Instantiates| CONN
    ROOT -->|2. Instantiates| SQLITE_REPO
    ROOT -->|3. Injects repo into| SERVICE
    SQLITE_REPO ..|>|Implements| PORT
    SERVICE -->|Depends on abstraction| PORT
    PORT -->|Returns/Accepts| ENTITY
    SQLITE_REPO -->|Uses| CONN
    CONN -->|Encapsulates| SQLITE_DRIVER

    style ROOT fill:#2d3748,stroke:#4a5568,color:#fff
    style SERVICE fill:#2b6cb0,stroke:#3182ce,color:#fff
    style PORT fill:#2b6cb0,stroke:#3182ce,color:#fff
    style ENTITY fill:#744210,stroke:#975a16,color:#fff
    style SQLITE_REPO fill:#2f855a,stroke:#38a169,color:#fff
    style CONN fill:#2f855a,stroke:#38a169,color:#fff
    style SQLITE_DRIVER fill:#1a202c,stroke:#2d3748,color:#fff
```

### Key Architectural Invariants
- **Dependency Inversion Principle**: The high-level `UserService` depends on the `IUserRepository` abstraction, not on SQLiteCpp.
- **Fast Unit Testing**: `UserService` can be unit-tested in isolation using an in-memory `FakeUserRepository` without booting SQLite.
- **Value Semantics**: Domain models use standard C++ types (`std::int64_t`, `std::string`) and `std::optional<User>` for missing records.

---

## 3. Step-by-Step Implementation Guide

### Step 1: Plain Domain Model
**File**: `libs/database/include/database/domain/user.h`

```cpp
#pragma once

#include <cstdint>
#include <string>

namespace database::domain {

struct User final {
  std::int64_t id{0};
  std::string name;
  std::string email;
};

}  // namespace database::domain
```

> **Voiceover / Blog Callout**:
> *"Notice how `User` is a plain struct. It contains zero SQLiteCpp includes, pointer wrappers, or ORM annotations."*

---

### Step 2: Repository Port Interface
**File**: `libs/database/include/database/repository/user_repository.h`

```cpp
#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include "database/domain/user.h"

namespace database::repository {

class IUserRepository {
 public:
  virtual ~IUserRepository() = default;

  virtual std::optional<domain::User> find_by_id(std::int64_t id) = 0;
  virtual std::optional<domain::User> find_by_email(const std::string& email) = 0;
  virtual std::int64_t create(const domain::User& user) = 0;
  virtual bool update(const domain::User& user) = 0;
  virtual bool delete_by_id(std::int64_t id) = 0;
  virtual std::vector<domain::User> find_all() = 0;
};

}  // namespace database::repository
```

> **Voiceover / Blog Callout**:
> *"We use `std::optional<User>` for find queries to represent 'record not found' explicitly, avoiding magic sentinel values or NULL pointers."*

---

### Step 3: SQLite Repository Adapter Implementation
**Header**: `libs/database/include/database/repository/sqlite_user_repository.h`  
**Source**: `libs/database/src/repository/sqlite_user_repository.cpp`

```cpp
#include "database/repository/sqlite_user_repository.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include "../detail.h"

namespace database::repository {

SqliteUserRepository::SqliteUserRepository(Connection& connection)
    : connection_(connection) {}

std::optional<domain::User> SqliteUserRepository::find_by_id(std::int64_t id) {
  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db, "SELECT id, name, email FROM users WHERE id = ?");
  query.bind(1, id); // Always parameterize queries!

  if (!query.executeStep()) {
    return std::nullopt;
  }

  return domain::User{
      .id = query.getColumn(0).getInt64(),
      .name = query.getColumn(1).getString(),
      .email = query.getColumn(2).getString(),
  };
}

std::int64_t SqliteUserRepository::create(const domain::User& user) {
  SQLite::Database& db = detail::get_native_db(connection_);
  SQLite::Statement query(db, "INSERT INTO users (name, email) VALUES (?, ?)");
  query.bind(1, user.name);
  query.bind(2, user.email);
  query.exec();

  return db.getLastInsertRowid();
}

}  // namespace database::repository
```

> **Voiceover / Blog Callout**:
> *"The repository owns identity allocation policy and SQL binding. Notice how parameters are bound via index rather than string concatenation, eliminating SQL injection risks."*

---

### Step 4: Application Service with Business Rules
**Header**: `libs/database/include/database/service/user_service.h`  
**Source**: `libs/database/src/service/user_service.cpp`

```cpp
namespace database::service {

UserService::UserService(repository::IUserRepository& repository)
    : repository_(repository) {}

domain::User UserService::register_user(const std::string& name, const std::string& email) {
  if (repository_.find_by_email(email).has_value()) {
    throw UserAlreadyExistsException(email);
  }

  domain::User new_user{.id = 0, .name = name, .email = email};
  const std::int64_t new_id = repository_.create(new_user);
  new_user.id = new_id;
  return new_user;
}

}  // namespace database::service
```

---

### Step 5: Fast In-Memory Unit Testing with Fake Repository
**File**: `tests/database/database_test.cpp`

```cpp
class FakeUserRepository final : public database::repository::IUserRepository {
 public:
  std::optional<database::domain::User> find_by_email(const std::string& email) override {
    auto it = std::find_if(users_.begin(), users_.end(), [&email](const auto& u) { return u.email == email; });
    return (it != users_.end()) ? std::make_optional(*it) : std::nullopt;
  }

  std::int64_t create(const database::domain::User& user) override {
    auto created = user;
    created.id = ++next_id_;
    users_.push_back(created);
    return created.id;
  }
  // ... other methods omitted for brevity
 private:
  std::vector<database::domain::User> users_;
  std::int64_t next_id_{0};
};

TEST(UserServiceTest, UnitTestWithFakeRepository) {
  FakeUserRepository fake_repo;
  database::service::UserService service(fake_repo);

  auto user = service.register_user("Ada Lovelace", "ada@example.com");
  EXPECT_GT(user.id, 0);

  // Duplicate email check
  EXPECT_THROW(service.register_user("Ada Copy", "ada@example.com"), database::service::UserAlreadyExistsException);
}
```

> **Voiceover / Blog Callout**:
> *"Because `UserService` depends on `IUserRepository`, we can run sub-millisecond unit tests using `FakeUserRepository` without initializing an actual SQLite database handle!"*

---

### Step 6: Manual DI in Composition Root
**File**: `app/main.cpp`

```cpp
int main() {
  // 1. Composition Root: Infrastructure Setup
  database::DatabaseConfig config{.database_path = ":memory:"};
  database::Connection conn = database::DatabaseFactory::create(config);

  // 2. Instantiate Concrete Repository & Schema
  database::repository::SqliteUserRepository user_repo(conn);
  user_repo.init_schema();

  // 3. Inject Repository into Application Service
  database::service::UserService user_service(user_repo);

  // 4. Execute Business Use Cases
  database::Transaction tx(conn);
  auto user1 = user_service.register_user("Ada Lovelace", "ada@example.com");
  auto user2 = user_service.register_user("Alan Turing", "alan@example.com");
  tx.commit();

  std::cout << "Successfully registered 2 users using Manual DI!\n";
  return 0;
}
```

---

## 4. Common Pitfalls & Anti-Patterns

| Anti-Pattern | Why it Fails | Phase 3 Best Practice |
|---|---|---|
| Returning driver objects (e.g. `SQLite::Column`) | Leaks driver headers into application services and UI layers. | Map rows directly into plain C++ domain structs (`User`). |
| Service Locators / Global Singletons | Hides true dependencies; causes initialization order bugs. | Inject dependencies explicitly through constructors. |
| Leaking auto-increment row IDs into domain API | Exposes database-specific auto-increment mechanics. | Abstract creation behind `std::int64_t create(const User& user)`. |
| "Generic DAO" Abstractions (`DAO<T>`) | Produces bloated, artificial base classes that model SQL instead of use cases. | Define clean, domain-driven repository ports per aggregate (`IUserRepository`). |

---

## 5. Live Terminal Demo & Verification Cues

### Commands to Run On Screen / In Article
1. **Compile Application & Unit Tests**:
   ```bash
   cmake --build build
   ```
2. **Execute CTest Suite (11 Tests Total)**:
   ```bash
   ctest --test-dir build --output-on-failure
   ```
   *Expected Output*: `100% tests passed out of 11` (including `UserServiceTest.UnitTestWithFakeRepository` and `SqliteUserRepositoryTest.FullCrudOperationsOnInMemoryDatabase`).
3. **Execute Application Binary**:
   ```bash
   ./build/debug/app/app
   ```
   *Expected Output*:
   ```text
   TestProject 0.1.0
   Database module version 0.1.0

   --- Executing Phase 3 Application Use Cases (Manual DI) ---
   Registered users in transaction: ID 1 (Ada Lovelace), ID 2 (Alan Turing)
   Total registered users: 2
    - [1] Ada Lovelace <ada@example.com>
    - [2] Alan Turing <alan@example.com>
   Caught expected domain exception: User with email 'ada@example.com' already exists
   ```

---

## 6. Teaser for Phase 4

With clean domain interfaces, repositories, application services, and manual dependency injection fully established, we are ready for **Phase 4: Multi-Engine Abstraction (PostgreSQL & MariaDB)**.

In Phase 4, we will cover:
- Extending `libs/database` to support enterprise relational databases.
- Creating PostgreSQL (`pqxx`) and MariaDB adapters satisfying `IUserRepository` ports.
- Swapping database backends in the composition root without altering a single line of business logic.

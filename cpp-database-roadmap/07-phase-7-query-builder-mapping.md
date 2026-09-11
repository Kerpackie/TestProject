# Phase 7 — Query Building & Safety: Mapping C++ Structs to SQL

## 1. The Pain Point

Hand-written SQL is not inherently bad. Unstructured SQL is. Toy examples often concatenate values directly into strings, duplicate column mapping in every method, and return untyped rows that push conversion bugs into business code.

Typical production failures:

- SQL injection through string interpolation;
- mismatched column indexes after schema changes;
- inconsistent null handling;
- date/time and integer-width conversion bugs;
- accidental `SELECT *` coupling;
- duplicated pagination/filter construction;
- logs containing sensitive parameter values.

## 2. Objective & Architecture

Keep SQL explicit, but centralize the mechanics around it:

```text
Repository
   |
   +--> Query definition (SQL + typed parameters)
   |
   +--> Database statement
   |
   +--> Row mapper
   |
   v
Domain struct
```

The aim is not to invent a magical ORM. The reader should still be able to see the SQL and understand the exact statement the database receives.

## 3. Technical Specifications

- Always bind values.
- Build dynamic SQL only from trusted structural fragments (column names, operators, sort direction selected from enums/whitelists).
- Prefer explicit column lists.
- Define mapper functions close to repository code or a dedicated mapping module.
- Handle `NULL` explicitly with `std::optional<T>`.
- Preserve integer width (`std::int64_t` rather than plain `int` for database IDs).
- Treat SQL text and bound values as two different data classes.

## 4. File Structure

```text
include/database/
├── SqlStatement.h
├── QueryBuilder.h
└── RowMapper.h
src/database/
├── SqlStatement.cpp
└── QueryBuilder.cpp
include/domain/
└── User.h
src/infrastructure/sqlite/
└── SqliteUserRepository.cpp
tests/
└── query_mapping_test.cpp
```

## 5. Draft Code

### Typed statement wrapper

```cpp
class SqlStatement final {
public:
    explicit SqlStatement(SQLite::Database& db, std::string sql)
        : statement_(db, std::move(sql)) {}

    void bind(std::int64_t index, std::int64_t value) {
        statement_.bind(index, value);
    }

    void bind(std::int64_t index, std::string_view value) {
        statement_.bind(index, std::string{value});
    }

    SQLite::Statement& native() noexcept { return statement_; }

private:
    SQLite::Statement statement_;
};
```

### Explicit mapper

```cpp
std::optional<User> map_user(SQLite::Statement& statement) {
    if (!statement.executeStep()) {
        return std::nullopt;
    }

    User result{
        .id = statement.getColumn("id").getInt64(),
        .name = statement.getColumn("name").getString(),
    };

    return result;
}
```

Named-column access is intentionally preferred in educational code when supported by the wrapper; it makes a schema/SQL change easier to review than anonymous column numbers.

### Safe dynamic ordering

```cpp
enum class UserOrder {
    IdAscending,
    NameAscending,
};

std::string order_clause(UserOrder order) {
    switch (order) {
        case UserOrder::IdAscending:
            return "id ASC";
        case UserOrder::NameAscending:
            return "name ASC";
    }
    throw std::logic_error("unsupported order");
}
```

Notice that user input is converted into an enum before it ever becomes SQL structure. Parameter binding cannot safely parameterize an identifier or `ASC/DESC`, so structural fragments must be selected from trusted code.

### Pagination

```cpp
SQLite::Statement stmt(
    db,
    "SELECT id, name "
    "FROM user "
    "ORDER BY " + order_clause(order) +
    " LIMIT ? OFFSET ?"
);

stmt.bind(1, limit);
stmt.bind(2, offset);
```

## 6. Nullability & Mapping Rules

Use a policy such as:

```cpp
std::optional<std::string> email;
```

for nullable columns. Do not convert SQL `NULL` into an empty string unless that is an explicit domain rule.

Likewise, distinguish “not found” from “found with null field”: these are different states and should remain different in C++.

## 7. Verification Criteria

1. Security tests prove interpolated user input never becomes SQL structure.
2. Mapping tests cover null, empty, maximum-length, and Unicode values.
3. Integer IDs round-trip without truncation.
4. Column renames fail loudly in tests rather than silently mapping wrong data.
5. Query builders generate only whitelisted structural SQL.
6. Integration tests prove prepared/bound values behave identically across supported engines where portable semantics are promised.

## 8. Gate to Phase 8

Once query execution is centralized, the same boundary becomes the natural place to instrument timings, error classification, SQL fingerprints, row counts, and connection-level diagnostics.

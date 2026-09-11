# Phase 6 — Schema Evolution & Seeding: Versioned Automated Migrations

## 1. The Pain Point

The toy approach is `CREATE TABLE IF NOT EXISTS ...` in application startup. That fails as soon as the schema changes:

- existing installations need ALTERs, data backfills, and indexes;
- there is no record of which changes ran;
- two developers can deploy different schema states;
- tests may accidentally depend on execution order;
- seed data can be duplicated or partially applied.

A production database needs a monotonically ordered migration history and a process that makes database state converge deterministically.

## 2. Objective & Architecture

Create a migration engine with:

- versioned migration identifiers;
- forward-only `up()` functions or scripts;
- a migration history table;
- one transaction per migration where the target engine supports transactional DDL for the statements involved;
- explicit handling for operations that cannot be rolled back;
- idempotent, clearly classified seed data.

```text
Migration catalog
      |
      v
+-------------------+
| schema_migrations  |
| version | applied  |
+-------------------+
      |
      v
Runner compares applied versions to compiled/packaged migrations
```

## 3. Technical Specifications

- Migration versions must be totally ordered.
- Never edit an already-applied migration in place; add a new migration.
- Migration metadata should include version, name, checksum (optional but useful), and applied timestamp.
- Seed/reference data should use stable natural or business keys so reruns can be safe.
- Migration discovery must be deterministic.
- Production deployment should fail rather than silently skipping a missing migration.

## 4. File Structure

```text
include/database/migrations/
├── Migration.h
└── MigrationRunner.h
src/database/migrations/
└── MigrationRunner.cpp
migrations/
├── 001_create_users.sql
├── 002_add_user_email.sql
└── 003_seed_reference_data.sql
tests/
└── migration_runner_test.cpp
```

## 5. Draft Code

### Migration metadata

```cpp
struct Migration final {
    std::int64_t version{};
    std::string name;

    // The callable receives a database session so the runner controls the
    // transaction boundary and error handling policy.
    std::function<void(SQLite::Database&)> apply;
};
```

### Migration history table

```sql
CREATE TABLE IF NOT EXISTS schema_migrations (
    version     INTEGER PRIMARY KEY,
    name        TEXT NOT NULL,
    checksum    TEXT,
    applied_at  TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

### Runner sketch

```cpp
void MigrationRunner::run(SQLite::Database& db,
                          const std::vector<Migration>& migrations) {
    db.exec("CREATE TABLE IF NOT EXISTS schema_migrations ("
            "version INTEGER PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "checksum TEXT,"
            "applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ")");

    auto applied = load_applied_versions(db);

    for (const auto& migration : migrations) {
        if (applied.contains(migration.version)) {
            // Optional production hardening: compare stored checksum with the
            // packaged checksum and fail if the content was mutated after it
            // was applied.
            continue;
        }

        SQLite::Transaction tx(db);
        migration.apply(db);

        SQLite::Statement record(
            db,
            "INSERT INTO schema_migrations(version, name, checksum) "
            "VALUES (?, ?, ?)"
        );
        record.bind(1, migration.version);
        record.bind(2, migration.name);
        // record.bind(3, migration.checksum);
        record.exec();

        tx.commit();
    }
}
```

### Seed design

```sql
INSERT INTO role(code, display_name)
VALUES ('admin', 'Administrator')
ON CONFLICT(code) DO UPDATE SET
    display_name = excluded.display_name;
```

When cross-database portability matters, do not assume every engine supports identical upsert syntax. Prefer adapter-specific migration scripts when necessary.

## 6. Startup vs Deployment

Do not automatically equate “migration runner” with “every application process may mutate production schema”. In a horizontally scaled service, a safer operational model is:

- deployment pipeline runs migrations once;
- application startup validates the schema version;
- application startup can optionally run migrations only in environments where coordinated execution is guaranteed.

The educational implementation can support both modes, but the production recommendation should distinguish them.

## 7. Verification Criteria

1. Fresh database reaches the latest version.
2. Old fixture at version N upgrades to N+K.
3. Re-running the runner is a no-op.
4. A migration failure rolls back its transactional work when supported.
5. Missing migration versions are detected.
6. Mutating an already-applied migration is detectable when checksums are enabled.
7. Seed data does not multiply across repeated deployments.
8. Concurrent migration attempts are either serialized or rejected by policy.

## 8. Gate to Phase 7

Schema is now deterministic. The next phase can build reusable query and mapping primitives against stable table contracts.

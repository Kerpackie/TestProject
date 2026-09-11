# Production-Grade C++ Database Layer — Master Roadmap

## Status

This documentation set is the source-of-truth blueprint for an educational, production-oriented C++ database layer built from a fresh CLion/CMake project.

The roadmap intentionally starts with SQLite + SQLiteCpp and then moves the abstraction boundary outward toward PostgreSQL/MariaDB, concurrency, schema evolution, query mapping, and observability.

## Design North Star

The database layer should:

- make invalid states difficult to represent;
- own resources through RAII;
- make transaction boundaries explicit;
- keep SQL/database-driver concerns out of domain code;
- support deterministic tests without requiring a live server for every unit test;
- preserve enough database-specific power to avoid designing a lowest-common-denominator interface;
- be explicit about thread/connection ownership;
- treat configuration, migrations, logging, and performance as first-class production concerns.

## Phase Gates

| Phase | Deliverable | Exit gate |
|---|---|---|
| 1 | CMake + SQLiteCpp dependency graph | Clean configure/build/test from a fresh checkout |
| 2 | Database lifecycle service | Connections/transactions are exception-safe and configurable |
| 3 | Repository + manual DI | Domain/application code no longer depends on SQLiteCpp |
| 4 | Multi-engine abstraction | PostgreSQL/MariaDB adapters can satisfy shared ports without leaking driver APIs |
| 5 | Concurrency model | Multi-worker workload behaves predictably under contention |
| 6 | Migration runner | Fresh and upgraded databases converge on the same schema |
| 7 | Query/mapping layer | Parameterized queries and row-to-struct mapping are consistent and tested |
| 8 | Diagnostics/profiling | Slow/failing queries produce actionable telemetry without leaking secrets |
| 9 | Production hardening | Audit context, automated interceptors, and transparent soft delete filtering |

## Non-Goals

This series does not attempt to build a full ORM. It also does not promise that SQLite, PostgreSQL, and MariaDB have identical semantics. The abstraction is deliberately placed at the application boundary rather than pretending the engines are interchangeable in every feature.

## Recommended Baseline

- C++: C++20 as the primary teaching target; code should remain broadly C++17-compatible where practical.
- Build: CMake 3.28+ for the tutorial baseline, while avoiding unnecessary bleeding-edge features.
- Dependency ingestion: `FetchContent_MakeAvailable()` for source-based reproducibility.
- Tests: CTest + a unit-test framework introduced in a dedicated test target.
- SQLite: SQLiteCpp 3.3.3 with a pinned revision/tag, not a floating branch.
- Errors: exceptions at infrastructure boundaries, with domain-specific error translation at architectural boundaries.
- Ownership: values and stack objects by default; `std::unique_ptr` for exclusive polymorphic ownership; `std::shared_ptr` only where shared lifetime is genuinely required.

## Dependency Reproducibility Rule

Every external dependency used by the project should be pinned to a known revision and reviewed before upgrading. A production tutorial must never rely on “whatever is on the reader's machine”.

## Source Verification Used While Writing

- SQLiteCpp repository/CMake configuration: <https://github.com/SRombauts/SQLiteCpp>
- CMake FetchContent documentation: <https://cmake.org/cmake/help/latest/module/FetchContent.html>
- CMake dependency guide: <https://cmake.org/cmake/help/latest/guide/using-dependencies/index.html>
- SQLite WAL documentation: <https://www.sqlite.org/wal.html>
- SQLite busy-timeout documentation: <https://sqlite.org/c3ref/busy_timeout.html>
- SQLite result codes: <https://www.sqlite.org/rescode.html>
- PostgreSQL client interfaces: <https://www.postgresql.org/docs/current/external-interfaces.html>
- libpqxx documentation: <https://libpqxx.readthedocs.io/latest/>
- MariaDB Connector/C++: <https://mariadb.com/downloads/enterprise/enterprise-manager/>

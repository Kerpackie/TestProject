# Phase 5 — Concurrency & Thread Safety: WAL, Busy Timeouts & Worker Handling

## 1. The Pain Point

“SQLite is thread-safe” is not enough of a production design. Thread safety depends on compile/runtime modes, connection ownership, transaction duration, and workload shape. Even WAL does not eliminate every possible `SQLITE_BUSY` condition. SQLite's own documentation says WAL allows readers and writers to proceed concurrently in the usual case, while still documenting cases where `SQLITE_BUSY` can occur. citeturn721891search3

Common failure modes:

- one global database object shared by every worker;
- one mutex around all database work, eliminating useful concurrency;
- long transactions holding locks while performing non-database work;
- unlimited retries that turn contention into latency explosions;
- worker threads creating connections inconsistently;
- accidental cross-thread use of connection-bound objects.

## 2. Objective & Architecture

Adopt a simple rule: **a connection belongs to one execution context at a time** unless the chosen driver and design explicitly prove safe sharing.

For SQLite:

- enable WAL for file-backed databases;
- set a finite busy timeout;
- keep write transactions short;
- use a small number of connections rather than a single global connection;
- prefer one connection per worker when the workload is simple and bounded;
- serialize writes at the application level when that is cheaper and more predictable than allowing uncontrolled write contention.

```text
Worker 1 ---> Connection 1 ---+
Worker 2 ---> Connection 2 ---+--> same SQLite file (WAL)
Worker 3 ---> Connection 3 ---+
```

## 3. Technical Specifications

- Use `std::jthread` + cooperative stop where C++20 is available.
- Avoid detached threads.
- Treat database connections as non-copyable resources.
- Make retry policies bounded and observable.
- Do not retry permanent constraint errors.
- Keep transactions small and avoid network/filesystem work inside a write transaction.
- Use `BEGIN IMMEDIATE` selectively when early lock acquisition produces more deterministic behavior for a write transaction; SQLite documents this as a way to avoid encountering `SQLITE_BUSY` later in the same transaction when the initial begin succeeds. citeturn721891search6

## 4. File Structure

```text
include/database/
├── ConcurrencyPolicy.h
├── ConnectionPool.h
└── RetryPolicy.h
src/database/
├── ConnectionPool.cpp
└── RetryPolicy.cpp
tests/
├── sqlite_concurrency_test.cpp
└── connection_pool_test.cpp
```

## 5. Draft Code

### SQLite concurrency initialization

```cpp
void configure_sqlite_for_file_database(SQLite::Database& db,
                                        std::chrono::milliseconds busy_timeout) {
    // WAL is persistent for the database file after it is successfully enabled.
    // Execute this during controlled initialization rather than from random
    // request paths.
    db.exec("PRAGMA journal_mode = WAL");

    // Bound how long SQLite waits for a conflicting lock.
    db.exec("PRAGMA busy_timeout = " +
            std::to_string(busy_timeout.count()));

    db.exec("PRAGMA synchronous = NORMAL");

    // Foreign keys are connection-scoped, so apply them for every connection.
    db.exec("PRAGMA foreign_keys = ON");
}
```

SQLite documents `busy_timeout` as a per-connection busy handler and WAL as the mode in which readers generally do not block writers and vice versa. citeturn721891search0turn721891search3

### Retry policy

```cpp
class RetryPolicy final {
public:
    explicit RetryPolicy(int attempts,
                         std::chrono::milliseconds base_delay)
        : attempts_(attempts), base_delay_(base_delay) {}

    template <typename Fn>
    decltype(auto) run(Fn&& operation) const {
        for (int attempt = 1; ; ++attempt) {
            try {
                return operation();
            } catch (const SQLite::Exception& ex) {
                // Production code must classify the actual SQLite error code
                // here. Retry only transient BUSY/LOCKED-like conditions;
                // never turn constraint or syntax errors into repeated work.
                if (!is_transient_lock_error(ex) || attempt >= attempts_) {
                    throw;
                }

                std::this_thread::sleep_for(base_delay_ * attempt);
            }
        }
    }

private:
    static bool is_transient_lock_error(const SQLite::Exception& ex) noexcept;

    int attempts_;
    std::chrono::milliseconds base_delay_;
};
```

Educational note: the actual production implementation should classify errors (busy/locked/transient versus constraint/syntax/corruption) before retrying. The key principle is **bounded retries plus observability**, not “catch everything and sleep”.

### Worker ownership

```cpp
void worker_loop(std::stop_token stop, DatabaseConfig config) {
    // The connection is constructed inside the worker. No connection object is
    // transferred across worker boundaries, making ownership easy to reason about.
    DatabaseConnection connection(config);

    while (!stop.stop_requested()) {
        // Dequeue work from an application queue.
        // Keep DB transactions short and do CPU/network work outside them.
    }
}
```

### Small connection pool

```cpp
class ConnectionPool final {
public:
    using Factory = std::function<std::unique_ptr<DatabaseConnection>()>;

    // A production pool needs a bounded queue, condition_variable, shutdown
    // semantics, and metrics. The educational point is that ownership is
    // exclusive: checkout gives one worker one connection at a time.
};
```

## 6. Concurrency Test Matrix

| Scenario | Expected behavior |
|---|---|
| many concurrent reads | high overlap, no data corruption |
| one writer + many readers in WAL | readers continue in normal WAL cases |
| competing writers | bounded waits/retries, not hangs |
| failed transaction | rollback, connection remains usable |
| worker shutdown | no detached DB activity after shutdown begins |
| long-running read + write | documented behavior; no indefinite application-level wait |

## 7. Verification Criteria

1. Run 8–32 worker threads repeatedly against the same file-backed SQLite DB.
2. Assert row counts and invariants, not only absence of crashes.
3. Measure retry counts and latency percentiles.
4. Kill workers during transactions and verify database integrity on restart.
5. Verify that no thread accesses a connection after its owning worker exits.
6. Prove that the system fails fast after its retry budget is exhausted.

## 8. Gate to Phase 6

The concurrency layer must be stable before migrations are introduced, because migrations are writes that may happen during startup, deployment, test setup, or rolling upgrades. The next phase defines migration serialization and schema convergence.

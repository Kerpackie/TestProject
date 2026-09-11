# Phase 8 — Logging, Diagnostics & Performance Profiling

## 1. The Pain Point

A database layer without observability produces the worst possible production incident message: “the endpoint is slow.”

Without structured diagnostics, the team cannot answer:

- which query was slow;
- whether time was spent waiting for a connection or inside SQLite/PostgreSQL/MariaDB;
- whether contention is increasing;
- whether errors are transient or permanent;
- which migration caused a schema regression;
- how many rows were returned or changed.

Logging every raw SQL statement with values is also dangerous because secrets and personal data can leak into logs.

## 2. Objective & Architecture

Instrument the database boundary with structured events:

```text
Repository
   |
   v
Database executor -----> Timer / Metrics / Logger
   |
   +--> query fingerprint
   +--> elapsed time
   +--> rows affected
   +--> outcome/error class
   +--> retry count
   +--> transaction id/correlation id
```

The executor should measure without changing repository semantics.

## 3. Technical Specifications

- Use `std::chrono::steady_clock` for elapsed-duration measurements.
- Log query fingerprints/templates, not arbitrary raw values.
- Make slow-query thresholds configurable.
- Never log credentials, connection strings, tokens, or bound secrets.
- Use correlation/request IDs where available.
- Classify errors into retryable/non-retryable categories.
- Track counters and histograms rather than only human-readable log lines.
- Measure connection checkout wait separately from SQL execution time.
- Provide a way to disable expensive tracing in high-throughput environments without recompiling.

## 4. File Structure

```text
include/observability/
├── DatabaseTelemetry.h
├── QueryFingerprint.h
└── DatabaseLogger.h
src/observability/
├── DatabaseTelemetry.cpp
├── QueryFingerprint.cpp
└── DatabaseLogger.cpp
include/database/
└── InstrumentedExecutor.h
tests/
└── database_telemetry_test.cpp
```

## 5. Draft Code

### Telemetry event

```cpp
struct DatabaseEvent final {
    std::string operation;        // e.g. "UserRepository.find_by_id"
    std::string statement_name;   // stable identifier, not full SQL with values
    std::chrono::microseconds elapsed{};
    std::size_t rows_returned{};
    std::size_t rows_affected{};
    int retry_count{};
    std::string outcome;          // "success", "timeout", "conflict", etc.
};
```

### Timing wrapper

```cpp
DatabaseEvent execute_instrumented(
    std::string statement_name,
    std::string operation,
    const std::function<std::size_t()>& execute) {

    const auto start = std::chrono::steady_clock::now();

    try {
        const std::size_t rows = execute();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start);

        return DatabaseEvent{
            .operation = std::move(operation),
            .statement_name = std::move(statement_name),
            .elapsed = elapsed,
            .rows_returned = rows,
            .outcome = "success",
        };
    } catch (...) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start);

        // Preserve the original exception. Observability must never replace
        // application failure with logging failure.
        log_database_failure(statement_name, elapsed);
        throw;
    }
}
```

## 6. Slow-Query Policy

Example policy:

```text
< 20 ms    normal
20–100 ms  monitor
> 100 ms   slow-query event
> 1 s      incident-worthy depending on operation
```

Do not copy these thresholds blindly. Establish them from service SLOs and percentile distributions.

## 7. Profiling Strategy

### Stage 1: query-level timing

Measure every database operation with a low-overhead monotonic clock.

### Stage 2: statement fingerprints

Normalize equivalent SQL templates into a stable identifier so metrics aggregate correctly.

### Stage 3: database-level inspection

Use engine-native tools for deeper investigations:

- SQLite query plans and indexes;
- PostgreSQL `EXPLAIN (ANALYZE, BUFFERS)` in controlled environments;
- MariaDB execution plans and server metrics.

### Stage 4: load testing

Run repeatable workloads and compare p50/p95/p99 latency, throughput, error rate, retries, and lock-wait behavior before and after an optimization.

## 8. Verification Criteria

1. Every database call emits a success/failure metric.
2. Slow statements are identified by stable name/fingerprint.
3. Sensitive values never appear in logs.
4. Connection checkout wait can be distinguished from execution time.
5. Retry counts are visible.
6. A performance regression test can fail CI when latency exceeds a deliberately chosen threshold in a controlled benchmark environment.
7. Diagnostics remain functional when an exception is thrown.

## 9. Final System Acceptance Gate

The database layer is production-ready for this educational series when a reader can:

1. clone the project;
2. configure and build from scratch;
3. run migrations against a clean database;
4. execute repository operations through an application-facing port;
5. switch database engines through the composition root;
6. run concurrent workloads without undefined ownership rules;
7. reproduce and diagnose a failed/slow query from structured telemetry;
8. run the full automated test suite in CI.

That is the point at which the implementation is no longer a collection of database examples and has become an architectural database subsystem.

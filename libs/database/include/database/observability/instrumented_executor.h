#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "database/observability/database_telemetry.h"
#include "database/observability/query_fingerprint.h"

namespace database::observability {

class InstrumentedExecutor final {
 public:
  explicit InstrumentedExecutor(std::chrono::microseconds slow_threshold = std::chrono::microseconds(20000))
      : slow_threshold_(slow_threshold) {}

  template <typename Fn>
  DatabaseEvent execute(const std::string& operation, const std::string& raw_sql, Fn&& action) const {
    const auto start = std::chrono::steady_clock::now();
    std::string fingerprint = QueryFingerprint::sanitize_and_fingerprint(raw_sql);

    DatabaseEvent event{
        .operation = operation,
        .statement_fingerprint = fingerprint,
        .elapsed = std::chrono::microseconds(0),
        .rows_affected = 0,
        .retry_count = 0,
        .outcome = "success",
        .category = QueryCategory::Normal,
    };

    try {
      if constexpr (std::is_same_v<decltype(action()), void>) {
        action();
      } else {
        event.rows_affected = static_cast<std::size_t>(action());
      }

      const auto end = std::chrono::steady_clock::now();
      event.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      event.category = DatabaseTelemetry::classify_duration(event.elapsed, slow_threshold_);
      return event;
    } catch (...) {
      const auto end = std::chrono::steady_clock::now();
      event.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      event.outcome = "failure";
      event.category = DatabaseTelemetry::classify_duration(event.elapsed, slow_threshold_);
      throw; // Preserve original exception semantics
    }
  }

 private:
  std::chrono::microseconds slow_threshold_;
};

}  // namespace database::observability

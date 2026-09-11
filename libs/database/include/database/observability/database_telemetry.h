#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace database::observability {

enum class QueryCategory {
  Normal,
  Monitor,
  SlowQuery,
  Critical
};

struct DatabaseEvent final {
  std::string operation;
  std::string statement_fingerprint;
  std::chrono::microseconds elapsed{0};
  std::size_t rows_affected{0};
  int retry_count{0};
  std::string outcome;
  QueryCategory category{QueryCategory::Normal};
};

class DatabaseTelemetry final {
 public:
  static QueryCategory classify_duration(std::chrono::microseconds elapsed,
                                          std::chrono::microseconds slow_threshold = std::chrono::microseconds(20000)) {
    if (elapsed > slow_threshold * 5) {
      return QueryCategory::Critical;
    } else if (elapsed > slow_threshold) {
      return QueryCategory::SlowQuery;
    } else if (elapsed > slow_threshold / 2) {
      return QueryCategory::Monitor;
    }
    return QueryCategory::Normal;
  }

  static std::string category_to_string(QueryCategory category) {
    switch (category) {
      case QueryCategory::Normal: return "NORMAL";
      case QueryCategory::Monitor: return "MONITOR";
      case QueryCategory::SlowQuery: return "SLOW_QUERY";
      case QueryCategory::Critical: return "CRITICAL_SLOW";
    }
    return "UNKNOWN";
  }
};

}  // namespace database::observability

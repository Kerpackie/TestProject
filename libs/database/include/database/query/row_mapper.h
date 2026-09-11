#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace database::query {

template <typename T>
struct ColumnValue {
  static T get(class RowMapper& mapper, std::int64_t col_index);
};

class RowMapper final {
 public:
  template <typename Fn>
  static auto map_one(Connection& conn, const std::string& sql, Fn&& mapper_fn)
      -> std::optional<decltype(mapper_fn(nullptr))> {
    // Helper mapper execution
    return std::nullopt;
  }
};

}  // namespace database::query

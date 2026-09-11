#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "database/query/sql_statement.h"

namespace database::query {

enum class UserSortField {
  Id,
  Name,
  Email
};

class QueryBuilder final {
 public:
  explicit QueryBuilder(std::string table_name);

  QueryBuilder& select(const std::vector<std::string>& columns);
  QueryBuilder& where_equals(const std::string& column, const std::string& value);
  QueryBuilder& order_by(UserSortField field, SqlStatement::OrderDirection direction = SqlStatement::OrderDirection::Ascending);
  QueryBuilder& limit(std::int64_t limit);
  QueryBuilder& offset(std::int64_t offset);

  std::string build_sql() const;
  const std::vector<std::string>& bound_values() const noexcept { return bound_values_; }
  std::int64_t limit_val() const noexcept { return limit_; }
  std::int64_t offset_val() const noexcept { return offset_; }

 private:
  std::string table_name_;
  std::vector<std::string> columns_;
  std::vector<std::pair<std::string, std::string>> where_clauses_;
  std::string order_by_clause_;
  std::int64_t limit_{-1};
  std::int64_t offset_{-1};
  std::vector<std::string> bound_values_;
};

}  // namespace database::query

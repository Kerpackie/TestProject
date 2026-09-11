#include "database/query/query_builder.h"

#include <sstream>
#include <stdexcept>

namespace database::query {

QueryBuilder::QueryBuilder(std::string table_name)
    : table_name_(std::move(table_name)) {}

QueryBuilder& QueryBuilder::select(const std::vector<std::string>& columns) {
  columns_ = columns;
  return *this;
}

QueryBuilder& QueryBuilder::where_equals(const std::string& column, const std::string& value) {
  where_clauses_.emplace_back(column, value);
  bound_values_.push_back(value);
  return *this;
}

QueryBuilder& QueryBuilder::order_by(UserSortField field, SqlStatement::OrderDirection direction) {
  std::string field_str;
  switch (field) {
    case UserSortField::Id: field_str = "id"; break;
    case UserSortField::Name: field_str = "name"; break;
    case UserSortField::Email: field_str = "email"; break;
  }
  order_by_clause_ = field_str + " " + SqlStatement::to_order_string(direction);
  return *this;
}

QueryBuilder& QueryBuilder::limit(std::int64_t limit) {
  limit_ = limit;
  return *this;
}

QueryBuilder& QueryBuilder::offset(std::int64_t offset) {
  offset_ = offset;
  return *this;
}

std::string QueryBuilder::build_sql() const {
  std::ostringstream sql;
  sql << "SELECT ";
  if (columns_.empty()) {
    sql << "*";
  } else {
    for (std::size_t i = 0; i < columns_.size(); ++i) {
      if (i > 0) sql << ", ";
      sql << columns_[i];
    }
  }

  sql << " FROM " << table_name_;

  if (!where_clauses_.empty()) {
    sql << " WHERE ";
    for (std::size_t i = 0; i < where_clauses_.size(); ++i) {
      if (i > 0) sql << " AND ";
      sql << where_clauses_[i].first << " = ?";
    }
  }

  if (!order_by_clause_.empty()) {
    sql << " ORDER BY " << order_by_clause_;
  }

  if (limit_ >= 0) {
    sql << " LIMIT " << limit_;
  }

  if (offset_ >= 0) {
    sql << " OFFSET " << offset_;
  }

  return sql.str();
}

}  // namespace database::query

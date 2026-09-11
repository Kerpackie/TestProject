#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace database::query {

class SqlStatement final {
 public:
  enum class OrderDirection {
    Ascending,
    Descending
  };

  static std::string to_order_string(OrderDirection dir) {
    switch (dir) {
      case OrderDirection::Ascending: return "ASC";
      case OrderDirection::Descending: return "DESC";
    }
    return "ASC";
  }
};

}  // namespace database::query

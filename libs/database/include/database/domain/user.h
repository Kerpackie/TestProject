#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace database::domain {

struct User final {
  std::int64_t id{0};
  std::string name;
  std::string email;
  std::string created_at;
  std::string updated_at;
  std::string created_by;
  std::string updated_by;
  std::optional<std::string> deleted_at;
};

}  // namespace database::domain

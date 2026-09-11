#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace database {
class Connection;
}

namespace database::migration {

struct Migration final {
  std::int64_t version{0};
  std::string name;
  std::string checksum;
  std::function<void(Connection&)> apply;
};

struct AppliedMigration final {
  std::int64_t version{0};
  std::string name;
  std::string checksum;
  std::string applied_at;
};

}  // namespace database::migration

#pragma once

#include <chrono>
#include <filesystem>
#include <string>

namespace database {

struct DatabaseConfig final {
  std::filesystem::path database_path{":memory:"};
  int open_flags{6};  // Default: OPEN_READWRITE (2) | OPEN_CREATE (4)
  std::chrono::milliseconds busy_timeout{5000};
  bool foreign_keys{true};
  bool wal_mode{false};
};

}  // namespace database

#pragma once

#include <cstdint>
#include <string>

namespace database::domain {

struct User final {
  std::int64_t id{0};
  std::string name;
  std::string email;
};

}  // namespace database::domain

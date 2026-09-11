#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "database/domain/user.h"

namespace database::repository {

class IUserRepository {
 public:
  virtual ~IUserRepository() = default;

  virtual std::optional<domain::User> find_by_id(std::int64_t id) = 0;
  virtual std::optional<domain::User> find_by_email(const std::string& email) = 0;
  virtual std::int64_t create(const domain::User& user) = 0;
  virtual bool update(const domain::User& user) = 0;
  virtual bool delete_by_id(std::int64_t id) = 0;
  virtual std::vector<domain::User> find_all() = 0;
};

}  // namespace database::repository

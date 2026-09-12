#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

#include "database/domain/user.h"
#include "database/repository/user_repository.h"

namespace database::service {

class UserAlreadyExistsException : public std::runtime_error {
 public:
  explicit UserAlreadyExistsException(const std::string& email)
      : std::runtime_error("User with email '" + email + "' already exists") {}
};

class UserService final {
 public:
  explicit UserService(repository::IUserRepository& repository);

  domain::User register_user(const std::string& name, const std::string& email);
  bool update_user(std::int64_t id, const std::string& name, const std::string& email);
  std::optional<domain::User> get_user_by_id(std::int64_t id);
  std::optional<domain::User> get_user_by_email(const std::string& email);
  std::vector<domain::User> list_users();
  bool remove_user(std::int64_t id);

 private:
  repository::IUserRepository& repository_;
};

}  // namespace database::service

#include "database/service/user_service.h"

namespace database::service {

UserService::UserService(repository::IUserRepository& repository)
    : repository_(repository) {}

domain::User UserService::register_user(const std::string& name, const std::string& email) {
  if (repository_.find_by_email(email).has_value()) {
    throw UserAlreadyExistsException(email);
  }

  domain::User new_user{
      .id = 0,
      .name = name,
      .email = email,
  };

  const std::int64_t new_id = repository_.create(new_user);
  new_user.id = new_id;
  return new_user;
}

std::optional<domain::User> UserService::get_user_by_id(std::int64_t id) {
  return repository_.find_by_id(id);
}

std::optional<domain::User> UserService::get_user_by_email(const std::string& email) {
  return repository_.find_by_email(email);
}

std::vector<domain::User> UserService::list_users() {
  return repository_.find_all();
}

bool UserService::remove_user(std::int64_t id) {
  return repository_.delete_by_id(id);
}

}  // namespace database::service

#pragma once

#include "database/database.h"
#include "database/repository/user_repository.h"

namespace database::repository {

class SqliteUserRepository final : public IUserRepository {
 public:
  explicit SqliteUserRepository(Connection& connection);

  std::optional<domain::User> find_by_id(std::int64_t id) override;
  std::optional<domain::User> find_by_email(const std::string& email) override;
  std::int64_t create(const domain::User& user) override;
  bool update(const domain::User& user) override;
  bool delete_by_id(std::int64_t id) override;
  std::vector<domain::User> find_all() override;

  void init_schema();

 private:
  Connection& connection_;
};

}  // namespace database::repository

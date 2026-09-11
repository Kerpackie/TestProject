#pragma once

#include <memory>
#include <string>

namespace database {

class Connection {
 public:
  explicit Connection(const std::string& db_name = ":memory:");
  ~Connection();

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  Connection(Connection&&) noexcept;
  Connection& operator=(Connection&&) noexcept;

  void execute(const std::string& sql);
  int execute_scalar_int(const std::string& sql);
  bool is_open() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

std::string version();

}  // namespace database

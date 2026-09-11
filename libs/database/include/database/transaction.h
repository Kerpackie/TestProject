#pragma once

#include <memory>

namespace database {

class Connection;

class Transaction final {
 public:
  explicit Transaction(Connection& connection);
  ~Transaction();

  Transaction(const Transaction&) = delete;
  Transaction& operator=(const Transaction&) = delete;

  Transaction(Transaction&&) noexcept;
  Transaction& operator=(Transaction&&) noexcept;

  void commit();
  bool is_committed() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace database

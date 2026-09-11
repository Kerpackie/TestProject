#include "database/transaction.h"

#include <SQLiteCpp/Transaction.h>

#include "database/database.h"
#include "detail.h"

namespace database {

struct Transaction::Impl {
  SQLite::Transaction tx;
  bool committed{false};

  explicit Impl(Connection& conn)
      : tx(detail::get_native_db(conn)) {}
};

Transaction::Transaction(Connection& connection)
    : impl_(std::make_unique<Impl>(connection)) {}

Transaction::~Transaction() = default;

Transaction::Transaction(Transaction&&) noexcept = default;
Transaction& Transaction::operator=(Transaction&&) noexcept = default;

void Transaction::commit() {
  if (impl_ && !impl_->committed) {
    impl_->tx.commit();
    impl_->committed = true;
  }
}

bool Transaction::is_committed() const noexcept {
  return impl_ ? impl_->committed : false;
}

}  // namespace database

#include "database/concurrency/connection_pool.h"

namespace database::concurrency {

ConnectionPool::ConnectionPool(DatabaseConfig config, std::size_t pool_size)
    : config_(config), pool_size_(pool_size) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (std::size_t i = 0; i < pool_size_; ++i) {
    pool_.push(std::make_unique<Connection>(config_));
  }
}

ConnectionPool::~ConnectionPool() {
  std::lock_guard<std::mutex> lock(mutex_);
  shutdown_ = true;
  while (!pool_.empty()) {
    pool_.pop();
  }
  cv_.notify_all();
}

ConnectionPool::PooledConnection ConnectionPool::acquire(std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!cv_.wait_for(lock, timeout, [this] { return !pool_.empty() || shutdown_; })) {
    throw std::runtime_error("ConnectionPool acquire timeout: no connections available");
  }

  if (shutdown_) {
    throw std::runtime_error("ConnectionPool is shut down");
  }

  auto conn = std::move(pool_.front());
  pool_.pop();
  return PooledConnection(std::move(conn), *this);
}

void ConnectionPool::return_connection(std::unique_ptr<Connection> conn) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!shutdown_ && conn) {
    pool_.push(std::move(conn));
    cv_.notify_one();
  }
}

std::size_t ConnectionPool::size() const noexcept {
  return pool_size_;
}

std::size_t ConnectionPool::available() const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  return pool_.size();
}

}  // namespace database::concurrency

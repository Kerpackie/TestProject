#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>

#include "database/database.h"
#include "database/database_config.h"

namespace database::concurrency {

class ConnectionPool final {
 public:
  class PooledConnection final {
   public:
    PooledConnection(std::unique_ptr<Connection> conn, ConnectionPool& pool)
        : conn_(std::move(conn)), pool_(&pool) {}

    ~PooledConnection() {
      if (conn_ && pool_) {
        pool_->return_connection(std::move(conn_));
      }
    }

    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;

    PooledConnection(PooledConnection&& other) noexcept
        : conn_(std::move(other.conn_)), pool_(other.pool_) {
      other.pool_ = nullptr;
    }

    PooledConnection& operator=(PooledConnection&& other) noexcept {
      if (this != &other) {
        if (conn_ && pool_) {
          pool_->return_connection(std::move(conn_));
        }
        conn_ = std::move(other.conn_);
        pool_ = other.pool_;
        other.pool_ = nullptr;
      }
      return *this;
    }

    Connection& get() noexcept { return *conn_; }
    Connection* operator->() noexcept { return conn_.get(); }

   private:
    std::unique_ptr<Connection> conn_;
    ConnectionPool* pool_{nullptr};
  };

  explicit ConnectionPool(DatabaseConfig config = DatabaseConfig{},
                         std::size_t pool_size = 4);
  ~ConnectionPool();

  ConnectionPool(const ConnectionPool&) = delete;
  ConnectionPool& operator=(const ConnectionPool&) = delete;

  PooledConnection acquire(std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));
  std::size_t size() const noexcept;
  std::size_t available() const noexcept;

 private:
  friend class PooledConnection;
  void return_connection(std::unique_ptr<Connection> conn);

  DatabaseConfig config_;
  std::size_t pool_size_;
  std::queue<std::unique_ptr<Connection>> pool_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  bool shutdown_{false};
};

}  // namespace database::concurrency

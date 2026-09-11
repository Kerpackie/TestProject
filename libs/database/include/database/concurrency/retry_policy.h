#pragma once

#include <chrono>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>

#include "database/error/database_error.h"

namespace database::concurrency {

struct RetryStats final {
  int attempts{0};
  int retries{0};
  bool succeeded{false};
};

class RetryPolicy final {
 public:
  explicit RetryPolicy(int max_attempts = 3,
                       std::chrono::milliseconds base_delay = std::chrono::milliseconds(10))
      : max_attempts_(max_attempts), base_delay_(base_delay) {}

  template <typename Fn>
  decltype(auto) execute(Fn&& operation, RetryStats* stats_out = nullptr) const {
    RetryStats local_stats{};
    for (int attempt = 1; ; ++attempt) {
      local_stats.attempts = attempt;
      try {
        if constexpr (std::is_same_v<decltype(operation()), void>) {
          operation();
          local_stats.succeeded = true;
          if (stats_out) *stats_out = local_stats;
          return;
        } else {
          auto result = operation();
          local_stats.succeeded = true;
          if (stats_out) *stats_out = local_stats;
          return result;
        }
      } catch (const error::TransientException& ex) {
        if (attempt >= max_attempts_) {
          if (stats_out) *stats_out = local_stats;
          throw;
        }
        local_stats.retries++;
        std::this_thread::sleep_for(base_delay_ * attempt);
      } catch (const error::ConflictException& ex) {
        if (stats_out) *stats_out = local_stats;
        throw;
      } catch (const std::exception& ex) {
        std::string msg(ex.what());
        if ((msg.find("busy") != std::string::npos || msg.find("locked") != std::string::npos) && attempt < max_attempts_) {
          local_stats.retries++;
          std::this_thread::sleep_for(base_delay_ * attempt);
        } else {
          if (stats_out) *stats_out = local_stats;
          throw;
        }
      }
    }
  }

  int max_attempts() const noexcept { return max_attempts_; }
  std::chrono::milliseconds base_delay() const noexcept { return base_delay_; }

 private:
  int max_attempts_;
  std::chrono::milliseconds base_delay_;
};

}  // namespace database::concurrency

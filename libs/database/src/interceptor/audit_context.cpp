#include "database/interceptor/audit_context.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace database::interceptor {

namespace {
thread_local AuditContextData g_current_context{};
}

void AuditContext::set_current(AuditContextData data) {
  g_current_context = std::move(data);
}

const AuditContextData& AuditContext::current() {
  return g_current_context;
}

void AuditContext::clear() {
  g_current_context = AuditContextData{};
}

std::string AuditContext::current_iso_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

}  // namespace database::interceptor

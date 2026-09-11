#pragma once

#include <cstdint>
#include <string>

namespace database::interceptor {

struct AuditContextData final {
  std::string operator_id{"system"};
  std::string tenant_id{"default"};
};

class AuditContext final {
 public:
  static void set_current(AuditContextData data);
  static const AuditContextData& current();
  static void clear();
  static std::string current_iso_timestamp();
};

}  // namespace database::interceptor

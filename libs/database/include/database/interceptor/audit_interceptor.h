#pragma once

#include <optional>
#include <string>

#include "database/interceptor/audit_context.h"

namespace database::interceptor {

struct AuditRecord {
  std::string created_at;
  std::string updated_at;
  std::string created_by;
  std::string updated_by;
  std::optional<std::string> deleted_at;
};

class AuditInterceptor final {
 public:
  static void prepare_for_insert(AuditRecord& record, const AuditContextData& ctx = AuditContext::current());
  static void prepare_for_update(AuditRecord& record, const AuditContextData& ctx = AuditContext::current());
  static void prepare_for_soft_delete(AuditRecord& record, const AuditContextData& ctx = AuditContext::current());
};

}  // namespace database::interceptor

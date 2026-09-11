#include "database/interceptor/audit_interceptor.h"

namespace database::interceptor {

void AuditInterceptor::prepare_for_insert(AuditRecord& record, const AuditContextData& ctx) {
  std::string now = AuditContext::current_iso_timestamp();
  record.created_at = now;
  record.updated_at = now;
  record.created_by = ctx.operator_id;
  record.updated_by = ctx.operator_id;
  record.deleted_at = std::nullopt;
}

void AuditInterceptor::prepare_for_update(AuditRecord& record, const AuditContextData& ctx) {
  record.updated_at = AuditContext::current_iso_timestamp();
  record.updated_by = ctx.operator_id;
}

void AuditInterceptor::prepare_for_soft_delete(AuditRecord& record, const AuditContextData&) {
  record.deleted_at = AuditContext::current_iso_timestamp();
}

}  // namespace database::interceptor

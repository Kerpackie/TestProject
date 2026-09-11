# Phase 9 — Production Hardening: Audit Trail Interceptors & Soft Deletes

## 1. The Pain Point

In production applications, data modification needs transparency and compliance:

- Direct `DELETE` statements permanently purge records, preventing recovery and breaking historical foreign key references.
- `created_at`, `updated_at`, `created_by`, and `updated_by` fields are manually set in individual repository methods, leading to forgotten or inconsistent audit timestamps.
- Soft-deleted records (`deleted_at IS NOT NULL`) are accidentally included in normal query results unless every repository query remembers to append `WHERE deleted_at IS NULL`.
- Multi-tenant applications risk data leaks if tenant boundaries are not automatically enforced across queries.

## 2. Objective & Architecture

Introduce a centralized interceptor and soft delete model:

1. **`AuditContext`**: Thread-local or session-scoped context holding active operator identity (`user_id`, `tenant_id`).
2. **`AuditInterceptor`**: Middleware that automatically populates audit timestamps (`created_at`, `updated_at`) and operator metadata (`created_by`, `updated_by`) on inserts and updates.
3. **`SoftDeleteRepository` / Interceptor**: Intercepts `delete` requests to convert hard SQL `DELETE` operations into soft update flags (`deleted_at = CURRENT_TIMESTAMP`), and automatically injects `deleted_at IS NULL` filters into query operations.

```text
Repository / Service
       |
       v
Audit & Soft-Delete Interceptor  <---> AuditContext (user_id, tenant_id)
       |
       +--> Auto-populates: created_at, updated_at, created_by
       +--> Converts DELETE -> UPDATE table SET deleted_at = CURRENT_TIMESTAMP
       +--> Injects WHERE deleted_at IS NULL into queries
       |
       v
Database Session / Connection
```

## 3. Technical Specifications

- `AuditContext` must be scoped per operation/request.
- `SoftDelete` must preserve soft-deleted records while making them invisible to standard repository reads.
- Provide a way to perform explicit hard deletes when administrative purge is required.
- Provide an `include_deleted` option for admin/audit queries.
- Keep interceptor logic completely decoupled from specific database drivers.

## 4. File Structure

```text
include/database/interceptor/
├── audit_context.h
├── audit_interceptor.h
└── soft_delete_repository.h
src/database/interceptor/
├── audit_context.cpp
└── audit_interceptor.cpp
tests/
└── audit_soft_delete_test.cpp
```

## 5. Verification Criteria

1. Inserts automatically populate `created_at` and `created_by` without manual repository code.
2. Updates automatically update `updated_at` and `updated_by`.
3. Standard repository `delete_by_id()` sets `deleted_at` instead of removing the row.
4. Queries for active records ignore soft-deleted rows by default.
5. Admin queries can include or view soft-deleted records when requested.
6. Hard delete remains available through explicit administrative interfaces.

#pragma once

#include <stdexcept>
#include <string>

namespace database::error {

enum class EngineType {
  SQLite,
  PostgreSQL,
  MariaDB
};

inline std::string engine_type_to_string(EngineType engine) {
  switch (engine) {
    case EngineType::SQLite: return "SQLite";
    case EngineType::PostgreSQL: return "PostgreSQL";
    case EngineType::MariaDB: return "MariaDB";
  }
  return "Unknown";
}

class DatabaseException : public std::runtime_error {
 public:
  explicit DatabaseException(const std::string& message, EngineType engine = EngineType::SQLite)
      : std::runtime_error(message), engine_(engine) {}

  EngineType engine() const noexcept { return engine_; }

 private:
  EngineType engine_;
};

class NotFoundException : public DatabaseException {
 public:
  explicit NotFoundException(const std::string& message, EngineType engine = EngineType::SQLite)
      : DatabaseException("Not Found: " + message, engine) {}
};

class ConflictException : public DatabaseException {
 public:
  explicit ConflictException(const std::string& message, EngineType engine = EngineType::SQLite)
      : DatabaseException("Conflict (Unique Constraint Violation): " + message, engine) {}
};

class TransientException : public DatabaseException {
 public:
  explicit TransientException(const std::string& message, EngineType engine = EngineType::SQLite)
      : DatabaseException("Transient Error (Lock/Timeout): " + message, engine) {}
};

}  // namespace database::error

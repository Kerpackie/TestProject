#include "database/database_factory.h"

namespace database {

Connection DatabaseFactory::create(const DatabaseConfig& config) {
  return Connection(config);
}

}  // namespace database

#include "core/core.h"

#include "detail.h"

namespace core {

std::string version() { return detail::kVersion; }

}  // namespace core

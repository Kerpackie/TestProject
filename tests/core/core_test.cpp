#include <gtest/gtest.h>

#include "core/core.h"

TEST(CoreTest, VersionIsNotEmpty) {
  EXPECT_FALSE(core::version().empty());
}

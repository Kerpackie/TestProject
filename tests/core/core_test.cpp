#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

#include "core/app_config.h"
#include "core/core.h"

TEST(CoreTest, VersionIsNotEmpty) {
  EXPECT_FALSE(core::version().empty());
}

TEST(CoreTest, AppConfigCanPersistToRelativeSettingsFile) {
  const std::filesystem::path config_path = "./test_app_settings.json";
  std::filesystem::remove(config_path);

  setenv("APP_SETTINGS_PATH", config_path.string().c_str(), 1);
  setenv("APP_NAME", "Template Test", 1);
  setenv("APP_DB_PATH", "template_test.db", 1);
  setenv("APP_THEME", "light", 1);
  setenv("APP_AUTO_SAVE", "true", 1);

  core::AppConfig config{
      .app_name = "Template Test",
      .database_path = "template_test.db",
      .theme = "light",
      .auto_save = true,
      .reset_database = false,
      .settings_path = config_path.string(),
  };

  core::save_app_config(config);

  EXPECT_TRUE(std::filesystem::exists(config_path));

  const core::AppConfig loaded = core::load_app_config();
  EXPECT_EQ(loaded.app_name, "Template Test");
  EXPECT_EQ(loaded.database_path, "template_test.db");
  EXPECT_EQ(loaded.theme, "light");
  EXPECT_TRUE(loaded.auto_save);

  std::filesystem::remove(config_path);
  unsetenv("APP_SETTINGS_PATH");
  unsetenv("APP_NAME");
  unsetenv("APP_DB_PATH");
  unsetenv("APP_THEME");
  unsetenv("APP_AUTO_SAVE");
}

#pragma once

#include <string>

namespace core {

struct AppConfig final {
  std::string app_name{"Your App"};
  std::string database_path{"your_app.db"};
  std::string theme{"dark"};
  bool auto_save{true};
  bool reset_database{false};
  std::string settings_path{"app_settings.json"};
};

AppConfig load_app_config();
void save_app_config(const AppConfig& config);
std::string default_app_name();
std::string default_settings_path();

}  // namespace core

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace views {

enum class ThemeMode {
  Dark,
  Light,
  Classic,
};

struct DemoUser final {
  std::int64_t id{0};
  std::string name;
  std::string email;
};

struct AppSettings final {
  std::string project_name;
  std::string database_path;
  ThemeMode theme_mode{ThemeMode::Dark};
  bool auto_save{true};
};

struct StarterAppData final {
  std::string project_name;
  std::string project_version;
  std::string database_path;
  std::string search_query;
  std::vector<DemoUser> users;
  std::string new_name;
  std::string new_email;
  std::string status_message;
  std::int64_t editing_user_id{0};
  bool is_editing{false};
  AppSettings settings;
  std::function<void(const std::string&, const std::string&)> on_add_user;
  std::function<void(std::int64_t, const std::string&, const std::string&)> on_update_user;
  std::function<void(std::int64_t)> on_remove_user;
  std::function<void(const AppSettings&)> on_apply_settings;
};

int run_starter_app(StarterAppData& data);

}  // namespace views

#include "core/app_config.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

bool parse_bool_value(const std::string& value) {
  if (value == "1" || value == "true" || value == "True" || value == "TRUE") {
    return true;
  }
  return false;
}

std::string read_env_or_default(const char* key, const std::string& fallback) {
  const char* value = std::getenv(key);
  if (value == nullptr || value[0] == '\0') {
    return fallback;
  }
  return std::string(value);
}

std::string read_file_contents(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return {};
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

std::string read_json_string_value(const std::string& json, const std::string& key) {
  const std::string pattern = "\"" + key + "\"";
  const std::size_t key_pos = json.find(pattern);
  if (key_pos == std::string::npos) {
    return {};
  }

  const std::size_t colon = json.find(':', key_pos + pattern.size());
  if (colon == std::string::npos) {
    return {};
  }

  const std::size_t value_begin = json.find_first_not_of(" \t\r\n", colon + 1);
  if (value_begin == std::string::npos) {
    return {};
  }

  if (json[value_begin] == '"') {
    const std::size_t value_end = json.find('"', value_begin + 1);
    if (value_end == std::string::npos) {
      return {};
    }
    return json.substr(value_begin + 1, value_end - value_begin - 1);
  }

  const std::size_t value_end = json.find_first_of(",}\r\n", value_begin);
  return json.substr(value_begin, value_end - value_begin);
}

bool read_json_bool_value(const std::string& json, const std::string& key) {
  const std::string value = read_json_string_value(json, key);
  return parse_bool_value(value);
}

core::AppConfig load_file_config(const std::filesystem::path& path) {
  core::AppConfig config;
  if (!std::filesystem::exists(path)) {
    return config;
  }

  const std::string json = read_file_contents(path);
  if (json.empty()) {
    return config;
  }

  config.app_name = read_json_string_value(json, "app_name");
  config.database_path = read_json_string_value(json, "database_path");
  config.theme = read_json_string_value(json, "theme");
  config.auto_save = read_json_bool_value(json, "auto_save");
  config.reset_database = read_json_bool_value(json, "reset_database");
  config.settings_path = path.string();
  return config;
}

std::string json_escape(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char ch : value) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += ch;
        break;
    }
  }
  return escaped;
}

}  // namespace

namespace core {

std::string default_app_name() { return "Your App"; }

std::string default_settings_path() { return "app_settings.json"; }

AppConfig load_app_config() {
  AppConfig config;
  config.settings_path = read_env_or_default("APP_SETTINGS_PATH", default_settings_path());
  const std::filesystem::path settings_path = config.settings_path;
  const AppConfig file_config = load_file_config(settings_path);
  config.app_name = file_config.app_name.empty() ? default_app_name() : file_config.app_name;
  config.database_path = file_config.database_path.empty() ? "your_app.db" : file_config.database_path;
  config.theme = file_config.theme.empty() ? "dark" : file_config.theme;
  config.auto_save = file_config.auto_save;
  config.reset_database = file_config.reset_database;
  config.settings_path = settings_path.string();

  const char* app_name = std::getenv("APP_NAME");
  if (app_name != nullptr && app_name[0] != '\0') {
    config.app_name = app_name;
  }
  const char* database_path = std::getenv("APP_DB_PATH");
  if (database_path != nullptr && database_path[0] != '\0') {
    config.database_path = database_path;
  }
  const char* theme = std::getenv("APP_THEME");
  if (theme != nullptr && theme[0] != '\0') {
    config.theme = theme;
  }
  const char* auto_save = std::getenv("APP_AUTO_SAVE");
  if (auto_save != nullptr && auto_save[0] != '\0') {
    config.auto_save = parse_bool_value(auto_save);
  }
  const char* reset_db = std::getenv("APP_RESET_DB");
  if (reset_db != nullptr && reset_db[0] != '\0') {
    config.reset_database = parse_bool_value(reset_db);
  }

  if (config.app_name.empty()) {
    config.app_name = default_app_name();
  }
  if (config.database_path.empty()) {
    config.database_path = "your_app.db";
  }
  if (config.theme.empty()) {
    config.theme = "dark";
  }

  return config;
}

void save_app_config(const AppConfig& config) {
  const std::filesystem::path path = config.settings_path.empty() ? default_settings_path() : config.settings_path;
  const std::filesystem::path parent = path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }

  std::ofstream output(path);
  if (!output) {
    return;
  }

  output << "{\n"
         << "  \"app_name\": \"" << json_escape(config.app_name) << "\",\n"
         << "  \"database_path\": \"" << json_escape(config.database_path) << "\",\n"
         << "  \"theme\": \"" << json_escape(config.theme) << "\",\n"
         << "  \"auto_save\": " << (config.auto_save ? "true" : "false") << ",\n"
         << "  \"reset_database\": " << (config.reset_database ? "true" : "false") << "\n"
         << "}\n";
}

}  // namespace core

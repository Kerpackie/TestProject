#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "core/app_config.h"
#include "core/core.h"
#include "database/database.h"
#include "database/database_config.h"
#include "database/domain/user.h"
#include "database/repository/sqlite_user_repository.h"
#include "database/service/user_service.h"
#include "database/transaction.h"
#include "views/app_demo_view.h"

namespace {

views::ThemeMode parse_theme_mode(const std::string& value) {
 if (value == "light") {
   return views::ThemeMode::Light;
 }
 if (value == "classic") {
   return views::ThemeMode::Classic;
 }
 return views::ThemeMode::Dark;
}

std::vector<views::DemoUser> to_demo_users(const std::vector<database::domain::User>& users) {
 std::vector<views::DemoUser> demo_users;
 demo_users.reserve(users.size());
 for (const auto& user : users) {
   demo_users.push_back({.id = user.id, .name = user.name, .email = user.email});
 }
 return demo_users;
}

}  // namespace

int main() {
 try {
   core::AppConfig app_config = core::load_app_config();
   const std::string app_name = app_config.app_name.empty() ? core::default_app_name() : app_config.app_name;
   const std::string database_path = app_config.database_path.empty() ? "your_app.db" : app_config.database_path;
   app_config.app_name = app_name;
   app_config.database_path = database_path;
   app_config.settings_path = app_config.settings_path.empty() ? core::default_settings_path() : app_config.settings_path;

   if (!std::filesystem::exists(app_config.settings_path)) {
     core::save_app_config(app_config);
   }

   if (app_config.reset_database) {
     std::filesystem::remove(database_path);
   }

   database::DatabaseConfig config{
       .database_path = database_path,
       .busy_timeout = std::chrono::milliseconds(5000),
       .foreign_keys = true,
       .wal_mode = true,
   };

   database::Connection connection(config);
   database::repository::SqliteUserRepository user_repository(connection);
   user_repository.init_schema();
   database::service::UserService user_service(user_repository);

   if (user_service.list_users().empty()) {
     database::Transaction tx(connection);
     user_service.register_user("Ada Lovelace", "ada@example.com");
     user_service.register_user("Grace Hopper", "grace@example.com");
     tx.commit();
   }

   views::StarterAppData app_data{
       .project_name = app_name,
       .project_version = core::version(),
       .database_path = database_path,
       .search_query = "",
       .users = to_demo_users(user_service.list_users()),
       .new_name = "",
       .new_email = "",
       .status_message = "Ready to add records.",
       .editing_user_id = 0,
       .is_editing = false,
       .settings = {
           .project_name = app_name,
           .database_path = database_path,
           .theme_mode = parse_theme_mode(app_config.theme),
           .auto_save = app_config.auto_save,
       },
       .on_add_user = nullptr,
       .on_update_user = nullptr,
       .on_remove_user = nullptr,
       .on_apply_settings = nullptr,
   };

   app_data.on_add_user = [&](const std::string& name, const std::string& email) {
     if (name.empty() || email.empty()) {
       app_data.status_message = "Both name and email are required.";
       return;
     }

     try {
       user_service.register_user(name, email);
       app_data.users = to_demo_users(user_service.list_users());
       app_data.status_message = "User added successfully.";
     } catch (const std::exception& e) {
       app_data.status_message = e.what();
     }
   };

   app_data.on_update_user = [&](std::int64_t id, const std::string& name, const std::string& email) {
     if (name.empty() || email.empty()) {
       app_data.status_message = "Both name and email are required.";
       return;
     }

     try {
       const bool updated = user_service.update_user(id, name, email);
       if (updated) {
         app_data.users = to_demo_users(user_service.list_users());
         app_data.status_message = "User updated successfully.";
       } else {
         app_data.status_message = "User could not be updated.";
       }
     } catch (const std::exception& e) {
       app_data.status_message = e.what();
     }
   };

   app_data.on_remove_user = [&](std::int64_t id) {
     try {
       const bool removed = user_service.remove_user(id);
       if (removed) {
         app_data.users = to_demo_users(user_service.list_users());
         app_data.status_message = "User removed.";
       } else {
         app_data.status_message = "User not found or already archived.";
       }
     } catch (const std::exception& e) {
       app_data.status_message = e.what();
     }
   };

   app_data.on_apply_settings = [&](const views::AppSettings& settings) {
     app_data.project_name = settings.project_name.empty() ? app_name : settings.project_name;
     app_data.database_path = settings.database_path.empty() ? database_path : settings.database_path;
     app_data.settings = settings;
     app_data.project_name = app_data.settings.project_name.empty() ? app_name : app_data.settings.project_name;
     app_data.database_path = app_data.settings.database_path.empty() ? database_path : app_data.settings.database_path;

     core::AppConfig updated_config;
     updated_config.app_name = app_data.settings.project_name.empty() ? app_name : app_data.settings.project_name;
     updated_config.database_path = app_data.settings.database_path.empty() ? database_path : app_data.settings.database_path;
     updated_config.theme = app_data.settings.theme_mode == views::ThemeMode::Light ? "light" :
                           (app_data.settings.theme_mode == views::ThemeMode::Classic ? "classic" : "dark");
     updated_config.auto_save = app_data.settings.auto_save;
     updated_config.reset_database = app_config.reset_database;
     updated_config.settings_path = app_config.settings_path;
     core::save_app_config(updated_config);
   };

   return views::run_starter_app(app_data);
 } catch (const std::exception& e) {
   std::cerr << "Failed to bootstrap starter app: " << e.what() << '\n';
   return 1;
 }
}

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace views {

struct DemoUser final {
  std::int64_t id{0};
  std::string name;
  std::string email;
};

struct PhaseResult final {
  std::string title;
  std::vector<std::string> lines;
};

struct AppDemoData final {
  std::string project_version;
  std::string database_version;
  std::vector<DemoUser> users;
  std::vector<PhaseResult> phases;
};

int run_app_demo(const AppDemoData& data);

}  // namespace views

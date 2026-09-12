#include "views/app_demo_view.h"

#include <cctype>
#include <cstring>
#include <iostream>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace views {

namespace {

void apply_theme(ThemeMode mode) {
  switch (mode) {
    case ThemeMode::Dark:
      ImGui::StyleColorsDark();
      break;
    case ThemeMode::Light:
      ImGui::StyleColorsLight();
      break;
    case ThemeMode::Classic:
      ImGui::StyleColorsClassic();
      break;
  }
}

}  // namespace

int run_starter_app(StarterAppData& data) {
  if (glfwInit() == GLFW_FALSE) {
    std::cerr << "Failed to initialize GLFW.\n";
    return 1;
  }

#if defined(__APPLE__)
  constexpr const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  constexpr const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

  const std::string window_title = std::string(data.project_name.empty() ? "Your App" : data.project_name) + " - Starter App";
  GLFWwindow* window = glfwCreateWindow(1400, 840, window_title.c_str(), nullptr, nullptr);
  if (window == nullptr) {
    std::cerr << "Failed to create GLFW window.\n";
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  apply_theme(data.settings.theme_mode);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    apply_theme(data.settings.theme_mode);

    ImGui::SetNextWindowSize(ImVec2(1320.0F, 780.0F), ImGuiCond_FirstUseEver);
    ImGui::Begin((std::string(data.project_name.empty() ? "Your App" : data.project_name) + " Dashboard").c_str());

    ImGui::Text("%s", data.project_name.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("v%s", data.project_version.c_str());
    ImGui::Text("Database: %s", data.database_path.c_str());
    ImGui::Separator();

    ImGui::BeginChild("records_panel", ImVec2(640.0F, 0.0F), true);
    ImGui::TextUnformatted("Records");
    char search_buffer[128] = {};
    std::strncpy(search_buffer, data.search_query.c_str(), sizeof(search_buffer) - 1);
    if (ImGui::InputText("Search", search_buffer, sizeof(search_buffer))) {
      data.search_query = search_buffer;
    }
    if (ImGui::BeginTable("user_table", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable)) {
      ImGui::TableSetupColumn("ID");
      ImGui::TableSetupColumn("Name");
      ImGui::TableSetupColumn("Email");
      ImGui::TableSetupColumn("Edit");
      ImGui::TableSetupColumn("Delete");
      ImGui::TableHeadersRow();

      std::string lower_query = data.search_query;
      for (char& ch : lower_query) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
      }

      for (const auto& user : data.users) {
        const std::string lower_name = user.name;
        const std::string lower_email = user.email;
        std::string name_lower = lower_name;
        std::string email_lower = lower_email;
        for (char& ch : name_lower) {
          ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        for (char& ch : email_lower) {
          ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }

        const bool matches_query = lower_query.empty() || name_lower.find(lower_query) != std::string::npos ||
                                   email_lower.find(lower_query) != std::string::npos;
        if (!matches_query) {
          continue;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%lld", static_cast<long long>(user.id));
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(user.name.c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::TextUnformatted(user.email.c_str());
        ImGui::TableSetColumnIndex(3);
        if (ImGui::SmallButton(("Edit##" + std::to_string(user.id)).c_str())) {
          data.is_editing = true;
          data.editing_user_id = user.id;
          data.new_name = user.name;
          data.new_email = user.email;
          data.status_message = "Editing user: " + user.name;
        }
        ImGui::TableSetColumnIndex(4);
        if (ImGui::SmallButton(("Delete##" + std::to_string(user.id)).c_str())) {
          if (data.on_remove_user) {
            data.on_remove_user(user.id);
          }
        }
      }
      ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("controls_panel", ImVec2(0.0F, 0.0F), true);
    ImGui::TextUnformatted(data.is_editing ? "Edit user" : "Add user");

    char name_buffer[128] = {};
    std::strncpy(name_buffer, data.new_name.c_str(), sizeof(name_buffer) - 1);
    if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
      data.new_name = name_buffer;
    }

    char email_buffer[128] = {};
    std::strncpy(email_buffer, data.new_email.c_str(), sizeof(email_buffer) - 1);
    if (ImGui::InputText("Email", email_buffer, sizeof(email_buffer))) {
      data.new_email = email_buffer;
    }

    if (ImGui::Button(data.is_editing ? "Update user" : "Save user", ImVec2(-1.0F, 0.0F))) {
      if (data.is_editing) {
        if (data.on_update_user) {
          data.on_update_user(data.editing_user_id, data.new_name, data.new_email);
        }
        data.is_editing = false;
        data.editing_user_id = 0;
      } else if (data.on_add_user) {
        data.on_add_user(data.new_name, data.new_email);
      }
      data.new_name.clear();
      data.new_email.clear();
    }

    if (data.is_editing && ImGui::Button("Cancel edit", ImVec2(-1.0F, 0.0F))) {
      data.is_editing = false;
      data.editing_user_id = 0;
      data.new_name.clear();
      data.new_email.clear();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted("Settings");
    char project_name_buffer[128] = {};
    std::strncpy(project_name_buffer, data.settings.project_name.c_str(), sizeof(project_name_buffer) - 1);
    if (ImGui::InputText("App name", project_name_buffer, sizeof(project_name_buffer))) {
      data.settings.project_name = project_name_buffer;
    }

    char database_path_buffer[256] = {};
    std::strncpy(database_path_buffer, data.settings.database_path.c_str(), sizeof(database_path_buffer) - 1);
    if (ImGui::InputText("Database path", database_path_buffer, sizeof(database_path_buffer))) {
      data.settings.database_path = database_path_buffer;
    }

    int theme_index = static_cast<int>(data.settings.theme_mode);
    const char* theme_items[] = {"Dark", "Light", "Classic"};
    if (ImGui::Combo("Theme", &theme_index, theme_items, IM_ARRAYSIZE(theme_items))) {
      data.settings.theme_mode = static_cast<ThemeMode>(theme_index);
      apply_theme(data.settings.theme_mode);
    }

    if (ImGui::Checkbox("Auto-save", &data.settings.auto_save)) {
      data.status_message = data.settings.auto_save ? "Auto-save enabled." : "Auto-save disabled.";
    }

    if (ImGui::Button("Apply settings", ImVec2(-1.0F, 0.0F))) {
      data.project_name = data.settings.project_name;
      data.database_path = data.settings.database_path;
      if (data.on_apply_settings) {
        data.on_apply_settings(data.settings);
      }
      data.status_message = "Settings updated.";
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", data.status_message.empty() ? "Ready." : data.status_message.c_str());
    ImGui::EndChild();

    ImGui::End();

    ImGui::Render();
    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    const float clear_r = data.settings.theme_mode == ThemeMode::Light ? 0.96F : 0.09F;
    const float clear_g = data.settings.theme_mode == ThemeMode::Light ? 0.96F : 0.09F;
    const float clear_b = data.settings.theme_mode == ThemeMode::Light ? 0.98F : 0.12F;
    glClearColor(clear_r, clear_g, clear_b, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

}  // namespace views

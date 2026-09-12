#include "views/app_demo_view.h"

#include <array>
#include <cstddef>
#include <iostream>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace views {

namespace {

constexpr std::array<const char*, 7> kPhaseButtonLabels = {
    "Phase 3: Repository + Manual DI",
    "Phase 4: Multi-Engine Abstraction",
    "Phase 5: Concurrency + Connection Pool",
    "Phase 6: Migrations",
    "Phase 7: Query Builder",
    "Phase 8: Observability",
    "Phase 9: Audit Trail + Soft Delete",
};

}  // namespace

int run_app_demo(const AppDemoData& data) {
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

  GLFWwindow* window = glfwCreateWindow(1440, 900, "TestProject - Application Demo", nullptr, nullptr);
  if (window == nullptr) {
    std::cerr << "Failed to create GLFW window.\n";
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  std::size_t selected_phase = 0;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(1200.0F, 760.0F), ImGuiCond_FirstUseEver);
    ImGui::Begin("Application Demo");

    ImGui::Text("TestProject %s", data.project_version.c_str());
    ImGui::Text("Database module %s", data.database_version.c_str());
    ImGui::Separator();

    ImGui::BeginChild("phase_controls", ImVec2(360.0F, 0.0F), true);
    ImGui::TextUnformatted("Available demonstrations");
    ImGui::Spacing();

    for (std::size_t index = 0; index < data.phases.size() && index < kPhaseButtonLabels.size(); ++index) {
      if (ImGui::Button(kPhaseButtonLabels[index], ImVec2(-1.0F, 0.0F))) {
        selected_phase = index;
      }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted("Registered users");
    for (const auto& user : data.users) {
      ImGui::BulletText("[%lld] %s <%s>",
                        static_cast<long long>(user.id),
                        user.name.c_str(),
                        user.email.c_str());
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("phase_output", ImVec2(0.0F, 0.0F), true);
    if (!data.phases.empty()) {
      const PhaseResult& phase = data.phases[selected_phase];
      ImGui::TextUnformatted(phase.title.c_str());
      ImGui::Separator();
      for (const auto& line : phase.lines) {
        ImGui::TextWrapped("%s", line.c_str());
      }
    }
    ImGui::EndChild();

    ImGui::End();

    ImGui::Render();
    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.10F, 0.10F, 0.12F, 1.0F);
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

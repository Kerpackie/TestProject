#include "views/imgui_demo_view.h"

#include <iostream>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace views {

int demonstrate_imgui_phase() {
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

  GLFWwindow* window = glfwCreateWindow(1280, 720, "TestProject - Dear ImGui", nullptr, nullptr);
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

  int frames_rendered = 0;
  while (!glfwWindowShouldClose(window) && frames_rendered < 300) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool open = true;
    ImGui::Begin("Phase 10: Dear ImGui Integration", &open);
    ImGui::TextUnformatted("Dear ImGui is linked and running inside the views library.");
    ImGui::Separator();
    ImGui::BulletText("Version: %s", IMGUI_VERSION);
    ImGui::BulletText("Renderer: GLFW + OpenGL3");
    ImGui::BulletText("Frame: %d", frames_rendered + 1);
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
    ++frames_rendered;
  }

  const ImDrawData* draw_data = ImGui::GetDrawData();
  std::cout << "ImGui context initialized. Version: " << IMGUI_VERSION << '\n';
  std::cout << "Generated draw lists: " << draw_data->CmdListsCount
            << ", total vertices: " << draw_data->TotalVtxCount
            << ", total indices: " << draw_data->TotalIdxCount << '\n';

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

}  // namespace views

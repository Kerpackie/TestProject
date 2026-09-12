add_library(views STATIC
  src/imgui_demo_view.cpp
)

find_package(OpenGL REQUIRED)

target_include_directories(views
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(views
  PRIVATE
    dear_imgui
    glfw
    OpenGL::GL
)

target_compile_definitions(views PRIVATE TESTPROJECT_IMGUI_WINDOW_ENABLED=1)

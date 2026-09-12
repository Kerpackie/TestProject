# External dependencies, fetched at configure time.
# Keep this list minimal; add entries only when a real need appears.

include(FetchContent)

option(TESTPROJECT_ENABLE_IMGUI_WINDOW "Build Dear ImGui with a real GLFW/OpenGL window backend" ON)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)

# GoogleTest — pinned to a specific release for reproducible builds.
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0
  GIT_SHALLOW TRUE
)

# Build gtest into our tree; never install it.
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# SQLiteCpp — pinned to a specific release for reproducible builds.
FetchContent_Declare(
  SQLiteCpp
  GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
  GIT_TAG 3.3.3
  GIT_SHALLOW TRUE
)

set(SQLITECPP_INTERNAL_SQLITE ON CACHE BOOL "" FORCE)
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SQLiteCpp)

# Dear ImGui — pinned docking branch for an immediate UI integration path.
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.2b-docking
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(imgui)

add_library(dear_imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

target_include_directories(dear_imgui
  PUBLIC
    ${imgui_SOURCE_DIR}
)

if(TESTPROJECT_ENABLE_IMGUI_WINDOW)
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
  )

  FetchContent_MakeAvailable(glfw)

  target_sources(dear_imgui PRIVATE
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )

  target_link_libraries(dear_imgui
    PUBLIC
      glfw
  )

  target_include_directories(dear_imgui
    PUBLIC
      ${imgui_SOURCE_DIR}/backends
  )
endif()

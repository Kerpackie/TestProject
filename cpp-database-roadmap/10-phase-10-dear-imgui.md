# Phase 10 — Dear ImGui Integration

## 1. The Pain Point

Once a C++ application grows beyond console output, teams need a fast way to add developer-facing UI without committing to a full retained-mode widget framework or platform-specific toolkits on day one.

Without a lightweight UI layer:

- diagnostics and internal tooling remain trapped in logs;
- prototypes require premature windowing architecture decisions;
- simple inspectors or debug panels become disproportionately expensive to build.

## 2. Objective & Architecture

Integrate Dear ImGui as a pinned source dependency through a dedicated `views` library and keep the first phase intentionally backend-agnostic:

```text
app target
   |
   +--> views library
           |
           +--> dear_imgui static library
                   |
                   +--> ImGui core sources
```

This phase proves that the application can compile, link, and execute an immediate-mode UI frame while preserving the current console-driven demo flow and keeping view logic out of the application layer.

## 3. Technical Specifications

- Pin Dear ImGui to a known tag/branch for reproducible builds.
- Build ImGui core sources as a dedicated static library target.
- Implement Dear ImGui view behavior inside a dedicated `views` library.
- Expose only ImGui's public headers through the library target include path.
- Keep platform/renderer backends out of this phase; prove the core API first.
- Let the application layer invoke a view-facing API instead of owning widget or frame logic directly.
- Document the integration in the same roadmap/script style as earlier phases.

## 4. File Structure

```text
libs/views/
├── CMakeLists.txt
├── include/views/imgui_demo_view.h
└── src/imgui_demo_view.cpp
cmake/
└── dependencies.cmake
app/
├── CMakeLists.txt
└── main.cpp
cpp-database-roadmap/
└── 10-phase-10-dear-imgui.md
scripts/
└── phase-10-script.md
```

## 5. Draft Code

### `cmake/dependencies.cmake`

```cmake
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
```

### `libs/views/src/imgui_demo_view.cpp`

```cpp
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGuiIO& io = ImGui::GetIO();

io.DisplaySize = ImVec2(1280.0F, 720.0F);
io.DeltaTime = 1.0F / 60.0F;

ImGui::StyleColorsDark();
ImGui::NewFrame();
ImGui::Begin("Phase 10: Dear ImGui Integration");
ImGui::TextUnformatted("Dear ImGui is linked and running inside the views library.");
ImGui::End();
ImGui::Render();
```

### `app/main.cpp`

```cpp
#include "views/imgui_demo_view.h"

// Composition root stays thin: invoke the view library, do not build widgets here.
views::demonstrate_imgui_phase();
```

## 6. Verification Criteria

**Pass** when:

1. A fresh configure downloads Dear ImGui automatically.
2. The `app` target links `views`, and `views` owns the ImGui dependency.
3. Running the executable reaches the ImGui demo phase without crashing.
4. Rendered draw data contains at least one generated command list.

## 7. Gate Beyond This Phase

Only after this phase passes should the project add a platform/window backend such as GLFW + OpenGL, SDL, or Vulkan integration. The immediate goal is deterministic dependency wiring and a minimal UI execution path, not a full renderer stack.

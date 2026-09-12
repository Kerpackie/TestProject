# TestProject

Minimal C++20 scaffold: CMake + Ninja, GoogleTest, SQLiteCpp, Dear ImGui, GLFW, OpenGL, CTest.

## Layout

```
libs/core/   core library — public headers in include/, private implementation in src/
libs/views/  UI/view library — Dear ImGui integration isolated from app composition
app/         thin executable entry point
tests/       unit tests, mirroring the libs/ structure
cmake/       dependency management (FetchContent)
```

## Prerequisites

- CMake >= 3.20
- Ninja
- A C++20 compiler (Apple Clang / Clang / GCC)
- Network access on first configure (fetches pinned GoogleTest, SQLiteCpp, Dear ImGui, and GLFW sources)

## Build

```sh
cmake --preset debug
cmake --build --preset debug
```

## Test

```sh
ctest --preset debug
```

## Run

```sh
./build/debug/app/app
```

The Dear ImGui phase opens a real window via the `views` library using GLFW + OpenGL3.
If GLFW platform dependencies are unavailable on the current machine, the build still succeeds and Phase 10 falls back to a backend-free ImGui demonstration.

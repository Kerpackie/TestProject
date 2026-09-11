# TestProject

Minimal C++20 scaffold: CMake + Ninja, GoogleTest (pinned to v1.16.0), CTest.

## Layout

```
libs/core/   core library — public headers in include/, private implementation in src/
app/         thin executable entry point
tests/       unit tests, mirroring the libs/ structure
cmake/       dependency management (FetchContent)
```

## Prerequisites

- CMake >= 3.20
- Ninja
- A C++20 compiler (Apple Clang / Clang / GCC)
- Network access on first configure (fetches GoogleTest v1.16.0)

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

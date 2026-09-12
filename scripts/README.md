# TestProject

Minimal C++20 scaffold: CMake + Ninja, GoogleTest (pinned to v1.16.0), CTest.

## Using as a GitHub Template

You can instantiate this repository directly on GitHub:

1. Click the green **"Use this template"** button $\rightarrow$ **"Create a new repository"**.
2. Or use the GitHub CLI:
   ```sh
   gh repo create my-awesome-project --template <owner>/<template-repo> --public
   ```
3. After cloning your new repository, customize it:
   - **Locally**: Run `./scripts/init_project.sh`
   - **Or via GitHub Actions**: Go to the **Actions** tab $\rightarrow$ select **"Template Initialization"** $\rightarrow$ **"Run workflow"** to automatically update project identifiers across all files.

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

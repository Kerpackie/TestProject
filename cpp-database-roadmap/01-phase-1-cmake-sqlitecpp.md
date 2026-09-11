# Phase 1 — CMake & SQLiteCpp Setup via FetchContent

## 1. The Pain Point

A toy C++ database example often assumes a globally installed SQLite development package, manually configured include paths, and linker flags that happen to work on one machine. The result is fragile onboarding:

- CLion works for the author but not CI.
- Windows, Linux, and macOS use different package-manager conventions.
- A developer can accidentally compile against one SQLite library and link against another.
- A floating Git branch can silently change the wrapper API or build options.
- Hand-written `include_directories()` and `link_directories()` leak dependency details into the entire project.
- Rebuilding from a clean machine becomes a scavenger hunt.

SQLiteCpp itself provides a CMake target and can build an internal SQLite3 copy; the current upstream project also exposes options such as `SQLITECPP_INTERNAL_SQLITE`. citeturn721891search1turn721891search5

## 2. Objective & Architecture

Create a reproducible dependency graph in which the application links to a namespaced/project target rather than manually managing raw include and library paths.

### Architecture

```text
app target
   |
   +--> database library (our code)
           |
           +--> SQLiteCpp::target (third-party target as provided by the dependency)
                    |
                    +--> SQLite3
```

The first phase keeps database infrastructure deliberately small. We are proving that the build is deterministic before layering lifecycle and architecture concerns on top.

CMake's modern `FetchContent_MakeAvailable()` pattern is preferred; the older single-argument `FetchContent_Populate()` flow is deprecated in newer CMake policies. citeturn397109search10turn397109search11

## 3. Technical Specifications

- Set the C++ standard at the top-level target, not ad hoc per source file.
- Prefer target-based CMake (`target_link_libraries`, `target_compile_features`, target include visibility).
- Pin SQLiteCpp to a release/tag or immutable commit.
- Avoid global `include_directories()` and `add_definitions()`.
- Make build options explicit and discoverable.
- Keep third-party targets private to the database infrastructure target unless application code truly consumes them.
- Use a separate test executable so “database dependency is reachable” can be verified without booting the whole application.

## 4. File Structure

```text
.
├── CMakeLists.txt
├── cmake/
│   └── Dependencies.cmake
├── src/
│   └── main.cpp
├── include/
└── tests/
    └── smoke_database.cpp
```

## 5. Draft Code

### `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.28)
project(database_layer LANGUAGES CXX)

# Keep the language baseline in one place. C++20 is the primary teaching target;
# individual targets should inherit this capability rather than redefining it.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build our third-party dependencies before targets that consume them.
include(cmake/Dependencies.cmake)

add_library(database
    src/database/Database.cpp
)

target_include_directories(database
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# SQLiteCpp and its SQLite dependency are implementation details of our
# database layer. Keep them PRIVATE so application targets do not start
# depending directly on wrapper-specific APIs.
target_link_libraries(database PRIVATE SQLiteCpp)

target_compile_features(database PUBLIC cxx_std_20)

add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE database)

enable_testing()
add_executable(database_smoke_test tests/smoke_database.cpp)
target_link_libraries(database_smoke_test PRIVATE database)
add_test(NAME database_smoke_test COMMAND database_smoke_test)
```

### `cmake/Dependencies.cmake`

```cmake
include(FetchContent)

# Pin to a known version/revision. Never use a moving branch in a production
# guide because a clean build six months later should resolve the same source.
FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG 3.3.3
    GIT_SHALLOW TRUE
)

# SQLiteCpp can build its own SQLite library. That removes one major source of
# machine-to-machine linker differences for this tutorial.
set(SQLITECPP_INTERNAL_SQLITE ON CACHE BOOL "Build SQLite from SQLiteCpp source" FORCE)
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "Do not build SQLiteCpp tests" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "Do not run SQLiteCpp lint in our build" FORCE)
set(SQLITECPP_INSTALL OFF CACHE BOOL "No install step for embedded dependency" FORCE)

# Modern CMake resolves and adds the dependency in one operation.
FetchContent_MakeAvailable(SQLiteCpp)
```

SQLiteCpp 3.3.3's upstream CMake file supports C++11+, builds a `SQLiteCpp` target, and can embed SQLite3. citeturn721891search1

### `src/main.cpp`

```cpp
#include <iostream>

int main() {
    // Phase 1 intentionally proves only that the application target can be
    // built and linked. Database ownership comes in Phase 2.
    std::cout << "Database layer bootstrap OK\n";
}
```

### Smoke test

```cpp
#include <cassert>

#include <SQLiteCpp/Database.h>

int main() {
    // Opening an in-memory database is ideal for a dependency smoke test:
    // it exercises the wrapper and SQLite without touching the filesystem.
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    db.exec("CREATE TABLE smoke (id INTEGER PRIMARY KEY, value TEXT NOT NULL)");
    db.exec("INSERT INTO smoke(value) VALUES('ok')");

    const auto count = db.execAndGet("SELECT COUNT(*) FROM smoke").getInt();
    assert(count == 1);
}
```

## 6. Build Workflow

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

The teaching point is that all three commands should succeed from a fresh checkout without a pre-installed SQLite development package.

## 7. Verification Criteria

**Pass** when:

1. A brand-new clone configures without manual include/library flags.
2. Debug and Release both compile.
3. The smoke test creates, inserts, and reads from an in-memory database.
4. Deleting the build directory and rebuilding produces the same result.
5. The application target does not include SQLiteCpp headers.
6. CI can perform the same configure/build/test sequence.

**Failure symptoms to diagnose**: missing target names, accidental system SQLite linkage, dependency options applied after `FetchContent_MakeAvailable()`, or CMake code that assumes a particular shell/toolchain.

## 8. Gate to Phase 2

Do not move on until the dependency graph is deterministic. The next phase assumes that a reader can open a database through a dedicated lifecycle component without knowing how SQLiteCpp was obtained.

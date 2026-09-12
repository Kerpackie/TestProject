# Application Starter Template

A C++20 starter template for a database-backed desktop application: SQLite persistence, repository/service patterns, and a polished Dear ImGui shell.

This repository is built to be a GitHub template. It includes a working sample app, but the code is intentionally structured so you can rename it, replace the sample domain model, and swap in your own business logic without rewriting the project architecture.

## Included

- SQLite-backed user repository and service layer
- ImGui starter UI with record listing, search, add/edit/delete flows
- App settings panel for app name, database path, theme, and auto-save configuration
- Centralized runtime config via environment variables (`APP_NAME`, `APP_DB_PATH`, `APP_THEME`, `APP_AUTO_SAVE`)
- CMake + FetchContent dependency setup for GoogleTest, SQLiteCpp, Dear ImGui, and GLFW
- CI workflow for GitHub Actions
- Unit tests covering the database layer

## Layout

```
libs/core/       app metadata, version, and runtime configuration helpers
libs/database/   persistence, repositories, services, and transaction patterns
libs/views/      Dear ImGui starter app shell
app/             executable entry point and composition root
tests/           unit tests
cmake/           dependency wiring
.github/workflows/ CI job for build + test
```

## Rename the template for your app

1. Update the default app name in `libs/core/include/core/app_config.h` or set environment variables at runtime.
2. Replace the sample `User` model in `libs/database/include/database/domain/user.h` with your own domain objects.
3. Update the UI labels in `libs/views/src/app_demo_view.cpp` to match your business domain.
4. Update this README and project metadata for your real app.

## Configure at runtime

The app persists user-facing settings to `app_settings.json` in the working directory by default. You can override the defaults without editing code:

```sh
APP_NAME="Acme CRM" APP_DB_PATH="acme_crm.db" APP_THEME="dark" APP_AUTO_SAVE="true" APP_SETTINGS_PATH="./app_settings.json" ./build/debug/app/app
```

The settings panel updates this file automatically when you click Apply settings.

## Prerequisites

- CMake >= 3.20
- Ninja
- A C++20 compiler
- Network access on first configure for FetchContent dependencies

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

On startup, the app initializes the schema, seeds a couple of sample records, and opens a presentable ImGui dashboard for managing records.

## Template conventions

- Prefer replacing the sample `User` entity with your app domain model instead of adding unrelated demo features.
- Keep the repository/service split for database access and business logic.
- Use environment variables or config files for runtime app settings instead of hardcoding production values.
- Keep the CI pipeline green before publishing the repo as a reusable template.

## License

This project is available under the MIT License. See `LICENSE` for details.

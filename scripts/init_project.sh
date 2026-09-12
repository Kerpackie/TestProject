#!/usr/bin/env bash
# ==============================================================================
# Project Scaffold Initializer
# Renames and configures a new project from this template.
# Works across macOS (BSD) and Linux (GNU).
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ANSI Colors
BOLD="\033[1m"
GREEN="\033[32m"
CYAN="\033[36m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

echo -e "${BOLD}${CYAN}=== C++20 Project Template Initializer ===${RESET}\n"

# 1. Gather Project Info
PROJECT_NAME=""
PROJECT_DESC=""
PROJECT_NAMESPACE=""

if [[ $# -ge 1 ]]; then
  PROJECT_NAME="$1"
fi
if [[ $# -ge 2 ]]; then
  PROJECT_DESC="$2"
fi

if [[ -z "${PROJECT_NAME}" ]]; then
  read -r -p "Enter new project name (e.g. MyEngine, TaskManager) [Default: MyProject]: " PROJECT_NAME
  PROJECT_NAME="${PROJECT_NAME:-MyProject}"
fi

# Sanitize project name for identifiers / namespace
DEFAULT_NS=$(echo "${PROJECT_NAME}" | tr '[:upper:]' '[:lower:]' | tr -cs 'a-z0-9' '_' | sed 's/^_//;s/_$//')

if [[ -z "${PROJECT_DESC}" ]]; then
  read -r -p "Enter short project description [Default: Modern C++20 application]: " PROJECT_DESC
  PROJECT_DESC="${PROJECT_DESC:-Modern C++20 application}"
fi

read -r -p "Enter root C++ namespace [Default: ${DEFAULT_NS}]: " PROJECT_NAMESPACE
PROJECT_NAMESPACE="${PROJECT_NAMESPACE:-${DEFAULT_NS}}"

echo ""
echo -e "${BOLD}Summary of changes:${RESET}"
echo -e "  - Project Name:       ${GREEN}${PROJECT_NAME}${RESET}"
echo -e "  - Description:        ${GREEN}${PROJECT_DESC}${RESET}"
echo -e "  - Default Namespace:  ${GREEN}${PROJECT_NAMESPACE}${RESET}"
echo ""

read -r -p "Proceed with project initialization? [y/N]: " CONFIRM
if [[ ! "${CONFIRM}" =~ ^[Yy]$ ]]; then
  echo -e "${YELLOW}Initialization cancelled.${RESET}"
  exit 0
fi

echo -e "\n${CYAN}Applying replacements...${RESET}"

# Cross-platform in-place replacement function
replace_in_file() {
  local pattern="$1"
  local replacement="$2"
  local file="$3"

  if [[ -f "${file}" ]]; then
    if [[ "$OSTYPE" == "darwin"* ]]; then
      sed -i '' "s|${pattern}|${replacement}|g" "${file}"
    else
      sed -i "s|${pattern}|${replacement}|g" "${file}"
    fi
  fi
}

cd "${ROOT_DIR}"

# 1. Update root CMakeLists.txt
replace_in_file "project(TestProject" "project(${PROJECT_NAME}" "CMakeLists.txt"
replace_in_file "DESCRIPTION \"Minimal C++20 project scaffold\"" "DESCRIPTION \"${PROJECT_DESC}\"" "CMakeLists.txt"

# 2. Update app/main.cpp
replace_in_file "\"TestProject \"" "\"${PROJECT_NAME} \"" "app/main.cpp"

# 3. Update README.md
cat << 'EOF' > README.md
# PROJECT_NAME_PLACEHOLDER

PROJECT_DESC_PLACEHOLDER

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
- Network access on first configure (fetches GoogleTest)

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
EOF

replace_in_file "PROJECT_NAME_PLACEHOLDER" "${PROJECT_NAME}" "README.md"
replace_in_file "PROJECT_DESC_PLACEHOLDER" "${PROJECT_DESC}" "README.md"

# 4. Clean previous build artifacts
if [[ -d "build" ]]; then
  echo -e "${YELLOW}Removing existing build/ artifacts...${RESET}"
  rm -rf build
fi

# 5. Prompt to reset git repository
echo ""
read -r -p "Do you want to re-initialize a fresh Git repository? [y/N]: " RESET_GIT
if [[ "${RESET_GIT}" =~ ^[Yy]$ ]]; then
  echo -e "${YELLOW}Reinitializing Git repository...${RESET}"
  rm -rf .git
  git init
  git add .
  git commit -m "chore: initial project baseline for ${PROJECT_NAME}"
  echo -e "${GREEN}✓ Git repository initialized with baseline commit.${RESET}"
fi

echo -e "\n${BOLD}${GREEN}✔ Project successfully configured as '${PROJECT_NAME}'!${RESET}\n"
echo -e "Next steps:"
echo -e "  1. ${CYAN}cmake --preset debug${RESET}"
echo -e "  2. ${CYAN}cmake --build --preset debug${RESET}"
echo -e "  3. ${CYAN}ctest --preset debug${RESET}"
echo -e "  4. ${CYAN}./build/debug/app/app${RESET}\n"

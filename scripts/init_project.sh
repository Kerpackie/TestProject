#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_NAME="${1:-MyApp}"
PROJECT_DESC="${2:-Modern C++20 desktop application}"
PROJECT_NAMESPACE="${3:-myapp}"

sanitize_namespace() {
  printf '%s' "$1" \
    | tr '[:upper:]' '[:lower:]' \
    | tr -cs 'a-z0-9_' '_' \
    | sed 's/^_//; s/_$//'
}

PROJECT_NAMESPACE="$(sanitize_namespace "${PROJECT_NAMESPACE}")"
if [[ -z "${PROJECT_NAMESPACE}" ]]; then
  PROJECT_NAMESPACE="myapp"
fi

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

replace_in_file "ApplicationStarterTemplate" "${PROJECT_NAME}" "CMakeLists.txt"
replace_in_file "Your App" "${PROJECT_NAME}" "libs/core/include/core/app_config.h"
replace_in_file "your_app.db" "${PROJECT_NAMESPACE}.db" "libs/core/include/core/app_config.h"
replace_in_file "Project Template" "${PROJECT_NAME}" "README.md"
replace_in_file "A C++20 starter template for a database-backed desktop application" "${PROJECT_DESC}" "README.md"
replace_in_file "__APP_NAMESPACE__" "${PROJECT_NAMESPACE}" "README.md"

cat > README.md <<EOF_README
# ${PROJECT_NAME}

${PROJECT_DESC}

## Getting started

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

## Run

```sh
APP_NAME="${PROJECT_NAME}" ./build/debug/app/app
```
EOF_README

echo "Initialized project template for ${PROJECT_NAME}."

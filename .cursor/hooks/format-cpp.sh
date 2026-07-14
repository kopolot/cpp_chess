#!/bin/bash
# Formatuje pliki C/C++ po edycji agenta (clang-format, styl Google).
set -euo pipefail

input=$(cat)

# Wyciągnij ścieżkę pliku z JSON hooka (obsługa jq lub fallback grep)
if command -v jq &>/dev/null; then
  file_path=$(echo "$input" | jq -r '.file_path // .path // .file // empty' 2>/dev/null || true)
else
  file_path=$(echo "$input" | grep -oP '"(?:file_path|path|file)"\s*:\s*"\K[^"]+' | head -1 || true)
fi

if [[ -z "${file_path:-}" ]]; then
  exit 0
fi

case "$file_path" in
  *.cpp|*.hpp|*.h|*.cc|*.cxx)
    if command -v clang-format &>/dev/null && [[ -f "$file_path" ]]; then
      clang-format -i "$file_path"
    fi
    ;;
esac

exit 0

#!/usr/bin/env bash
set -euo pipefail

# --- Configuration ---
EXE="${1:-build/bin/libresprite.exe}"
DEST_DIR="$(dirname "$EXE")"

# Ensure executable exists
if [[ ! -f "$EXE" ]]; then
  echo "ERROR: Executable not found: $EXE" >&2
  exit 1
fi

MSYS_ROOT="/ucrt64"

ldd "$EXE" | grep -iE '/ucrt64/' | while IFS= read -r line; do
  # Extract the path between '=>' and '('
  libpath=$(echo "$line" | sed -nE 's/.*=>[[:space:]]*([^[:space:]]+)[[:space:]]*\(.*/\1/p')
  
  if [[ -f "$libpath" ]]; then
    cp -u "$libpath" "$DEST_DIR"/
  else
    echo "ERROR: Missing file: $libpath"
  fi
done

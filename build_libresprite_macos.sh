#!/bin/bash
# Copyright (C) 2026  LibreSprite contributors

set -e  # Exit immediately on error

echo "🍎 LibreSprite macOS build script"
echo "================================="

# -------- Configuration --------
REPO_URL="https://github.com/LibreSprite/LibreSprite.git"
REPO_DIR="LibreSprite"
BREW_PREFIX="/opt/homebrew"
# --------------------------------

# ---- Check Homebrew ----
if ! command -v brew >/dev/null 2>&1; then
  echo "❌ Homebrew not found."
  echo "Install it with:"
  echo '/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
  exit 1
fi

echo "🍺 Homebrew found"

# ---- Check for git (needed to clone repo) ----
if ! command -v git >/dev/null 2>&1; then
  echo "❌ Missing tool: git"
  echo "Install it with: brew install git"
  exit 1
fi

echo "✅ Git available"

# ---- Install dependencies ----
echo "📦 Installing LibreSprite dependencies (Homebrew)"

brew update

brew install \
  cmake \
  ninja \
  gnutls \
  freetype \
  jpeg \
  webp \
  pixman \
  sdl2 \
  sdl2_image \
  tinyxml2 \
  libarchive \
  v8 \
  zlib \
  xmlto \
  dylibbundler || true

echo "✅ Dependencies installed"

# ---- Verify Homebrew prefix ----
if [ ! -d "$BREW_PREFIX" ]; then
  echo "❌ Homebrew prefix not found at $BREW_PREFIX"
  exit 1
fi

echo "🍺 Homebrew prefix: $BREW_PREFIX"

# ---- Clone or update repo ----
if [ ! -d "$REPO_DIR" ]; then
  echo "📥 Cloning LibreSprite repository"
  git clone --recursive "$REPO_URL"
else
  echo "🔄 Updating existing repository"
  cd "$REPO_DIR"
  git pull
  git submodule update --init --recursive
  cd ..
fi

cd "$REPO_DIR"

# ---- Clean build directory ----
if [ -d "build" ]; then
  echo "🧹 Removing existing build directory"
  rm -rf build
fi

mkdir build
cd build

# ---- Configure ----
echo "⚙️  Configuring project with CMake"

cmake \
  -G Ninja \
  -DCMAKE_PREFIX_PATH="$BREW_PREFIX" \
  -DCMAKE_EXE_LINKER_FLAGS="-L$BREW_PREFIX/lib" \
  ..

# ---- Build ----
echo "🔨 Building LibreSprite"
ninja libresprite

echo
echo "🎉 LibreSprite build completed successfully!"
echo "➡ Run with: ./LibreSprite/build/bin/libresprite"

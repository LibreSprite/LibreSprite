# LibreSprite Build System
# Usage: make help

.PHONY: help deps build bundle dmg run clean distclean

# Detect OS
UNAME := $(shell uname -s)

# Build configuration
BUILD_DIR := build
BUILD_TYPE := Release
BUNDLE_DIR := bundle
APP_NAME := libresprite
APP_BUNDLE := $(BUNDLE_DIR)/$(APP_NAME).app

# Default target
help:
	@echo "LibreSprite Build System"
	@echo ""
	@echo "Usage: make <target>"
	@echo ""
	@echo "Targets:"
	@echo "  deps       Install dependencies (macOS: brew, Linux: apt/dnf)"
	@echo "  build      Configure and build LibreSprite"
	@echo "  bundle     Create macOS .app bundle (macOS only)"
	@echo "  dmg        Create distributable DMG (macOS only)"
	@echo "  run        Run LibreSprite"
	@echo "  clean      Remove build artifacts"
	@echo "  distclean  Remove build, bundle, and DMG"
	@echo ""
	@echo "Examples:"
	@echo "  make deps build run    # First time setup"
	@echo "  make build run         # Rebuild and run"
	@echo "  make dmg               # Create distributable (builds if needed)"

# Install dependencies
deps:
ifeq ($(UNAME),Darwin)
	brew install gnutls freetype jpeg webp pixman sdl2 sdl2_image tinyxml2 \
		libarchive v8 ninja zlib xmlto dylibbundler cmake create-dmg
else ifeq ($(shell which apt-get 2>/dev/null),)
	sudo dnf install g++ cmake libcurl-devel freetype-devel giflib-devel \
		gtest-devel libjpeg-devel pixman-devel libpng-devel SDL2-devel \
		SDL2_image-devel tinyxml2-devel zlib-devel ninja-build nodejs-devel \
		libarchive-devel
else
	sudo apt-get install cmake g++ libcurl4-gnutls-dev libfreetype6-dev \
		libgif-dev libgtest-dev libjpeg-dev libpixman-1-dev libpng-dev \
		libsdl2-dev libsdl2-image-dev libtinyxml2-dev libnode-dev ninja-build \
		zlib1g-dev libarchive-dev
endif

# Configure and build
build: $(BUILD_DIR)/build.ninja
	ninja -C $(BUILD_DIR)

$(BUILD_DIR)/build.ninja:
	@mkdir -p $(BUILD_DIR)
ifeq ($(UNAME),Darwin)
	cmake -S . -B $(BUILD_DIR) -G Ninja \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_OSX_ARCHITECTURES=arm64 \
		-DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
else
	cmake -S . -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
endif

# Create macOS app bundle
bundle: build
ifeq ($(UNAME),Darwin)
	@echo "Creating app bundle..."
	@rm -rf $(BUNDLE_DIR)
	@mkdir -p $(APP_BUNDLE)/Contents/{MacOS,Resources,libs}
	@cp $(BUILD_DIR)/bin/$(APP_NAME) $(APP_BUNDLE)/Contents/MacOS/
	@cp -r $(BUILD_DIR)/bin/data $(APP_BUNDLE)/Contents/Resources/
	@cd $(APP_BUNDLE)/Contents/MacOS && ln -sf ../Resources/data data
	@cp desktop/Info.plist $(APP_BUNDLE)/Contents/
	@# Generate icons
	@mkdir -p $(BUNDLE_DIR)/$(APP_NAME).iconset
	@sips -z 16 16 data/icons/ase64.png --out $(BUNDLE_DIR)/$(APP_NAME).iconset/icon_16x16.png
	@sips -z 32 32 data/icons/ase64.png --out $(BUNDLE_DIR)/$(APP_NAME).iconset/icon_16x16@2x.png
	@sips -z 32 32 data/icons/ase64.png --out $(BUNDLE_DIR)/$(APP_NAME).iconset/icon_32x32.png
	@sips -z 64 64 data/icons/ase64.png --out $(BUNDLE_DIR)/$(APP_NAME).iconset/icon_32x32@2x.png
	@sips -z 128 128 data/icons/ase64.png --out $(BUNDLE_DIR)/$(APP_NAME).iconset/icon_128x128.png
	@sips -z 256 256 data/icons/ase64.png --out $(BUNDLE_DIR)/$(APP_NAME).iconset/icon_128x128@2x.png
	@sips -z 256 256 data/icons/ase64.png --out $(BUNDLE_DIR)/$(APP_NAME).iconset/icon_256x256.png
	@sips -z 512 512 data/icons/ase64.png --out $(BUNDLE_DIR)/$(APP_NAME).iconset/icon_256x256@2x.png
	@sips -z 512 512 data/icons/ase64.png --out $(BUNDLE_DIR)/$(APP_NAME).iconset/icon_512x512.png
	@sips -z 512 512 data/icons/ase64.png --out $(BUNDLE_DIR)/$(APP_NAME).iconset/icon_512x512@2x.png
	@iconutil -c icns $(BUNDLE_DIR)/$(APP_NAME).iconset -o $(APP_BUNDLE)/Contents/Resources/$(APP_NAME).icns
	@# Bundle dynamic libraries
	@echo "Bundling dynamic libraries..."
	@dylibbundler -od -b -ns -x $(APP_BUNDLE)/Contents/MacOS/$(APP_NAME) \
		-d $(APP_BUNDLE)/Contents/libs/
	@# Sign all components (inside-out order)
	@echo "Signing app bundle..."
	@for lib in $(APP_BUNDLE)/Contents/libs/*.dylib; do \
		codesign --force -s - "$$lib"; \
	done
	@codesign --force -s - $(APP_BUNDLE)/Contents/MacOS/$(APP_NAME)
	@codesign --force -s - $(APP_BUNDLE)
	@echo "Bundle created: $(APP_BUNDLE)"
else
	@echo "Error: 'make bundle' is only supported on macOS"
	@exit 1
endif

# Create DMG
dmg: bundle
ifeq ($(UNAME),Darwin)
	@echo "Creating DMG..."
	@rm -f $(APP_NAME).dmg
	@create-dmg \
		--volname "LibreSprite" \
		--window-pos 200 120 \
		--window-size 600 400 \
		--icon-size 100 \
		--icon "$(APP_NAME).app" 150 185 \
		--app-drop-link 450 185 \
		"$(APP_NAME).dmg" \
		"$(APP_BUNDLE)"
	@echo "DMG created: $(APP_NAME).dmg"
else
	@echo "Error: 'make dmg' is only supported on macOS"
	@exit 1
endif

# Run the application
run: build
ifeq ($(UNAME),Darwin)
	@if [ -d "$(APP_BUNDLE)" ]; then \
		open $(APP_BUNDLE); \
	else \
		$(BUILD_DIR)/bin/$(APP_NAME); \
	fi
else
	$(BUILD_DIR)/bin/$(APP_NAME)
endif

# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Build directory removed"

# Full clean including bundle and DMG
distclean: clean
	@rm -rf $(BUNDLE_DIR) $(APP_NAME).dmg
	@echo "Bundle and DMG removed"

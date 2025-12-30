# Tests Module

*Last updated: 2025-12-30*

Testing infrastructure for LibreSprite using Google Test.

## Files

| File | Purpose |
|------|---------|
| `test.h` | Test harness header - includes gtest and handles GUI test setup |

## Usage

Include `test.h` in test files. Tests are compiled with `_tests.cpp` suffix throughout the codebase (e.g., `document_api_tests.cpp`, `color_tests.cpp`).

## Test Modes

- **Standard tests**: Unit tests without GUI
- **GUI tests**: When `TEST_GUI` is defined, initializes `ui::UISystem` and `ui::Manager`

## Running Tests

Tests are built as separate executables via CMake. Run after building.

## Dependencies

Depends on: gtest (Google Test framework)

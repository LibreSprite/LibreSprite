# Clip Library Tests

*Last updated: 2025-12-30*

This directory contains unit tests for the clip clipboard library.

## Test Files

| File | Description |
|------|-------------|
| **text_tests.cpp** | Tests for text clipboard operations (set/get text, lock API, clear functionality) |
| **image_tests.cpp** | Tests for image clipboard operations |
| **user_format_tests.cpp** | Tests for custom user-defined clipboard formats |

## Test Infrastructure

- **CMakeLists.txt** - CMake configuration for building and running tests via CTest
- **test.h** - Test helper macros (EXPECT_TRUE, EXPECT_FALSE, EXPECT_EQ, etc.)

## Purpose

These tests verify the correctness of the clip library's clipboard functionality across different data types and APIs. They are included as part of the forked/vendored third-party code and are executed by the upstream library's CI pipeline.

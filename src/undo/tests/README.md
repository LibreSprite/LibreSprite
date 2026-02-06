# Undo Library Tests

*Last updated: 2025-12-30*

This directory contains unit tests for the undo/redo history library.

## Test Files

| File | Description |
|------|-------------|
| **basics.cpp** | Tests for basic undo/redo operations including command execution, history management, canUndo/canRedo state tracking |
| **tree.cpp** | Tests for undo tree structures (branching undo history) |
| **complex_tree.cpp** | Tests for complex undo tree scenarios with multiple branches |

## Test Infrastructure

- **CMakeLists.txt** - CMake configuration for building and running tests via CTest
- **test.h** - Test helper macros (EXPECT_TRUE, EXPECT_FALSE, EXPECT_EQ, etc.)
- **cmd.h** - Test command implementation used to simulate undoable operations

## Purpose

These tests verify the correctness of the undo library's history management functionality, including linear undo/redo operations and branching undo trees. They are included as part of the forked/vendored third-party code.

# Clip Library CI Workflows

*Last updated: 2025-12-30*

This directory contains GitHub Actions workflow definitions for the clip clipboard library.

## Workflows

### build.yml

Continuous integration workflow that runs on push and pull request events. It performs:

- **Multi-platform builds**: Windows, macOS, and Ubuntu (latest versions)
- **Makefile generation**: Uses CMake to generate platform-appropriate makefiles (NMake for Windows, Unix Makefiles for Linux/macOS)
- **Compilation**: Builds the library using the appropriate make tool
- **Test execution**: Runs CTest with output on failure (uses xvfb-run on Linux for display server requirements)
- **MinGW builds**: Additional Windows builds using MSYS2 with multiple toolchains (mingw64, mingw32, ucrt64, clang64)

## Purpose

These CI workflows are used by the upstream clip library repository. They are included here as part of the forked/vendored third-party code but are not executed by the LibreSprite CI pipeline.

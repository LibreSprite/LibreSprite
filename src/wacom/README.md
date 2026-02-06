# Wacom Module

*Last updated: 2025-12-30*

Wintab API definitions for Wacom tablet support on Windows.

## Overview

Contains header files from the Wintab SDK for interfacing with Wacom tablets. These headers define the protocol for receiving pen pressure, tilt, and other tablet-specific input data.

## Files

| File | Purpose |
|------|---------|
| `wintab.h` | Core Wintab API types, constants, and message definitions |
| `pktdef.h` | Packet definition macros for tablet data |
| `msgpack.h` | Message packing utilities |

## Key Concepts

- **WTPKT** - Packet mask bits (pressure, tilt, buttons, position)
- **HCTX** - Tablet context handle
- **FIX32** - Fixed-point type for tablet coordinates

## License

Copyright Wacom Company, Ltd. 2010. Text and information may be freely used without compensation (see MIT-license.txt).

## Dependencies

Windows-only. Used by: she (platform abstraction layer)

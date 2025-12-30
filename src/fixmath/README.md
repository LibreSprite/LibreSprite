# Fixmath Library

*Last updated: 2025-12-30*

Fixed-point arithmetic library for precise, deterministic math operations.

## Overview

Based on the Allegro library by Shawn Hargreaves. Uses 16.16 fixed-point representation (16 bits integer, 16 bits fraction) stored in 32-bit integers.

## Key Functions

| Function | Purpose |
|----------|---------|
| `ftofix(double)` | Convert float/double to fixed |
| `fixtof(fixed)` | Convert fixed to double |
| `itofix(int)` | Convert integer to fixed |
| `fixtoi(fixed)` | Convert fixed to integer |
| `fixmul`, `fixdiv` | Multiplication and division |
| `fixadd`, `fixsub` | Addition and subtraction with overflow detection |
| `fixsqrt`, `fixhypot` | Square root and hypotenuse |
| `fixsin`, `fixcos`, `fixtan` | Trigonometric functions (lookup table based) |
| `fixasin`, `fixacos`, `fixatan` | Inverse trigonometric functions |

## Why Fixed-Point?

- Consistent behavior across platforms (no floating-point variance)
- Suitable for pixel-precise graphics operations
- Efficient on hardware without FPU

## Dependencies

Depends on: base

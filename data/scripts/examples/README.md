# LibreSprite Script Examples

This directory contains example JavaScript scripts demonstrating the LibreSprite scripting API capabilities.

## Example Scripts

| Script | Description |
|--------|-------------|
| `Heightmap.js` | Generates procedural terrain heightmaps using noise algorithms and hydraulic erosion simulation. Supports export to STL format for 3D printing. |
| `PerLineOscillation.js` | Creates animated oscillation effects on sprites with multiple modes: horizontal/vertical waves, anti-aliased variants, and RGB channel desync for glitch effects. |
| `Voxel.js` | Converts layered sprites into 3D voxel models using marching cubes. Includes a real-time 3D preview with rotation and STL export. |
| `ai.js` | Integrates with AI image generation services (EasyDiffusion and lorastudio.co) for text-to-image generation directly within LibreSprite. |
| `http.js` | Demonstrates HTTP requests using the `storage.fetch()` API to retrieve remote content. |

## Key Concepts Demonstrated

- **Dialog creation**: Using `app.createDialog()` to build custom UI with buttons, labels, entries, and image views
- **Image manipulation**: Reading/writing pixel data via `getImageData()` and `putImageData()`
- **Animation**: Creating multi-frame animations by iterating through frames
- **Event handling**: Using the `onEvent()` callback pattern for UI interactions
- **Storage API**: Fetching remote data, saving/loading files, and Base64 encoding
- **Command execution**: Running LibreSprite commands like `NewFrame`, `ChangePixelFormat`

## Running Scripts

1. Open LibreSprite
2. Go to **File > Scripts > Run Script** (or use the Scripts menu)
3. Select a script from this examples directory
4. Follow the script's dialog prompts

Alternatively, place scripts in the user scripts folder and they will appear in the Scripts menu automatically.

## API Reference

For the complete scripting API documentation, see [SCRIPTING.md](../../SCRIPTING.md) in the project root.

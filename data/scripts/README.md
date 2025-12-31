# Bundled Scripts

This directory contains JavaScript scripts that ship with LibreSprite. Scripts extend LibreSprite's functionality by manipulating images, generating content, connecting to external services, and more.

## How Scripts Work

Scripts are executed via LibreSprite's JavaScript engine. They interact with the application through the global `app` object, which provides access to:

- `app.activeSprite` - The currently open sprite
- `app.activeImage` - The active image/cel
- `app.pixelColor` - Color manipulation utilities (rgba, extraction methods)
- `app.command` - Execute built-in commands
- `app.createDialog()` - Create UI dialogs

Scripts communicate with LibreSprite through an event-driven pattern. Each script defines an `onEvent(eventName)` function that handles events like `init` (script startup) and UI callbacks.

## Available Scripts

| Script | Description |
|--------|-------------|
| `Random.js` | Fills the active image with random grayscale noise |
| `white_to_alpha.js` | Converts white pixels to transparency based on brightness |

## Examples Directory

The `examples/` subdirectory contains more advanced scripts demonstrating various capabilities:

| Script | Description |
|--------|-------------|
| `Heightmap.js` | Generates procedural terrain heightmaps with erosion simulation; exports to STL |
| `PerLineOscillation.js` | Creates wave distortion animations with multiple effect modes |
| `Voxel.js` | Converts layered sprites to 3D voxel models with real-time preview; exports to STL |
| `ai.js` | Integrates with EasyDiffusion and LoraStudio for AI image generation |
| `http.js` | Demonstrates HTTP fetching using the `storage` API |

## Script Structure

A minimal script follows this pattern:

```javascript
// Access the scripting API
const col = app.pixelColor;
const img = app.activeImage;

// Manipulate pixels
for (var y = 0; y < img.height; ++y) {
    for (var x = 0; x < img.width; ++x) {
        var c = img.getPixel(x, y);
        // Process pixel...
        img.putPixel(x, y, col.rgba(r, g, b, a));
    }
}
```

For scripts with UI dialogs:

```javascript
const eventHandlers = {
    init: function() {
        dialog = app.createDialog('myDialog');
        dialog.addEntry("Label:", "fieldId");
        dialog.addButton("Run", "run");
    },
    run_click: function() {
        // Handle button click
    },
    myDialog_close: function() {
        // Cleanup on close
    }
};

function onEvent(eventName) {
    var handler = eventHandlers[eventName];
    if (typeof handler == 'function')
        handler();
}
```

## Full API Reference

See [SCRIPTING.md](/SCRIPTING.md) in the repository root for complete API documentation including all classes, properties, and methods.

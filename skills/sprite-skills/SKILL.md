---
name: sprite-skills
description: Operate LibreSprite as the pixel-art source of truth for a 16-bit JRPG through the libresprite MCP server. Use when inspecting, validating, editing or exporting .ase/.aseprite sprites, tilesets, spritesheets or palettes, or when asked to check art against the master palette or the tile grid.
---

# Sprite pipeline

LibreSprite is the single place where this project's art is validated, formatted and exported.
Every `.ase` in the assets root is authoritative; PNGs and atlases are build artefacts and may be
regenerated at any time. Never hand-edit an exported PNG and treat it as source.

The `libresprite` MCP server drives a headless LibreSprite in a container. Tools operate on paths
relative to the assets root; nothing outside that directory is reachable.

## The loop

Work in this order. It is not a suggestion — steps 1 and 5 catch most mistakes.

0. **`create_sprite`** — start a blank sprite, or **`convert_to_pixel_art`** to turn an existing
   drawing/photo into a small palette-limited sprite (writes a new file, never the source).
1. **`sprite_info`** — dimensions, colour mode, frame count and every layer. Do this before
   anything else. Layer indices and names you assume are usually wrong.
2. **`validate_palette`** — check the art against the master palette *before* you accept or edit
   it. If it already fails, say so rather than silently building on top of a broken asset.
3. **Mutate** — `layer_ops`, `pixel_ops`. These write the `.ase` in place.
4. **Export** — `export_spritesheet` for animation, `export_png` for single images,
   `resize_sprite` for scale/crop/trim into a new file.
5. **`render_preview`** — actually look at the result. A tool returning success only means no
   exception was thrown; it does not mean the art is right. Note it shows a *single layer*, not
   the composite — for the finished picture, `export_png` and read that.

## Project conventions

| Thing | Convention |
|---|---|
| Canvas | Whole multiples of the 16×16 tile grid. `validate_palette` enforces via `tileWidth`/`tileHeight`. |
| Palette | One master palette file per art set. Enforce with `validate_palette`; locking a sprite to INDEXED is a GUI step. |
| Colour budget | 16-bit era discipline: keep unique colours per sprite low and deliberate. Pass `maxColors` to enforce. |
| Layer names | Descriptive and stable (`base`, `outline`, `shading`, `fx`). Tools address layers by name or index; layers can only be renamed headless, not restructured. |
| Animation | Every animation is a frame tag. `list_tags` first, then `export_spritesheet` with `frameTag`. |
| Atlas format | `format: "json-array"` with `listTags` and `listLayers` enabled, so the engine gets `meta.frameTags` and `meta.layers`. |

## Constraints that will bite you

- **Indexed sprites store palette indices, not colours.** `pixel_ops` refuses them unless you pass
  `allowIndexed: true` *and* give values as `{ index: n }`. Check `colorMode` from `sprite_info`
  first. Converting to RGB to draw and back to indexed afterwards loses the index mapping — prefer
  editing indices directly.
- **Edit `.ase`, never `.png`.** `layer_ops` on a PNG reports success and changes nothing, because
  PNG stores no layer names or visibility. Convert first, or refuse.
- **Layers cannot be added, removed, merged or flattened headless.** `layer_ops` does renames and
  visible/editable flags only; the rest is rejected by the schema. There is no workaround — it is a
  GUI job (`docker compose --profile gui up`). Do not try to fake it by flattening or re-exporting.
- **`quantize_to_palette` rewrites every pixel in place, with no undo.** It snaps colours to the
  master palette but leaves the sprite RGB — converting to INDEXED is impossible headless. Run
  `validate_palette` first, and say what will change before you run it.
- **Converted art is a draft, not an asset.** `convert_to_pixel_art` gives a mechanically correct
  sprite: right size, right palette. It is not hand-quality pixel art. Present it as a blockout or
  reference and say so — do not pass it off as finished.
- **`resample: area` for drawings/photos, `nearest` for art that is already pixel art.** `nearest`
  never invents a colour, so it preserves an existing palette; on smooth art it produces noise.
- **`pixel_ops` writes one pixel at a time.** Thousands are fine, millions are not. For wholesale
  image replacement, use the GUI.
- **`render_preview` shows one layer, not the composite.** On a multi-layer sprite it is a partial
  view, and it says so. To see the finished picture, `export_png` and read the file.
- **`resize_sprite` writes to a new file** and refuses to overwrite the source. Scaling through the
  script API hangs, so this CLI-backed tool is the only route.
- **`export_png` with `splitLayers` or multiple frames needs `filenameFormat`** or files overwrite
  each other. Tokens: `{path} {title} {layer} {frame} {extension}`.
- **`list_layers` only shows visible layers** unless `allLayers: true`. `sprite_info` always shows
  all of them, with visibility flags — prefer it.

## Recipes

**Accept a new character sprite**
```
sprite_info                → confirm 16-grid canvas, layer names, colour mode
validate_palette           → allowed: <master palette>, tileWidth: 16, tileHeight: 16, maxColors: 16
render_preview             → look at it
```

**Export a walk cycle for the engine**
```
list_tags                              → find the tag, e.g. "walk_down"
export_spritesheet                     → frameTag: "walk_down", type: "rows",
                                         data: "atlas/hero_walk.json", format: "json-array"
```
`type: "rows"` keeps frames in readable order; `"packed"` is denser but the layout is opaque.

**Recolour to the master palette**
```
validate_palette   → see exactly which colours are off, with counts and coordinates
(ask the user which allowed colour each offender should become)
pixel_ops          → replace_color per offender
validate_palette   → confirm it now passes
```
Locking the sprite to INDEXED against the master palette is a GUI step; there is no headless tool
for it.

**Fix a sprite that fails the colour budget**
Report the offending colours to the user with counts (`validate_palette` returns `usedColors`
sorted by frequency) and ask which to merge. Do not silently pick replacements — colour choices are
art direction, not a mechanical fix.

## When not to use these tools

Freehand drawing, dithering, anti-aliasing decisions and anything needing a human eye belong in the
GUI. Bring up `docker compose --profile gui up` and hand it to the user. These tools are for
inspection, validation, mechanical edits and repeatable exports.

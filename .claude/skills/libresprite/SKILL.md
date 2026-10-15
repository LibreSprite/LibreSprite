---
name: libresprite
description: Create, edit, inspect and export pixel art and sprite animations with LibreSprite through the `lsprite` CLI (headless). Use for any pixel art, sprite, tileset, icon, spritesheet, .ase/.aseprite/.png sprite file, palette or frame animation work, and whenever the user mentions LibreSprite or Aseprite-style editing.
---

# LibreSprite via `lsprite`

`tools/lsprite` drives LibreSprite headlessly (`build/bin/libresprite`, or `libresprite` on PATH, with `data/scripts/cli.js`). Build first: see INSTALL.md. Add `tools` to PATH or call `tools/lsprite`. Every call prints one JSON line: `{"ok": true, ...}` or `{"ok": false, "error": ...}` (exit 1).

## Rules

- Headless by default. Open the GUI (`lsprite gui FILE`) only when the user asks to see or work in the app.
- You must look at your work. Mutating commands print a `preview` PNG path (upscaled, animations as a left-to-right strip). Read that PNG before the next edit and fix what looks wrong. A hook reminds you.
- Each call takes about 2-3 s, so batch: put many ops into one `draw` call.
- Work in `.ase` (keeps layers, frames, palette). Export PNG or a sheet at the end.
- Edits save in place. Use `--out PATH` to write a copy instead.

## Commands

```
lsprite new FILE --w 32 --h 32 [--bg COLOR]
lsprite info FILE                                  # size, mode, frames, layers
lsprite render FILE [--frame N] [--scale N|auto] [--out PNG]
lsprite pixels FILE [--layer N --frame N --x --y --w --h] [--ascii]
lsprite draw FILE --ops JSON|@ops.json|- [--layer N --frame N]
lsprite palette FILE [--set '{"0":"#1a1c2c"}'] [--load file.gpl]
lsprite layer list|add|rename|show|hide|lock|unlock FILE [--index N --name S]
lsprite frame list|add|remove FILE [--at N] [--copy FROM] [--index N]
lsprite resize FILE --w N --h N          # canvas size, keeps pixels
lsprite crop FILE --x --y --w --h
lsprite flip FILE [--vertical]
lsprite mode FILE rgb|grayscale|indexed [--dithering ordered]
lsprite sheet FILE --out sheet.png [--data sheet.json]
lsprite commands | command-info ID | cmd FILE ID [name=value ...]
lsprite eval FILE --code 'JS' [--no-save]  # full script API, `sprite`, `app`, `lsprite` helpers
lsprite run FILE script.js [--no-save]
lsprite gui [FILE]
```

Add `--no-preview` to skip the preview render, for example in the middle of a batch.

Colors: `#RGB`, `#RRGGBB`, `#RRGGBBAA`, `[r,g,b,a]`, `transparent`. Indexed sprites accept only colors that are in the palette.

## Draw ops

`--ops` takes one op object or an array of them. Coordinates are sprite pixels, origin top-left.

| op | fields |
|---|---|
| `ascii` | `x, y, rows:["..##"], map:{"#":"#e43b44"}` (unmapped chars are skipped) |
| `pixel` | `x, y, c` |
| `pixels` | `points:[[x,y],[x,y,"#hex"]], c` |
| `line` | `x0, y0, x1, y1, c` |
| `polyline` | `points:[[x,y],...], c, closed` |
| `rect` | `x, y, w, h, c, fill` |
| `ellipse` | `cx, cy, rx, ry, c, fill` |
| `fill` | `x, y, c` (flood fill on the cel) |
| `replace` | `from, to` |
| `clear` | `c` (default transparent) |

`ascii` is the most reliable way to place a designed shape. Write the sprite as rows, then refine with other ops. `pixels --ascii` returns the same format, so you can read art back, edit the rows and redraw them.

## Workflow

1. Plan the sprite: size, a small palette (4-16 colors), and the silhouette.
2. `lsprite new` the file, then `draw` with `ascii` for shapes and outlines.
3. Read the preview, check the silhouette, readability, contrast and stray pixels, and iterate.
4. For animation: `frame add --copy N`, then edit `--frame N+1`. Check the strip preview.
5. Export with `render --scale 1 --out x.png`, or `sheet --out sheet.png --data sheet.json`.

## Anything else

- `lsprite commands` lists the 130+ LibreSprite commands. `command-info ID` shows a command's params, and `cmd FILE ID k=v` runs it. Commands that open dialogs do not work headless.
- `eval` and `run` expose the whole script API (`sprite.layer(i).cel(f).image.getPixel/putPixel/getImageData`, `sprite.addEmptyFrame/copyFrame/removeFrame`, `layer.createCel`, `app.command.*`). The API reference is `SCRIPTING.md` in the repo. Return a value to print it.
- The JS runtime is QuickJS: no `require`, `import` or `process`. A hook checks script syntax.

## Maintenance

- Rebuild after C++ changes: `ninja -C build libresprite`.
- `data/scripts/cli.js` runs inside LibreSprite. `tools/lsprite` is the Python wrapper. Overrides: `LSPRITE_BIN`, `LSPRITE_SCRIPT`, `LSPRITE_REPO`, `LSPRITE_PREVIEWS`.
- Hooks in `.claude/settings.json`: after an `lsprite` call they point to the preview PNG; after an edit they run `node --check` on `.js` and `-fsyntax-only` on `.cpp` (needs `build/compile_commands.json`, configure with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`).

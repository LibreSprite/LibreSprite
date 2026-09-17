# libresprite-mcp

MCP server that exposes LibreSprite as a pixel-art source of truth. The server runs on the host and
drives a headless LibreSprite in an ephemeral container per tool call.

```
LLM ──stdio──▶ libresprite-mcp ──▶ docker run --rm --network none
                                     -v <assets>:/work
                                     libresprite:mcp --batch [flags | --script /scripts/op.js]
```

## Setup

```sh
# 1. Build the image (needs network: three git submodules are fetched)
docker build -t libresprite:mcp .

# 2. Build the server
cd mcp/libresprite-mcp && npm install && npm run build
```

The repo's `.mcp.json` registers the server with `LIBRESPRITE_ASSETS_ROOT=workspace`.

| Variable | Default | Meaning |
|---|---|---|
| `LIBRESPRITE_ASSETS_ROOT` | cwd | Host directory holding the art. Nothing outside is mounted or reachable. |
| `LIBRESPRITE_IMAGE` | `libresprite:mcp` | Image tag. |
| `LIBRESPRITE_LS_MODE` | `batch` | `batch` starts Xvfb; `dummy` uses SDL's dummy video driver (faster, unverified). |
| `LIBRESPRITE_TIMEOUT_MS` | `120000` | Per-call timeout. |

## Tools

| Tool | Route | Writes |
|---|---|---|
| `create_sprite` | host + CLI | new file |
| `convert_to_pixel_art` | host + CLI | new file |
| `quantize_to_palette` | script | the sprite |
| `sprite_info` | script | no |
| `list_layers`, `list_tags` | native CLI | no |
| `render_preview` | script | no |
| `get_palette`, `validate_palette` | script | no |
| `export_png`, `export_spritesheet`, `resize_sprite` | native CLI | output files |
| `layer_ops`, `pixel_ops` | script | the sprite |

"Native CLI" means plain LibreSprite flags. "Script" means a generated ES5 file run via `--script`.
Native is preferred wherever the flags cover the job — faster, and it avoids the scripting
constraints below.

## Converting a drawing into a sprite

`convert_to_pixel_art` resamples on the host and then snaps every colour to a master palette.
Measured on a 256x256 gradient drawing with 45 218 colours -> 32x32:

| resample | result |
|---|---|
| `area` + 16-colour palette | 12 colours, passes `validate_palette` |
| `nearest` + 16-colour palette | 11 colours |
| `area`, no palette | 765 colours — resampled only, **not** pixel art yet |

Pick `area` for drawings and photos: it averages each source block, so shape and mass survive.
Pick `nearest` when rescaling art that is *already* pixel art — it samples one pixel per block and
therefore never invents a colour. This matches LibreSprite's own `--scale`
(`RESIZE_METHOD_NEAREST_NEIGHBOR`, `cmd_sprite_size.cpp:258`).

Pixels are handled on the host, not in the container: reading a 256x256 image one `getPixel()` at a
time through Duktape takes ~20 s. `src/png.ts` is a dependency-free PNG codec (verified
byte-identical on round-trip); the container is used only for the `.ase` format, which is the one
thing it uniquely provides.

A layered `.ase` source is composited on the way in — verified against `data/splash.ase` (54 layers,
three hidden): the conversion reads the true composite, not the bottom layer, and excludes hidden
layers unless `allLayers: true`. The output always has one layer.

**This produces a mechanically correct sprite, not hand-quality pixel art.** Good pixel art is drawn
pixel by pixel. Treat the output as a draft, a colour blockout, or a reference — not a finished
asset.

`quantize_to_palette` does the palette-snapping half alone, in place, preserving layers and frames
(it runs in-script via `getPixel`/`putPixel`). It does not change colour mode — `ChangePixelFormat`
segfaults headless — so the sprite stays RGB while using only palette colours, which is exactly what
`validate_palette` checks.

Both quantisers drop palette entries with alpha 0 before matching. Without that, a `#00000000` entry
reaches the matcher as `{0,0,0}` and drags dark pixels onto an opaque black that is not in the
palette at all.

## Headless capability matrix

Measured against the built image, not inferred. A `--script` run can do far less than the GUI:

| | Status |
|---|---|
| Read sprite/layer/cel/palette/pixel data | works |
| `Image.getPNGData()` | works |
| `Layer.name`, `.isVisible`, `.isEditable` writes | works |
| `Image.putPixel` / `.clear`, then `sprite.save()` | works, verified by read-back |
| `sprite.resize()`, `sprite.loadPalette()` | **hangs** — the `app::Transaction` path |
| `NewLayer`, `FlattenLayers`, other document-modifying commands | **refused**, return 0 (`command_script.cpp:31` gates on `isEnabled`) |
| `ChangePixelFormat` | **segfaults** |
| Native CLI `--scale`, `--trim`, `--crop`, `--sheet`, `--save-as`, `--list-*` | works |

Two consequences worth stating plainly:

- **No colour-mode conversion.** `ChangePixelFormat` segfaults. `quantize_to_palette` snaps colours
  to the palette but leaves the sprite RGB.
- **`layer_ops` cannot add, remove, merge or flatten layers** — only rename and toggle flags. The
  schema rejects the rest rather than failing at runtime.
- **`render_preview` cannot composite.** It renders one layer. `export_png` produces the real
  flattened image.

Interestingly the same commands *do* work on the native CLI route, because `app.cpp` executes them
without the `isEnabled` gate — hence `resize_sprite` exists as a CLI-route tool.

### Argument order is load-bearing

`--scale` needs an already-open document, so the input file must come *first*. Verified: with the
file after the flag the scale is silently ignored (output stays 32×32); with the file first it
produces 64×64. `src/tools/export.ts` always emits modifier flags → input file → `--scale` →
`--save-as`.

### Edit `.ase`, not `.png`

`layer_ops` on a PNG reports success and changes nothing on disk — PNG stores no layer names or
visibility. Layer edits only round-trip through `.ase`/`.aseprite`.

## Why the scripts look like 2009 JavaScript

Everything in `src/templates.ts` is ES5.1 with `var` and no arrow functions, because the container
runs Duktape. `npm run build` parses every template with `acorn --ecmaVersion 5` and fails the build
if anything modern slips in.

Constraints verified against the LibreSprite source:

- **`--batch` is not headless.** `src/she/sdl2/she.cpp:967` calls `SDL_Init(SDL_INIT_VIDEO)`
  unconditionally, before any flag is parsed, and aborts if it fails. Hence Xvfb.
- **No `--script-param`.** This is Aseprite-1.1-era code; the full flag list is
  `src/app/app_options.cpp:28-59`. Parameters are injected as a `PARAMS` literal at the top of the
  generated file.
- **No event loop in batch.** `app.yield()` and script event listeners are pumped by a `ui::Timer`
  (`src/app/task_manager.h:108`) that never ticks without a UI. Scripts must be straight-line.
- **Three APIs segfault in batch** by dereferencing a null `ui::Manager`:
  `image.putImageData()` (`image_script.cpp:74`), `palette.set()` and `palette.length =`
  (`palette_script.cpp:59`), and `app.redraw()` (`app_script.cpp:116`). Pixel writes therefore use
  `putPixel`/`clear`; `sprite.loadPalette()` is unusable too (it hangs).
- **`sprite.crop()` is a no-op** — its body is commented out (`sprite_script.cpp:135-150`). The
  `--crop` CLI flag works.
- **`app.activeSprite`, `activeDocument` and `activeImage` are always null in batch.** They read
  through `app::current_editor` (`app_script.cpp:60-71`), and there is no editor. Scripts get their
  sprite from `app.open(file).sprite` instead.
- **Commands can only target layer 0 in batch.** Without an editor, `UIContext::onGetActiveSite`
  hands out a dummy site with `layer(LayerIndex(0))` hard-coded (`ui_context.cpp:223-238`), and
  `app.activeLayerNumber` reads back 0 regardless. Combined with the `isEnabled` gate this makes
  layer commands unusable, so they are not exposed at all. Property writes (`name`, `isVisible`,
  `isEditable`) go through the `Layer` object and work on any layer.
- **Argument order matters.** LibreSprite processes options and filenames positionally
  (`src/app/app.cpp:214-639`) and `--save-as` targets `documents().lastAdded()` (`app.cpp:355-360`).
  `src/tools/export.ts` assembles argv in a fixed canonical shape.

## Safety

- Paths are resolved and confined to `LIBRESPRITE_ASSETS_ROOT`, including through symlinks
  (`src/paths.ts`). Only that directory is mounted.
- Containers run with `--network none`, which also neutralises the script API's `storage.fetch()`.
- Containers run as `--user $(id -u):$(id -g)` so output lands owned by the caller.
- Writes to the same file are serialised by a per-path lock (`withFileLock` in `src/docker.ts`),
  since each call is a separate container and would otherwise race.

## Development

```sh
npm run build       # tsc + ES5 template check
npm run check:es5   # ES5 check alone
npm run inspect     # MCP Inspector against the built server
```

## Known gaps

- No colour-mode conversion to INDEXED (`ChangePixelFormat` segfaults). `quantize_to_palette` gets
  you palette-conformant RGB, which is usually enough.
- No layer add/remove/merge/flatten.
- No compositing in `render_preview`.
- No tool for importing a full PNG into a cel. `putImageData` is unusable in batch, and a
  `putPixel` loop over a large image is too slow to be worth exposing.
- Frame count is derived from the highest cel frame across layers — `Sprite` exposes no
  `frameCount`.
- Layer opacity and blend mode are not in the script API and cannot be set.

`LS_MODE=dummy` is confirmed working (`--version` succeeds with no X server), so Xvfb is a safety
net rather than a hard requirement. It has not been exercised across every tool; `batch` remains
the default. A typical script call round-trips in ~300 ms.

# Headless pixel-art pipeline (Docker + MCP)

This fork adds a way to run LibreSprite **without a GUI**, inside a container, and to drive it from
an AI assistant over the [Model Context Protocol](https://modelcontextprotocol.io). The goal is to
make LibreSprite the single source of truth for a 16-bit JRPG's art: the place where sprites are
validated, formatted and exported, instead of art drifting across a folder of loose PNGs.

Nothing in `src/` was modified. This is additive: a container, a server that talks to it, and some
assistant-facing configuration.

> **Read the caveats.** Roughly half of what LibreSprite's scripting API appears to offer does not
> work headless — some of it hangs, some of it segfaults. That only became visible after running it.
> The [Caveats](#caveats) section is the most important part of this document.

---

## What was added

| Area | Files | What it does |
|---|---|---|
| Container | `Dockerfile`, `docker-compose.yml`, `docker/entrypoint.sh`, `.dockerignore` | Builds LibreSprite and runs it headless, or as a GUI over noVNC |
| MCP server | `mcp/libresprite-mcp/` | Node/TypeScript server exposing 14 tools; runs on the host, drives the container |
| Assistant config | `skills/sprite-skills/`, `agents/`, `.claude/`, `.mcp.json` | A skill describing the workflow, three agents, and server registration |

Deep technical detail lives in [`mcp/libresprite-mcp/README.md`](mcp/libresprite-mcp/README.md).
This file covers the shape of the thing and the things that will bite you.

---

## Running it

```sh
# 1. Build the image. Needs network (three git submodules are fetched) and 15-40 min.
docker build -t libresprite:mcp .

# 2. Build the server.
cd mcp/libresprite-mcp && npm install && npm run build
```

`.mcp.json` registers the server against `LIBRESPRITE_ASSETS_ROOT=workspace`. Only that directory is
mounted into the container; nothing outside it is reachable.

For hand editing, the same image runs a full GUI in the browser:

```sh
docker compose --profile gui up     # http://localhost:6080/vnc.html
```

---

## Why it is built this way

**One throwaway container per tool call.** `docker run --rm` rather than a long-lived daemon. No
state leaks between calls, and a crashed operation cannot poison the next one. Measured cost is
~300 ms per call, which is cheap enough not to bother optimising.

**Pixels are processed on the host, not in the container.** Reading a 256×256 image one `getPixel()`
at a time through the embedded script engine takes ~20 s. So `mcp/libresprite-mcp/src/png.ts` is a
dependency-free PNG codec, and the container is used only for the thing it uniquely provides: the
`.ase` format. Resampling and palette matching happen in TypeScript.

**Generated scripts are ES5.** The image compiles Duktape, not V8 (V8 is only picked up when
`libnode-dev` is present, which it deliberately is not). A stray arrow function fails at runtime
inside a container where the error is nearly unreadable, so `npm run build` parses every template
with `acorn --ecmaVersion 5` and fails the build instead.

**Two routes, native preferred.** Where LibreSprite's own CLI flags cover a job (`--sheet`,
`--save-as`, `--scale`, `--trim`, `--crop`, `--list-*`), the server uses them. They are faster and
they sidestep every scripting limitation below. The script route is used only for what flags cannot
express.

**`--batch` is not headless.** `src/she/sdl2/she.cpp:967` calls `SDL_Init(SDL_INIT_VIDEO)`
unconditionally, before any command-line flag is parsed, and aborts if it fails. A video subsystem is
therefore mandatory even for pure file conversion — hence Xvfb in the entrypoint.

---

## Headless capability matrix

Measured against the built image. Not inferred from the source.

| Operation | Status |
|---|---|
| Read sprite / layer / cel / palette / pixel data | works |
| `Image.getPNGData()` | works |
| `Layer.name`, `.isVisible`, `.isEditable` writes | works |
| `Image.putPixel` / `.clear`, then `sprite.save()` | works, verified by read-back |
| `sprite.resize()`, `sprite.loadPalette()` | **hangs** (the `app::Transaction` path) |
| `NewLayer`, `FlattenLayers`, other document-modifying commands | **refused**, return 0 — `command_script.cpp:31` gates on `isEnabled` |
| `ChangePixelFormat` | **segfaults** |
| Native CLI `--scale`, `--trim`, `--crop`, `--sheet`, `--save-as`, `--list-*` | works |

The same commands *do* work through the native CLI, because `app.cpp` executes them without the
`isEnabled` gate. That asymmetry is why `resize_sprite` exists as a CLI-route tool rather than a
script one.

---

## Caveats

### Things that are simply not possible headless

- **No conversion to INDEXED colour mode.** `ChangePixelFormat` segfaults. `quantize_to_palette`
  gets you an RGB sprite that uses only palette colours — which is what `validate_palette` checks —
  but the colour mode stays RGB.
- **Layers cannot be added, removed, merged or flattened.** `layer_ops` renames layers and toggles
  their visible/editable flags; everything else is rejected by the schema rather than failing at
  runtime. Structural layer work is a GUI job.
- **`render_preview` cannot composite.** It renders a single layer and says which one. For the real
  flattened image, use `export_png` and look at the file.
- **`app.activeSprite` / `activeDocument` / `activeImage` are always null.** They resolve through
  `app::current_editor`, and there is no editor. Scripts obtain their sprite via
  `app.open(file).sprite`.

### Things that look like they worked but did not

- **`layer_ops` on a `.png` reports success and changes nothing.** PNG stores no layer names or
  visibility. Layer edits only round-trip through `.ase`/`.aseprite`. Check the extension first.
- **Argument order is semantic.** `--scale` operates on already-open documents, so the input file
  must come *before* it. With the flag first the scale is silently ignored — measured as 32×32
  output where 64×64 was expected. `src/tools/export.ts` always emits
  modifiers → input file → `--scale` → `--save-as`.

### Quality and scope

- **`convert_to_pixel_art` flattens the source.** A layered `.ase` is composited on the way in, so
  the output always has exactly one layer. Verified against `data/splash.ase` — 320x160, INDEXED,
  54 layers, three of them hidden: the conversion reads the true composite (byte-identical to an
  independently reconstructed downscale of the CLI's own composite export), not just the bottom
  layer. Hidden layers are excluded by default, matching what you see in the editor; pass
  `allLayers: true` to include them.

  Flattening is inherent to the operation, not a defect — but it does mean this tool cannot be used
  to rework a layered sprite in place. Use it to bring outside art *in*, then keep the layered
  `.ase` as the source of truth.
- **Converted art is a draft, not a finished asset.** The pipeline produces a mechanically correct
  sprite: right size, right palette. Good pixel art is drawn pixel by pixel. Present converted output
  as a blockout or reference. The tool descriptions and `skills/sprite-skills/SKILL.md` say this
  explicitly so the assistant does not oversell it.
- **`pixel_ops` writes one pixel at a time.** Thousands are fine; a full 32×32 rewrite is ~300 ms.
  Millions are not. Wholesale image replacement belongs in the GUI.

### Operational

- **Building the image needs network and 15–40 minutes.** `third_party/simpleini`,
  `third_party/duktape` and `src/flic` are uninitialised submodules fetched at build time.
- **`dist/` is not committed.** Run `npm install && npm run build` in `mcp/libresprite-mcp/` before
  first use.
- **`workspace/` is gitignored, so no test fixtures ship with the repo.** To reproduce the
  verification runs, create `workspace/test/` with a small PNG and a `.gpl` palette, then convert it:
  `docker run --rm -v "$PWD/workspace:/work" libresprite:mcp --batch /work/test/hero.png --save-as /work/test/hero.ase`
- **`.claude/agents` and `.claude/skills/sprite-skills` are symlinks.** On Windows they need
  `git config core.symlinks=true` before cloning, or they land as plain text files.
- **`LS_MODE=dummy` works** — LibreSprite starts with SDL's dummy video driver and no X server at
  all, so Xvfb is a safety net rather than a hard requirement. It has only been exercised on
  `--version`, so `batch` remains the default.

### Security posture

Containers run with `--network none` (which also neutralises the script API's `storage.fetch()`) and
`--user $(id -u):$(id -g)` so output is owned by the caller rather than the image's uid 1000. All
caller-supplied paths are resolved and confined to `LIBRESPRITE_ASSETS_ROOT`, including through
symlinks; traversal attempts are rejected before reaching the container. Concurrent writes to the
same file are serialised by a per-path lock, because each call is a separate container and would
otherwise race.

---

## A bug worth recording

The two quantisers — one on the host (`src/raster.ts`), one in-script (`src/templates.ts`) —
disagreed. The script path serialised the palette as `{r,g,b}` and dropped alpha, so a fully
transparent entry like `#00000000` arrived as `{0,0,0}` and became a valid match target, pulling dark
pixels onto an opaque black that was not in the palette at all. The host path already filtered it.

It surfaced only because both paths were run against the same palette and the results compared:
`convert_to_pixel_art` reported `valid: true` while `quantize_to_palette` reported `valid: false`.
Either test alone would have passed. Both paths now filter alpha-0 entries before matching.

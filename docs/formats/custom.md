# Custom file formats in script

LibreSprite can load and save file formats implemented in script. A script
registers a format with `file.registerFormat()`; when a file with one of the
format's extensions is opened or saved, the app raises a `format_load` /
`format_save` event to the script. The script's event handler decodes or
encodes the file and replies through the `file.onLoad` / `file.onSave` API.

A complete working example: [data/scripts/formats/ppm.js](../../data/scripts/formats/ppm.js)
(a PPM/PGM format, loaded at startup via `///listen app_boot`).

## Registering a format

    file.registerFormat({
      name: "PPM/PGM",        // shown in the file dialogs
      extensions: "ppm,pgm",  // comma-separated, no dots
      load: true,
      save: true,
    });

The registration is bound to the calling script's canonical path: the app
routes `format_load` / `format_save` events back to that script. Register at
the top level (a re-run of the script re-registers with any changed spec).

A format script convention:

    ///listen app_boot

    // ... codec code ...

    file.registerFormat({ name: "...", extensions: "...", load: true, save: true });

    function onEvent(name, id, path, payload) {
      if (name === "format_load")
        app.critical(() => decode(id, payload));
      else if (name === "format_save")
        app.critical(() => encode(id, path, payload));
    }
    setEventHandler(onEvent);

The `///listen app_boot` header makes the app load the script at startup so
the format is registered before any file is opened. A top-level `function
onEvent` is registered as the event handler automatically (backward
compatibility); an explicit `setEventHandler(onEvent)` also works and is
needed when the handler is not named `onEvent`.

If a script drives open/save of its own format from within its own run (for
example a batch script that registers the format and then calls
`command.OpenFile` / `command.SaveFileAs`), it must call
`setEventHandler(onEvent)` before the command call: the automatic
registration runs after the top-level body, so the handler is not registered
yet while the body runs.

## Codec events

The event handler receives the event name first, followed by the event's
payload (the same convention as all script events):

- `format_load` - `onEvent("format_load", id, path, data)`. `data` is a
  `Uint8Array` with the whole file.
- `format_save` - `onEvent("format_save", id, path, frames)`. `frames` is an
  array with one pre-rendered frame descriptor per frame (see below), in the
  sprite's pixel format. `path` is the target file name (use it to pick a
  variant, as ppm.js does for `.ppm` vs `.pgm`).

`id` is the request id: pass it back in the reply. Multiple requests can be
in flight at once (a GUI open and a save can run concurrently).

## Replying

- `file.onLoad(id, result)` - success. `result` is `{ frames: [descriptor, ...] }`
  (or a single descriptor for one frame).
- `file.onSave(id, bytes)` - success. `bytes` is a `Uint8Array` with the whole
  file; the app writes it to the target file.
- `file.onLoadError(id, message)` / `file.onSaveError(id, message)` - failure.
  The open/save fails with `message` (no hang).
- `file.resetWatchdog(id)` - re-arms the watchdog window for the in-flight
  request (see below).

Unknown or already-completed ids are ignored (a console warning is printed).
A request is settled at most once: the first reply (or the watchdog) wins.

## Frame descriptor

A frame descriptor is a plain object:

    {
      width: number,        // pixels
      height: number,       // pixels
      format: string,       // see below
      data: Uint8Array,     // pixel data, tight rows (no stride padding)
      palette: number[],    // optional, INDEXED only: 32-bit colors, 1-256 entries
      transparentIndex: number,  // optional, INDEXED only
    }

`data` is row-major with tight rows: its length must be exactly
`width * height * bytesPerPixel` for the frame's format.

### Load (script -> app)

The script's `format_load` reply maps `format` to the document's pixel
format. All conversions are lossless (alpha is never dropped):

| `format`    | document format | data bytes/pixel |
| ----------- | --------------- | ---------------- |
| `"RGBA"`    | RGB (RGBA)      | 4                |
| `"RGB"`     | RGB (RGBA)      | 3 (expanded, alpha = 255) |
| `"GRAYA"`   | RGB (RGBA)      | 2 (gray expanded to RGB, alpha kept) |
| `"INDEXED"` | indexed         | 1 (+ `palette`, optional `transparentIndex`) |

Any other `format` string rejects the load. All frames must have the same
width, height, and pixel format (a document has one of each). The app caps
loads at 10000 frames and rejects dimensions above the file-image ceiling.

### Save (app -> script)

The app pre-renders each frame in the sprite's pixel format and passes the
descriptors; the script returns a single `Uint8Array` for the whole file.

| sprite format | `format`    | data bytes/pixel |
| ------------- | ----------- | ---------------- |
| RGB           | `"RGBA"`    | 4                |
| indexed       | `"INDEXED"` | 1 (+ `palette`, `transparentIndex` when set) |
| grayscale     | `"GRAY"`    | 1                |
| black & white | `"RGBA"`    | 4 (1-bit expanded to 0/255, alpha = 255) |

The reply is written once to the single target file. Script formats are
single-file: a multi-frame document saves all frames into that one file (or,
as in ppm.js, the codec may keep only what the format can express). Layers
are flattened per frame (like PNG/GIF), and frame durations are not part of
the descriptor (loaded documents get the default frame duration).

## The watchdog and long codecs

A codec that does not reply within a 2 second window fails the open/save
with a "codec did not reply (timeout)" error. The window is checked only
between JavaScript calls (while the main loop pumps), so it bounds the
*loop-visible* time - the time between two yields - not the total CPU time:
a monolithic codec that never yields cannot trip the watchdog (it does
freeze the UI for the full decode, though).

A codec that does long work should split it into chunks and re-arm the
watchdog between them:

    app.critical(async () => {
      try {
        for (const chunk of chunks) {
          process(chunk);
          file.resetWatchdog(id);
          await app.yield();
        }
        file.onLoad(id, { frames });
      } catch (err) {
        file.onLoadError(id, String(err));
      }
    });

`app.critical` keeps the script's environment alive across the async window
(and refuses a re-run of the script in the meantime), so an async
continuation cannot outlive its environment. In the GUI the progress window
stays responsive between chunks; the progress bar itself does not advance
during the encode (the codec does not report sub-frame progress).

## Error handling

Wrap the codec body in `try/catch` and call `file.onLoadError` /
`file.onSaveError` with a user-facing message - that is the only way the
message reaches the user. A bare throw is printed to the console and the
request fails via the watchdog with a generic message: diagnosable, but the
user does not see the real error.

## Notes for batch scripts

A script that uses `fs.*` (for example to verify its own output in a batch
run) must declare the permission in its header; headless auto-grants it
(with a console warning):

    ///permission fs.read
    ///permission fs.write

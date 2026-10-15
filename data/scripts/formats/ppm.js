///listen app_boot

// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// PPM/PGM file format implemented in script
//
// Loaded at startup by the app_boot event (///listen app_boot) and
// registers the "PPM/PGM" format for the ppm/pgm extensions. When a file
// with one of those extensions is opened or saved, the app raises a
// format_load / format_save event to this script; the codec below
// decodes/encodes and replies through file.onLoad / file.onSave (or the
// *Error variants on failure).
//
// PPM (P6) and PGM (P5) are single-frame formats: a multi-frame document
// saves only its first frame.
//
// Load: P6/P5, maxval 255 (# comments in the header are skipped).
// Save: maxval 255; PGM stores the luminance of each pixel.

file.registerFormat({
  name: "PPM/PGM",
  extensions: "ppm,pgm",
  load: true,
  save: true,
});

// --- header parsing ---

// ASCII string to bytes (headers only - the codec output is binary).
function ascii(s) {
  const out = new Uint8Array(s.length);
  for (let i = 0; i < s.length; i++)
    out[i] = s.charCodeAt(i);
  return out;
}

const WS = [0x20, 0x09, 0x0a, 0x0d]; // space, tab, LF, CR

// Parses a P6/P5 header (magic, width, height, maxval). # comments and
// whitespace between tokens are skipped. Returns the raster offset or
// throws an Error with a user-facing message.
function parseHeader(bytes) {
  if (bytes.length < 2 || bytes[0] !== 0x50 /* 'P' */)
    throw new Error("not a PPM/PGM file (bad magic)");
  const magic = "P" + String.fromCharCode(bytes[1]);
  if (magic !== "P6" && magic !== "P5")
    throw new Error("not a PPM/PGM file (bad magic)");

  let i = 2;
  const readNumber = (what) => {
    for (;;) {
      while (i < bytes.length && WS.indexOf(bytes[i]) >= 0)
        i++;
      if (i < bytes.length && bytes[i] === 0x23 /* '#' */) {
        while (i < bytes.length && bytes[i] !== 0x0a)
          i++;
        continue;
      }
      break;
    }
    let v = 0, digits = 0;
    while (i < bytes.length && bytes[i] >= 0x30 && bytes[i] <= 0x39) {
      v = v * 10 + (bytes[i] - 0x30);
      i++;
      digits++;
    }
    if (!digits)
      throw new Error("bad " + what + " in PPM/PGM header");
    return v;
  };

  const width = readNumber("width");
  const height = readNumber("height");
  const maxval = readNumber("maxval");
  if (width < 1 || height < 1)
    throw new Error("bad dimensions in PPM/PGM header");
  if (maxval !== 255)
    throw new Error("unsupported PPM/PGM maxval " + maxval + " (only 255)");

  // Skip the whitespace separating the header from the raster.
  while (i < bytes.length && WS.indexOf(bytes[i]) >= 0)
    i++;
  return { magic: magic, width: width, height: height, dataStart: i };
}

// --- codec ---

// format_load: parse the file bytes and reply with the frames.
function formatLoad(id, data) {
  let header;
  try {
    header = parseHeader(data);
  } catch (err) {
    file.onLoadError(id, String(err));
    return;
  }

  const { magic, width, height, dataStart } = header;
  const n = width * height;
  const expected = magic === "P6" ? n * 3 : n;
  if (data.length < dataStart + expected) {
    file.onLoadError(id, "truncated PPM/PGM data");
    return;
  }

  let frame;
  if (magic === "P6") {
    // PPM is RGB: pass the pixels through as the "RGB" descriptor
    // (loaded with alpha = 255).
    frame = {
      width: width,
      height: height,
      format: "RGB",
      data: data.subarray(dataStart, dataStart + expected),
    };
  } else {
    // PGM is gray: "GRAYA" (gray + alpha 255), expanded to RGB on load.
    const src = data.subarray(dataStart, dataStart + expected);
    const grayA = new Uint8Array(n * 2);
    for (let p = 0; p < n; p++) {
      grayA[p * 2] = src[p];
      grayA[p * 2 + 1] = 255;
    }
    frame = { width: width, height: height, format: "GRAYA", data: grayA };
  }
  file.onLoad(id, { frames: [frame] });
}

// The luminance of a pixel (Rec. 601 weights).
function luminance(r, g, b) {
  return Math.round(0.299 * r + 0.587 * g + 0.114 * b);
}

// Encodes one frame as PPM (RGB) bytes.
function encodePpm(f, w, h) {
  const n = w * h;
  const out = new Uint8Array(n * 3);
  if (f.format === "RGBA") {
    const d = f.data;
    for (let p = 0; p < n; p++) {
      out[p * 3] = d[p * 4];
      out[p * 3 + 1] = d[p * 4 + 1];
      out[p * 3 + 2] = d[p * 4 + 2];
    }
  } else if (f.format === "GRAY") {
    const d = f.data;
    for (let p = 0; p < n; p++) {
      out[p * 3] = d[p];
      out[p * 3 + 1] = d[p];
      out[p * 3 + 2] = d[p];
    }
  } else if (f.format === "INDEXED") {
    const d = f.data, pal = f.palette;
    for (let p = 0; p < n; p++) {
      const c = pal[d[p]];
      out[p * 3] = c & 0xff;
      out[p * 3 + 1] = (c >> 8) & 0xff;
      out[p * 3 + 2] = (c >> 16) & 0xff;
    }
  } else {
    throw new Error("unsupported frame format for PPM: " + f.format);
  }
  return out;
}

// Encodes one frame as PGM (gray) bytes: the luminance of each pixel.
function encodePgm(f, w, h) {
  const n = w * h;
  const out = new Uint8Array(n);
  if (f.format === "GRAY") {
    out.set(f.data);
  } else if (f.format === "RGBA") {
    const d = f.data;
    for (let p = 0; p < n; p++)
      out[p] = luminance(d[p * 4], d[p * 4 + 1], d[p * 4 + 2]);
  } else if (f.format === "INDEXED") {
    const d = f.data, pal = f.palette;
    for (let p = 0; p < n; p++) {
      const c = pal[d[p]];
      out[p] = luminance(c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
    }
  } else {
    throw new Error("unsupported frame format for PGM: " + f.format);
  }
  return out;
}

// format_save: encode the frames (the first one - single-frame format)
// and reply with the file bytes.
function formatSave(id, path, frames) {
  const f = frames[0];
  const w = f.width, h = f.height;
  const ppm = path.toLowerCase().endsWith(".ppm");
  try {
    const body = ppm ? encodePpm(f, w, h) : encodePgm(f, w, h);
    const header = ascii((ppm ? "P6" : "P5") + "\n" + w + " " + h + "\n255\n");
    const bytes = new Uint8Array(header.length + body.length);
    bytes.set(header, 0);
    bytes.set(body, header.length);
    file.onSave(id, bytes);
  } catch (err) {
    file.onSaveError(id, String(err));
  }
}

// The codec runs in this script's event handler. The app.critical wrap
// keeps the script's environment alive across the codec's work (and
// refuses a re-run in the meantime) if the codec is or becomes async.
function onEvent(name, id, path, payload) {
  if (name === "format_load")
    app.critical(() => formatLoad(id, payload));
  else if (name === "format_save")
    app.critical(() => formatSave(id, path, payload));
}
setEventHandler(onEvent);

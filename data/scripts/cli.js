// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// Command line interface to edit sprites without the UI.
// tools/lsprite runs it with the arguments in LSPRITE_ARGV.

const col = app.pixelColor;
const OUT = "@@lsprite ";

function emit(obj) {
  console.log(OUT + JSON.stringify(obj));
}

function fail(message) {
  emit({ ok: false, error: message });
}

const BOOL_FLAGS = ["ascii", "no-save", "vertical", "fill"];

function parseArgv() {
  if (typeof LSPRITE_ARGV === "undefined")
    throw new Error("run this script with tools/lsprite");
  const list = LSPRITE_ARGV;
  const positional = [];
  const flags = {};
  for (let i = 0; i < list.length; ++i) {
    const a = list[i];
    if (a.startsWith("--")) {
      const eq = a.indexOf("=");
      if (eq > 0)
        flags[a.slice(2, eq)] = a.slice(eq + 1);
      else if (!BOOL_FLAGS.includes(a.slice(2)) && i + 1 < list.length && !list[i + 1].startsWith("--"))
        flags[a.slice(2)] = list[++i];
      else
        flags[a.slice(2)] = "true";
    } else {
      positional.push(a);
    }
  }
  return { positional, flags };
}

function parseColor(value) {
  if (typeof value === "number")
    return value >>> 0;
  if (typeof value === "string") {
    const s = value.trim();
    if (s === "transparent" || s === "none")
      return col.rgba(0, 0, 0, 0);
    const m = /^#?([0-9a-f]{3}|[0-9a-f]{6})([0-9a-f]{2})?$/i.exec(s);
    if (m) {
      let hex = m[1];
      if (hex.length === 3)
        hex = hex[0] + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
      return col.rgba(parseInt(hex.slice(0, 2), 16), parseInt(hex.slice(2, 4), 16),
                      parseInt(hex.slice(4, 6), 16), m[2] ? parseInt(m[2], 16) : 255);
    }
  }
  if (Array.isArray(value))
    return col.rgba(value[0] | 0, value[1] | 0, value[2] | 0, value.length > 3 ? value[3] | 0 : 255);
  if (value && typeof value === "object")
    return col.rgba(value.r | 0, value.g | 0, value.b | 0, value.a !== undefined ? value.a | 0 : 255);
  throw new Error("invalid color " + JSON.stringify(value) + " (use #RGB, #RRGGBB, #RRGGBBAA, [r,g,b,a] or transparent)");
}

function hex2(n) {
  return (n & 255).toString(16).padStart(2, "0");
}

function colorToHex(c) {
  return "#" + hex2(col.rgbaR(c)) + hex2(col.rgbaG(c)) + hex2(col.rgbaB(c)) + hex2(col.rgbaA(c));
}

function int(value, name, def) {
  if (value === undefined)
    return def;
  const n = Number(value);
  if (!Number.isInteger(n))
    throw new Error(name + " must be an integer, got " + value);
  return n;
}

function bool(value) {
  return value === true || value === "true" || value === "1" || value === "yes";
}

function jsonArg(value, name) {
  if (value === undefined)
    throw new Error("missing --" + name);
  if (typeof value !== "string")
    return value;
  return JSON.parse(value);
}

class Canvas {
  constructor(sprite, layerIndex, frameIndex) {
    if (layerIndex < 0 || layerIndex >= sprite.layerCount)
      throw new Error("layer " + layerIndex + " out of range (0.." + (sprite.layerCount - 1) + ")");
    const layer = sprite.layer(layerIndex);
    if (!layer.isImage)
      throw new Error("layer " + layerIndex + " is a folder");
    if (frameIndex < 0 || frameIndex >= sprite.frameCount)
      throw new Error("frame " + frameIndex + " out of range (0.." + (sprite.frameCount - 1) + ")");
    const cel = layer.cel(frameIndex) || layer.createCel(frameIndex);
    if (!cel || !cel.image)
      throw new Error("layer " + layerIndex + " has no image at frame " + frameIndex);
    this.cel = cel;
    this.image = cel.image;
    this.ox = cel.x;
    this.oy = cel.y;
    this.indexed = sprite.colorMode === ColorMode.INDEXED || sprite.colorMode === ColorMode.BITMAP;
    this.gray = sprite.colorMode === ColorMode.GRAYSCALE;
    this.palette = sprite.palette;
    this.count = 0;
  }

  toNative(color) {
    if (this.gray) {
      const v = Math.round(0.299 * col.rgbaR(color) + 0.587 * col.rgbaG(color) + 0.114 * col.rgbaB(color));
      return col.graya(v, col.rgbaA(color));
    }
    if (!this.indexed)
      return color;
    for (let i = 0; i < this.palette.length; ++i)
      if ((this.palette.get(i) >>> 0) === (color >>> 0))
        return i;
    throw new Error("color " + colorToHex(color) + " is not in the palette of this indexed sprite");
  }

  inside(x, y) {
    const lx = x - this.ox, ly = y - this.oy;
    return lx >= 0 && ly >= 0 && lx < this.image.width && ly < this.image.height;
  }

  get(x, y) {
    if (!this.inside(x, y))
      return col.rgba(0, 0, 0, 0);
    const v = this.image.getPixel(x - this.ox, y - this.oy);
    if (this.gray) {
      const g = col.grayaV(v);
      return col.rgba(g, g, g, col.grayaA(v));
    }
    return this.indexed ? this.palette.get(v) : v;
  }

  set(x, y, native) {
    if (!this.inside(x, y))
      return;
    this.image.putPixel(x - this.ox, y - this.oy, native);
    this.count++;
  }

  line(x0, y0, x1, y1, c) {
    const dx = Math.abs(x1 - x0), dy = -Math.abs(y1 - y0);
    const sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    let err = dx + dy;
    for (;;) {
      this.set(x0, y0, c);
      if (x0 === x1 && y0 === y1)
        break;
      const e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }

  rect(x, y, w, h, c, filled) {
    for (let j = y; j < y + h; ++j)
      for (let i = x; i < x + w; ++i)
        if (filled || j === y || j === y + h - 1 || i === x || i === x + w - 1)
          this.set(i, j, c);
  }

  ellipse(cx, cy, rx, ry, c, filled) {
    for (let j = -ry; j <= ry; ++j) {
      for (let i = -rx; i <= rx; ++i) {
        const d = (i * i) / ((rx + 0.5) * (rx + 0.5)) + (j * j) / ((ry + 0.5) * (ry + 0.5));
        if (d > 1)
          continue;
        if (!filled) {
          const di = Math.abs(i) + 1, dj = Math.abs(j) + 1;
          const out1 = (di * di) / ((rx + 0.5) * (rx + 0.5)) + (j * j) / ((ry + 0.5) * (ry + 0.5));
          const out2 = (i * i) / ((rx + 0.5) * (rx + 0.5)) + (dj * dj) / ((ry + 0.5) * (ry + 0.5));
          if (out1 <= 1 && out2 <= 1)
            continue;
        }
        this.set(cx + i, cy + j, c);
      }
    }
  }

  flood(x, y, c) {
    if (!this.inside(x, y))
      return;
    const lx0 = x - this.ox, ly0 = y - this.oy;
    const w = this.image.width, h = this.image.height;
    const target = this.image.getPixel(lx0, ly0);
    if (target === c)
      return;
    const stack = [[lx0, ly0]];
    while (stack.length) {
      const [px, py] = stack.pop();
      if (px < 0 || py < 0 || px >= w || py >= h)
        continue;
      if (this.image.getPixel(px, py) !== target)
        continue;
      this.image.putPixel(px, py, c);
      this.count++;
      stack.push([px + 1, py], [px - 1, py], [px, py + 1], [px, py - 1]);
    }
  }
}

function openSprite(path) {
  if (!path)
    throw new Error("missing file argument");
  const doc = app.open(path);
  if (!doc)
    throw new Error("cannot open " + path);
  return doc.sprite;
}

function layerList(sprite) {
  const layers = [];
  for (let i = 0; i < sprite.layerCount; ++i) {
    const l = sprite.layer(i);
    layers.push({
      index: i,
      name: l.name,
      visible: l.isVisible,
      editable: l.isEditable,
      kind: l.isImage ? (l.isBackground ? "background" : "image") : "folder",
      cels: l.celCount,
    });
  }
  return layers;
}

function modeName(mode) {
  for (const k of ["RGB", "GRAYSCALE", "INDEXED", "BITMAP"])
    if (ColorMode[k] === mode)
      return k.toLowerCase();
  return String(mode);
}

function paletteList(sprite) {
  const pal = sprite.palette;
  const colors = [];
  for (let i = 0; i < pal.length; ++i)
    colors.push(colorToHex(pal.get(i)));
  return colors;
}

function commandIds() {
  const proto = Object.getPrototypeOf(app.command);
  return Object.getOwnPropertyNames(proto)
    .filter((k) => k !== "constructor" && k !== "setParameter" && k !== "clearParameters" && k !== "info")
    .sort();
}

function runCommand(name, params) {
  const fn = app.command[name];
  if (typeof fn !== "function")
    throw new Error("unknown command " + name + " (see: lsprite commands)");
  const ok = params && Object.keys(params).length ? fn.call(app.command, params) : fn.call(app.command);
  if (!ok)
    throw new Error("command " + name + " is not available in the current state");
}

function save(sprite, flags) {
  sprite.commit();
  if (flags.out)
    sprite.saveAs(flags.out, true);
  else
    sprite.save();
  return flags.out || sprite.filename;
}

function canvasFor(sprite, flags) {
  return new Canvas(sprite, int(flags.layer, "layer", 0), int(flags.frame, "frame", 0));
}

function applyOp(cv, op) {
  const c = op.c !== undefined ? cv.toNative(parseColor(op.c)) : undefined;
  const need = () => {
    if (c === undefined)
      throw new Error("op " + op.op + " needs c (color)");
    return c;
  };
  switch (op.op) {
    case "pixel":
      cv.set(op.x, op.y, need());
      break;
    case "pixels":
      for (const p of op.points) {
        const pc = p.length > 2 ? cv.toNative(parseColor(p[2])) : need();
        cv.set(p[0], p[1], pc);
      }
      break;
    case "line":
      cv.line(op.x0, op.y0, op.x1, op.y1, need());
      break;
    case "polyline": {
      const pts = op.points;
      for (let i = 1; i < pts.length; ++i)
        cv.line(pts[i - 1][0], pts[i - 1][1], pts[i][0], pts[i][1], need());
      if (op.closed && pts.length > 2)
        cv.line(pts[pts.length - 1][0], pts[pts.length - 1][1], pts[0][0], pts[0][1], need());
      break;
    }
    case "rect":
      cv.rect(op.x, op.y, op.w, op.h, need(), !!op.fill);
      break;
    case "ellipse":
      cv.ellipse(op.cx, op.cy, op.rx, op.ry !== undefined ? op.ry : op.rx, need(), !!op.fill);
      break;
    case "fill":
      cv.flood(op.x, op.y, need());
      break;
    case "clear":
      cv.rect(cv.ox, cv.oy, cv.image.width, cv.image.height,
              op.c !== undefined ? c : cv.toNative(col.rgba(0, 0, 0, 0)), true);
      break;
    case "ascii": {
      const map = op.map || {};
      const natives = {};
      for (const k in map)
        natives[k] = cv.toNative(parseColor(map[k]));
      const x0 = op.x || 0, y0 = op.y || 0;
      op.rows.forEach((row, j) => {
        for (let i = 0; i < row.length; ++i) {
          const ch = row[i];
          if (natives[ch] !== undefined)
            cv.set(x0 + i, y0 + j, natives[ch]);
        }
      });
      break;
    }
    case "replace": {
      const from = cv.toNative(parseColor(op.from));
      const to = cv.toNative(parseColor(op.to));
      for (let y = 0; y < cv.image.height; ++y)
        for (let x = 0; x < cv.image.width; ++x)
          if (cv.image.getPixel(x, y) === from) {
            cv.image.putPixel(x, y, to);
            cv.count++;
          }
      break;
    }
    default:
      throw new Error("unknown op " + op.op + " (pixel, pixels, line, polyline, rect, ellipse, fill, clear, ascii, replace)");
  }
}

const COMMANDS = {
  info(pos, flags) {
    const sprite = openSprite(pos[0]);
    return {
      file: sprite.filename,
      width: sprite.width,
      height: sprite.height,
      colorMode: modeName(sprite.colorMode),
      frames: sprite.frameCount,
      layers: layerList(sprite),
      paletteSize: sprite.palette.length,
    };
  },

  pixels(pos, flags) {
    const sprite = openSprite(pos[0]);
    const cv = canvasFor(sprite, flags);
    const x0 = int(flags.x, "x", 0), y0 = int(flags.y, "y", 0);
    const w = int(flags.w, "w", sprite.width - x0), h = int(flags.h, "h", sprite.height - y0);
    if (bool(flags.ascii)) {
      const legend = {};
      const symbols = ".#@%&*+=-:ox0123456789abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
      let next = 1;
      const rows = [];
      for (let y = y0; y < y0 + h; ++y) {
        let row = "";
        for (let x = x0; x < x0 + w; ++x) {
          const c = colorToHex(cv.get(x, y));
          if (c.endsWith("00")) { row += "."; continue; }
          if (legend[c] === undefined) {
            if (next >= symbols.length)
              throw new Error("too many colors for --ascii, read a smaller area or drop --ascii");
            legend[c] = symbols[next++];
          }
          row += legend[c];
        }
        rows.push(row);
      }
      const map = {};
      for (const k in legend)
        map[legend[k]] = k;
      return { x: x0, y: y0, rows, map };
    }
    const rows = [];
    for (let y = y0; y < y0 + h; ++y) {
      const row = [];
      for (let x = x0; x < x0 + w; ++x)
        row.push(colorToHex(cv.get(x, y)));
      rows.push(row);
    }
    return { x: x0, y: y0, rows };
  },

  draw(pos, flags) {
    const sprite = openSprite(pos[0]);
    const cv = canvasFor(sprite, flags);
    let ops = jsonArg(flags.ops, "ops");
    if (!Array.isArray(ops))
      ops = [ops];
    ops.forEach(op => applyOp(cv, op));
    return { pixels: cv.count, ops: ops.length, saved: save(sprite, flags) };
  },

  palette(pos, flags) {
    const sprite = openSprite(pos[0]);
    const pal = sprite.palette;
    let changed = false;
    if (flags.set !== undefined) {
      const entries = jsonArg(flags.set, "set");
      for (const k in entries) {
        const i = int(k, "index");
        if (i < 0 || i >= pal.length)
          throw new Error("palette index " + i + " out of range (0.." + (pal.length - 1) + ")");
        const c = parseColor(entries[k]);
        pal.set(i, col.rgbaR(c), col.rgbaG(c), col.rgbaB(c), col.rgbaA(c));
      }
      changed = true;
    }
    if (flags.load !== undefined) {
      sprite.loadPalette(flags.load);
      changed = true;
    }
    const result = { palette: paletteList(sprite) };
    if (changed)
      result.saved = save(sprite, flags);
    return result;
  },

  layer(pos, flags) {
    const action = pos[0];
    const sprite = openSprite(pos[1]);
    const idx = int(flags.index, "index", sprite.layerCount - 1);
    const pick = () => {
      if (idx < 0 || idx >= sprite.layerCount)
        throw new Error("layer " + idx + " out of range (0.." + (sprite.layerCount - 1) + ")");
      return sprite.layer(idx);
    };
    switch (action) {
      case "list":
        return { layers: layerList(sprite) };
      case "add":
        runCommand("NewLayer", { name: flags.name || "", top: "true" });
        break;
      case "rename":
        pick().name = flags.name;
        break;
      case "show":
        pick().isVisible = true;
        break;
      case "hide":
        pick().isVisible = false;
        break;
      case "lock":
        pick().isEditable = false;
        break;
      case "unlock":
        pick().isEditable = true;
        break;
      default:
        throw new Error("layer action must be list, add, rename, show, hide, lock or unlock");
    }
    return { layers: layerList(sprite), saved: save(sprite, flags) };
  },

  frame(pos, flags) {
    const action = pos[0];
    const sprite = openSprite(pos[1]);
    const last = sprite.frameCount - 1;
    switch (action) {
      case "list":
        return { frames: sprite.frameCount };
      case "add":
        if (flags.copy !== undefined)
          sprite.copyFrame(int(flags.copy, "copy"), int(flags.at, "at", last + 1));
        else
          sprite.addEmptyFrame(int(flags.at, "at", last + 1));
        break;
      case "remove":
        sprite.removeFrame(int(flags.index, "index", last));
        break;
      default:
        throw new Error("frame action must be list, add or remove");
    }
    return { frames: sprite.frameCount, saved: save(sprite, flags) };
  },

  resize(pos, flags) {
    const sprite = openSprite(pos[0]);
    sprite.resize(int(flags.w, "w", sprite.width), int(flags.h, "h", sprite.height));
    return { width: sprite.width, height: sprite.height, saved: save(sprite, flags) };
  },

  crop(pos, flags) {
    const sprite = openSprite(pos[0]);
    runCommand("CropSprite", {
      x: String(int(flags.x, "x", 0)),
      y: String(int(flags.y, "y", 0)),
      width: String(int(flags.w, "w", sprite.width)),
      height: String(int(flags.h, "h", sprite.height)),
    });
    return { width: sprite.width, height: sprite.height, saved: save(sprite, flags) };
  },

  flip(pos, flags) {
    const sprite = openSprite(pos[0]);
    runCommand("Flip", { orientation: bool(flags.vertical) ? "vertical" : "horizontal" });
    return { saved: save(sprite, flags) };
  },

  mode(pos, flags) {
    const sprite = openSprite(pos[0]);
    const format = pos[1];
    if (!["rgb", "grayscale", "indexed"].includes(format))
      throw new Error("mode must be rgb, grayscale or indexed");
    runCommand("ChangePixelFormat", { format, dithering: flags.dithering || "none" });
    return { colorMode: modeName(sprite.colorMode), saved: save(sprite, flags) };
  },

  commands() {
    return { commands: commandIds() };
  },

  "command-info"(pos) {
    const info = app.command.info(pos[0]);
    if (!info)
      throw new Error("unknown command " + pos[0]);
    return info;
  },

  cmd(pos, flags) {
    const sprite = openSprite(pos[0]);
    const name = pos[1];
    const params = {};
    for (const kv of pos.slice(2)) {
      const eq = kv.indexOf("=");
      if (eq < 0)
        throw new Error("command params must be name=value, got " + kv);
      params[kv.slice(0, eq)] = kv.slice(eq + 1);
    }
    runCommand(name, params);
    return { ran: name, saved: save(sprite, flags) };
  },

  eval(pos, flags) {
    const sprite = pos[0] ? openSprite(pos[0]) : null;
    const code = flags.code;
    if (!code)
      throw new Error("missing --code");
    const helpers = { parseColor, colorToHex, Canvas, applyOp, runCommand, col };
    const fn = new Function("sprite", "app", "lsprite", code);
    const value = fn(sprite, app, helpers);
    const result = { result: value === undefined ? null : value };
    if (sprite && !bool(flags["no-save"]))
      result.saved = save(sprite, flags);
    return result;
  },
};

function main() {
  const { positional, flags } = parseArgv();
  const name = positional.shift();
  const command = COMMANDS[name];
  if (!command)
    throw new Error("unknown command " + name + ". Available: " + Object.keys(COMMANDS).join(", "));
  emit(Object.assign({ ok: true }, command(positional, flags)));
}

try {
  main();
} catch (e) {
  fail(e && e.message ? e.message : String(e));
}

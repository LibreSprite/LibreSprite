/**
 * Scripts handed to `libresprite --batch --script`.
 *
 * Hard constraints, all verified against the source:
 *
 *  - ES5.1 only. The image builds Duktape (src/script/CMakeLists.txt:21); V8 is
 *    only compiled when libnode-dev is present (CMakeLists.txt:204-207), which
 *    it deliberately is not. No let/const, no arrow functions, no template
 *    literals, no Array.prototype.includes, no Object.assign.
 *
 *  - Straight-line and synchronous. app.yield() and event listeners are pumped
 *    by a ui::Timer (src/app/task_manager.h:108) that never ticks without a UI.
 *
 *  - These APIs dereference ui::Manager::getDefault() and segfault in batch:
 *      image.putImageData()   src/app/script/api/image_script.cpp:74
 *      palette.set() / .length =   src/app/script/api/palette_script.cpp:59
 *      app.redraw()           src/app/script/api/app_script.cpp:116
 *    Pixel writes therefore go through putPixel/clear only, and palettes are
 *    changed with sprite.loadPalette(), which uses DocumentApi instead.
 *
 *  - sprite.crop() is a no-op: its body is commented out
 *    (src/app/script/api/sprite_script.cpp:135-150). Use the --crop CLI flag.
 *
 *  - There is no --script-param in this codebase, so parameters are injected as
 *    a PARAMS literal at the top of the generated file.
 *
 * Empirically verified against the built container (see README "Headless
 * capability matrix"). From a --script run:
 *
 *    works    Layer.name / .isVisible / .isEditable writes
 *             Image.putPixel / .clear / .getPixel / .getPNGData
 *             sprite.commit() + sprite.save()
 *    HANGS    sprite.resize(), sprite.loadPalette()  -- the Transaction path
 *    returns 0 (isEnabled false, blocked by command_script.cpp:31)
 *             NewLayer, FlattenLayers, and the other document-modifying commands
 *    SEGV     ChangePixelFormat
 *
 * So: no template may call a document-modifying app.command, sprite.resize() or
 * sprite.loadPalette(). Sprite geometry and colour-mode work belongs on the
 * native CLI route, where the same commands do work (app.cpp drives them
 * without the isEnabled gate).
 */

const PRELUDE = `
function lsPad2(n) {
  var s = n.toString(16);
  return s.length < 2 ? '0' + s : s;
}

// Canonical colour key: 8 lowercase hex digits, rgba.
function lsKey(color) {
  return lsPad2(app.pixelColor.rgbaR(color))
       + lsPad2(app.pixelColor.rgbaG(color))
       + lsPad2(app.pixelColor.rgbaB(color))
       + lsPad2(app.pixelColor.rgbaA(color));
}

function lsHex(color) {
  return '#' + lsKey(color);
}

function lsColorModeName(v) {
  var names = ['RGB', 'GRAYSCALE', 'INDEXED', 'BITMAP'];
  return names[v] !== undefined ? names[v] : ('UNKNOWN(' + v + ')');
}

// app.activeSprite / activeDocument / activeImage all read through
// app::current_editor (src/app/script/api/app_script.cpp:60-71), which is null
// without a UI -- they are always null in batch. app.open() returns the
// Document script object directly, so that is the only usable entry point.
function lsOpen(file) {
  var doc = app.open(file);
  if (!doc) throw new Error('could not open sprite: ' + file);
  var s = doc.sprite;
  if (!s) throw new Error('opened document has no sprite: ' + file);
  return s;
}

function lsFindLayer(s, ref) {
  if (typeof ref === 'number') {
    if (ref < 0 || ref >= s.layerCount) {
      throw new Error('layer index ' + ref + ' out of range (sprite has ' + s.layerCount + ')');
    }
    return s.layer(ref);
  }
  var names = [];
  for (var i = 0; i < s.layerCount; i++) {
    var l = s.layer(i);
    names.push(l.name);
    if (l.name === ref) return l;
  }
  throw new Error('no layer named "' + ref + '". Available: ' + names.join(', '));
}

function lsLayerIndex(s, ref) {
  if (typeof ref === 'number') return ref;
  for (var i = 0; i < s.layerCount; i++) {
    if (s.layer(i).name === ref) return i;
  }
  throw new Error('no layer named "' + ref + '"');
}

function lsFirstImageLayer(s) {
  for (var i = 0; i < s.layerCount; i++) {
    var l = s.layer(i);
    if (l.isImage && l.isVisible) return l;
  }
  for (var j = 0; j < s.layerCount; j++) {
    if (s.layer(j).isImage) return s.layer(j);
  }
  throw new Error('sprite has no image layer');
}

// Sprite exposes no frameCount (src/app/script/api/sprite_script.cpp:30-90),
// so derive it from the highest cel frame across layers.
function lsFrameCount(s) {
  var max = 0;
  for (var i = 0; i < s.layerCount; i++) {
    var l = s.layer(i);
    for (var c = 0; c < l.celCount; c++) {
      var cel = l.cel(c);
      if (cel && cel.frame + 1 > max) max = cel.frame + 1;
    }
  }
  return max;
}

// Layer.cel(i) indexes the layer's cel list, not the timeline.
function lsCelAtFrame(layer, frame) {
  for (var c = 0; c < layer.celCount; c++) {
    var cel = layer.cel(c);
    if (cel && cel.frame === frame) return cel;
  }
  return null;
}

// Duktape marshals booleans back as 1/0; normalise so JSON.stringify emits
// real booleans and the host does not have to guess.
function lsBool(v) {
  return v ? true : false;
}

function lsStripDataUri(s) {
  var comma = s.indexOf(',');
  return comma === -1 ? s : s.substring(comma + 1);
}

// Colours are resolved in-engine rather than packed on the host: doc::rgba
// packs alpha into bit 31 (src/doc/color.h:48-51), which JavaScript bitwise
// ops would turn negative. Going through app.pixelColor guarantees the value
// matches whatever getPixel() returns.
function lsColorValue(c) {
  if (c === null || c === undefined) throw new Error('missing colour value');
  if (typeof c === 'number') return c;
  if (c.index !== undefined) return c.index;
  return app.pixelColor.rgba(c.r, c.g, c.b, c.a === undefined ? 255 : c.a);
}

// Indexed images store palette indices, not packed colours.
function lsResolver(sprite) {
  if (sprite.colorMode !== ColorMode.INDEXED) {
    return function (v) { return v; };
  }
  var pal = sprite.palette;
  var cache = [];
  for (var i = 0; i < pal.length; i++) cache.push(pal.get(i));
  return function (v) { return cache[v] !== undefined ? cache[v] : 0; };
}
`;

const EPILOGUE = `
(function () {
  var payload;
  try {
    payload = { ok: true, data: main() };
  } catch (e) {
    payload = { ok: false, error: (e && e.message) ? e.message : ('' + e) };
  }
  console.log('<<<LSMCP>>>' + JSON.stringify(payload) + '<<<END>>>');
})();
`;

export const TEMPLATES: Record<string, string> = {
  // -------------------------------------------------------------------------
  sprite_info: `
function main() {
  var s = lsOpen(PARAMS.file);
  var layers = [];
  for (var i = 0; i < s.layerCount; i++) {
    var l = s.layer(i);
    layers.push({
      index: i,
      name: l.name,
      visible: lsBool(l.isVisible),
      editable: lsBool(l.isEditable),
      isImage: lsBool(l.isImage),
      isBackground: lsBool(l.isBackground),
      celCount: l.celCount
    });
  }
  return {
    file: s.filename,
    width: s.width,
    height: s.height,
    colorMode: lsColorModeName(s.colorMode),
    layerCount: s.layerCount,
    frameCount: lsFrameCount(s),
    paletteSize: s.palette.length,
    layers: layers
  };
}
`,

  // -------------------------------------------------------------------------
  render_preview: `
function main() {
  var s = lsOpen(PARAMS.file);
  var frame = PARAMS.frame || 0;

  // There is no composite renderer for scripts, and FlattenLayers is refused
  // headless (isEnabled is false), so a single layer's cel is the most that can
  // be rendered. The host reports which layer was picked.
  var layer = (PARAMS.layer === null || PARAMS.layer === undefined)
    ? lsFirstImageLayer(s)
    : lsFindLayer(s, PARAMS.layer);

  var cel = lsCelAtFrame(layer, frame);
  if (!cel) {
    throw new Error('layer "' + layer.name + '" has no cel at frame ' + frame
      + ' (sprite has ' + lsFrameCount(s) + ' frames)');
  }

  var img = cel.image;
  return {
    png: lsStripDataUri(img.getPNGData()),
    width: img.width,
    height: img.height,
    x: cel.x,
    y: cel.y,
    layer: layer.name,
    layerCount: s.layerCount,
    frame: frame
  };
}
`,

  // -------------------------------------------------------------------------
  get_palette: `
function main() {
  var s = lsOpen(PARAMS.file);
  var pal = s.palette;
  var colors = [];
  for (var i = 0; i < pal.length; i++) {
    colors.push(lsHex(pal.get(i)));
  }
  return {
    size: pal.length,
    colorMode: lsColorModeName(s.colorMode),
    colors: colors
  };
}
`,

  // -------------------------------------------------------------------------
  validate_palette: `
function main() {
  var s = lsOpen(PARAMS.file);
  var resolve = lsResolver(s);
  var allowed = PARAMS.allowed || null;
  var allowedSet = {};
  var i;
  if (allowed) {
    for (i = 0; i < allowed.length; i++) allowedSet[allowed[i]] = true;
  }

  var budget = PARAMS.maxPixels || 4000000;
  var truncated = false;
  var histogram = {};
  var uniqueCount = 0;
  var violations = [];
  var scanned = 0;

  for (var li = 0; li < s.layerCount; li++) {
    var layer = s.layer(li);
    if (!layer.isImage) continue;
    if (PARAMS.visibleOnly && !layer.isVisible) continue;

    for (var ci = 0; ci < layer.celCount; ci++) {
      var cel = layer.cel(ci);
      if (!cel) continue;
      var img = cel.image;

      for (var y = 0; y < img.height; y++) {
        for (var x = 0; x < img.width; x++) {
          if (scanned >= budget) { truncated = true; break; }
          scanned++;

          var color = resolve(img.getPixel(x, y));
          if (PARAMS.ignoreTransparent && app.pixelColor.rgbaA(color) === 0) continue;

          var key = lsKey(color);
          if (histogram[key] === undefined) {
            histogram[key] = { count: 0 };
            uniqueCount++;
            if (allowed && !allowedSet[key]) {
              violations.push({
                color: '#' + key,
                layer: layer.name,
                frame: cel.frame,
                x: x + cel.x,
                y: y + cel.y
              });
            }
          }
          histogram[key].count++;
        }
        if (truncated) break;
      }
      if (truncated) break;
    }
    if (truncated) break;
  }

  var used = [];
  for (var k in histogram) {
    if (histogram.hasOwnProperty(k)) used.push({ color: '#' + k, count: histogram[k].count });
  }
  used.sort(function (a, b) { return b.count - a.count; });

  var problems = [];
  if (violations.length > 0) {
    problems.push(violations.length + ' colour(s) outside the master palette');
  }
  if (PARAMS.maxColors && uniqueCount > PARAMS.maxColors) {
    problems.push('uses ' + uniqueCount + ' colours, limit is ' + PARAMS.maxColors);
  }
  if (PARAMS.tileWidth && s.width % PARAMS.tileWidth !== 0) {
    problems.push('width ' + s.width + ' is not a multiple of tile width ' + PARAMS.tileWidth);
  }
  if (PARAMS.tileHeight && s.height % PARAMS.tileHeight !== 0) {
    problems.push('height ' + s.height + ' is not a multiple of tile height ' + PARAMS.tileHeight);
  }

  return {
    file: s.filename,
    colorMode: lsColorModeName(s.colorMode),
    width: s.width,
    height: s.height,
    paletteSize: s.palette.length,
    uniqueColors: uniqueCount,
    pixelsScanned: scanned,
    truncated: truncated,
    valid: problems.length === 0,
    problems: problems,
    offendingColors: violations.slice(0, PARAMS.maxReported || 50),
    offendingColorCount: violations.length,
    usedColors: used.slice(0, PARAMS.maxReported || 50)
  };
}
`,

  // -------------------------------------------------------------------------
  // Snap every pixel to the nearest master-palette colour, in place, one cel at
  // a time so layer structure survives. Runs in-script rather than shipping
  // pixels to the host: putPixel/getPixel are among the few APIs that work
  // headless, and this avoids transferring megabytes through a JSON payload.
  //
  // Note this does NOT change the sprite's colour mode -- ChangePixelFormat
  // segfaults headless. The result is an RGB sprite that only uses palette
  // colours, which is what validate_palette checks for anyway.
  quantize: `
function main() {
  var s = lsOpen(PARAMS.file);
  if (s.colorMode === ColorMode.INDEXED) {
    throw new Error('sprite is already INDEXED; quantising it again would corrupt the indices');
  }

  var pal = PARAMS.palette;
  if (!pal || !pal.length) throw new Error('empty palette');
  var thr = PARAMS.alphaThreshold;
  var budget = PARAMS.maxPixels;
  var transparent = app.pixelColor.rgba(0, 0, 0, 0);

  var scanned = 0, changed = 0, truncated = false;
  var cache = {};

  for (var li = 0; li < s.layerCount; li++) {
    var layer = s.layer(li);
    if (!layer.isImage) continue;

    for (var ci = 0; ci < layer.celCount; ci++) {
      var cel = layer.cel(ci);
      if (!cel) continue;
      var img = cel.image;

      for (var y = 0; y < img.height; y++) {
        for (var x = 0; x < img.width; x++) {
          if (scanned >= budget) { truncated = true; break; }
          scanned++;

          var c = img.getPixel(x, y);
          var a = app.pixelColor.rgbaA(c);
          if (a < thr) {
            if (a !== 0) { img.putPixel(x, y, transparent); changed++; }
            continue;
          }

          var r = app.pixelColor.rgbaR(c);
          var g = app.pixelColor.rgbaG(c);
          var b = app.pixelColor.rgbaB(c);
          var key = r + ',' + g + ',' + b;

          var bi = cache[key];
          if (bi === undefined) {
            var bd = 1e12;
            bi = 0;
            for (var i = 0; i < pal.length; i++) {
              var dr = r - pal[i].r, dg = g - pal[i].g, db = b - pal[i].b;
              var d = dr * dr * 0.3 + dg * dg * 0.59 + db * db * 0.11;
              if (d < bd) { bd = d; bi = i; }
            }
            cache[key] = bi;
          }

          var p = pal[bi];
          var nv = app.pixelColor.rgba(p.r, p.g, p.b, 255);
          if (nv !== c) { img.putPixel(x, y, nv); changed++; }
        }
        if (truncated) break;
      }
      if (truncated) break;
    }
    if (truncated) break;
  }

  s.commit();
  s.save();
  return {
    file: s.filename,
    paletteSize: pal.length,
    pixelsScanned: scanned,
    pixelsChanged: changed,
    truncated: truncated
  };
}
`,

  // -------------------------------------------------------------------------
  layer_ops: `
function main() {
  var s = lsOpen(PARAMS.file);
  var applied = [];

  for (var i = 0; i < PARAMS.ops.length; i++) {
    var op = PARAMS.ops[i];

    // Only Layer property writes are available headless. Structural edits
    // (add/remove/merge/flatten) go through app::Command, which is refused
    // without an editor -- see the capability matrix at the top of this file.
    if (op.op === 'set_visible') {
      var lv = lsFindLayer(s, op.layer);
      lv.isVisible = !!op.value;
      applied.push('set_visible ' + lv.name + '=' + (!!op.value));

    } else if (op.op === 'set_editable') {
      var le = lsFindLayer(s, op.layer);
      le.isEditable = !!op.value;
      applied.push('set_editable ' + le.name + '=' + (!!op.value));

    } else if (op.op === 'rename') {
      var lr = lsFindLayer(s, op.layer);
      var old = lr.name;
      lr.name = op.name;
      applied.push('rename ' + old + ' -> ' + op.name);

    } else {
      throw new Error('unknown layer op: ' + op.op);
    }
  }

  s.commit();
  s.save();

  var layers = [];
  for (var j = 0; j < s.layerCount; j++) {
    var l = s.layer(j);
    layers.push({ index: j, name: l.name, visible: lsBool(l.isVisible) });
  }
  return { file: s.filename, applied: applied, layers: layers };
}
`,

  // -------------------------------------------------------------------------
  pixel_ops: `
function main() {
  var s = lsOpen(PARAMS.file);
  var frame = PARAMS.frame || 0;
  var layer = lsFindLayer(s, PARAMS.layer);
  var cel = lsCelAtFrame(layer, frame);
  if (!cel) {
    throw new Error('layer "' + layer.name + '" has no cel at frame ' + frame);
  }
  var img = cel.image;

  if (s.colorMode === ColorMode.INDEXED && !PARAMS.allowIndexed) {
    throw new Error('sprite is INDEXED: putPixel takes palette indices, not colours. '
      + 'Pass allowIndexed=true and give colours as indices, or convert to RGB first.');
  }

  var written = 0;
  var skipped = 0;

  function put(x, y, value) {
    if (x < 0 || y < 0 || x >= img.width || y >= img.height) { skipped++; return; }
    img.putPixel(x, y, value);
    written++;
  }

  for (var i = 0; i < PARAMS.ops.length; i++) {
    var op = PARAMS.ops[i];

    if (op.op === 'put_pixels') {
      for (var p = 0; p < op.pixels.length; p++) {
        put(op.pixels[p].x, op.pixels[p].y, lsColorValue(op.pixels[p].value));
      }

    } else if (op.op === 'fill_rect') {
      var fill = lsColorValue(op.value);
      for (var y = op.y; y < op.y + op.h; y++) {
        for (var x = op.x; x < op.x + op.w; x++) put(x, y, fill);
      }

    } else if (op.op === 'replace_color') {
      var from = lsColorValue(op.from);
      var to = lsColorValue(op.to);
      for (var ry = 0; ry < img.height; ry++) {
        for (var rx = 0; rx < img.width; rx++) {
          if (img.getPixel(rx, ry) === from) put(rx, ry, to);
        }
      }

    } else if (op.op === 'clear') {
      img.clear(lsColorValue(op.value));
      written += img.width * img.height;

    } else {
      throw new Error('unknown pixel op: ' + op.op);
    }
  }

  s.commit();
  s.save();
  return {
    file: s.filename,
    layer: layer.name,
    frame: frame,
    imageWidth: img.width,
    imageHeight: img.height,
    pixelsWritten: written,
    pixelsOutOfBounds: skipped
  };
}
`,
};

/** Render a template into a runnable script with its parameters baked in. */
export function buildScript(name: string, params: Record<string, unknown>): string {
  const body = TEMPLATES[name];
  if (!body) {
    throw new Error(`Unknown script template "${name}". Known: ${Object.keys(TEMPLATES).join(', ')}`);
  }
  return [
    '// generated by libresprite-mcp -- do not edit',
    `var PARAMS = ${JSON.stringify(params)};`,
    PRELUDE,
    body,
    EPILOGUE,
  ].join('\n');
}

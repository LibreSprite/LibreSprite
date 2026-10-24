// outline.js
// Draw a black outline around the current layer onto a new layer.
//
// The outline is a one-pixel halo: every transparent pixel that is
// 8-adjacent to a visible pixel of the current layer. The halo sits just
// outside the layer's silhouette, so it does not cover the layer itself.
// A shape touching the canvas edge has its halo clipped to the canvas.
//
// A new transparent layer is created above the current layer and named
// "<layer> outline". One cel is written per frame that has an outline.

(function main() {
    const sprite = app.activeSprite;
    if (!sprite) {
        console.log("outline: no active sprite");
        return;
    }

    const src = sprite.layer(app.activeLayerNumber);
    if (!src || !src.isImage) {
        console.log("outline: the active layer is not an image layer");
        return;
    }

    const pc = app.pixelColor;
    const W = sprite.width, H = sprite.height;
    const frames = sprite.totalFrames;
    const indexed = sprite.colorMode === ColorMode.INDEXED;

    // How to test a pixel for "visible", and which values are black and
    // transparent.
    let isOn, black, trans;
    if (indexed) {
        // Precompute visibility per palette entry, then pick a transparent
        // index (alpha 0) to fill with and a black index (a pure opaque
        // black if present, else the darkest opaque one).
        const pal = sprite.palette;
        const on = new Uint8Array(pal.length);
        let transIdx = -1, pure = -1, dark = -1, best = 1e9;
        for (let i = 0; i < pal.length; i++) {
            const c = pal.get(i), a = pc.rgbaA(c);
            on[i] = a > 0 ? 1 : 0;
            if (a === 0) {
                if (transIdx < 0)
                    transIdx = i;
                continue;
            }
            const r = pc.rgbaR(c), g = pc.rgbaG(c), b = pc.rgbaB(c);
            if (r === 0 && g === 0 && b === 0 && pure < 0)
                pure = i;
            const lum = r + g + b;
            if (lum < best) {
                best = lum;
                dark = i;
            }
        }
        if (transIdx < 0) {
            console.log("outline: the palette has no transparent entry, so a transparent outline layer cannot be made");
            return;
        }
        black = pure >= 0 ? pure : dark;
        if (black < 0) {
            console.log("outline: no opaque palette entry to draw black");
            return;
        }
        trans = transIdx;
        isOn = (d, i) => on[d[i]] === 1;
    } else {
        // i is the byte offset of the pixel's first channel.
        isOn = (d, i) => d[i + 3] > 0;
    }

    // Map a cel's pixels onto a full-canvas on/off grid. The cel may be
    // trimmed and offset, so map through (cel.x, cel.y).
    function onMap(cel) {
        const m = new Uint8Array(W * H);
        const img = cel.image;
        const cw = img.width, ch = img.height, sc = img.stride;
        const bpp = sc / cw;
        const d = img.getImageData();
        for (let ly = 0; ly < ch; ly++)
            for (let lx = 0; lx < cw; lx++) {
                const x = cel.x + lx, y = cel.y + ly;
                if (x < 0 || x >= W || y < 0 || y >= H)
                    continue;
                if (isOn(d, ly * sc + lx * bpp))
                    m[y * W + x] = 1;
            }
        return m;
    }

    // Halo of an on/off grid: transparent pixels with a visible 8-neighbour.
    // The ring sits just outside the shape's silhouette.
    function halo(m) {
        const o = new Uint8Array(W * H);
        for (let y = 0; y < H; y++)
            for (let x = 0; x < W; x++) {
                if (m[y * W + x])
                    continue;   // the halo is outside the shape
                let ring = false;
                for (let dy = -1; dy <= 1 && !ring; dy++)
                    for (let dx = -1; dx <= 1; dx++) {
                        if ((!dx && !dy) || (dx && dy))
                            continue;
                        const nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < W && ny >= 0 && ny < H && m[ny * W + nx]) {
                            ring = true;
                            break;
                        }
                    }
                if (ring)
                    o[y * W + x] = 1;
            }
        return o;
    }

    // Create the outline layer above the current one (it becomes active).
    command.NewLayer();
    const out = sprite.layer(app.activeLayerNumber);
    out.name = src.name + " outline";

    // For each frame with a halo, create a cel and remember its mask.
    const writes = [];
    for (let f = 0; f < frames; f++) {
        const cel = src.cel(f);
        if (!cel)
            continue;
        const mask = halo(onMap(cel));
        let any = false;
        for (let i = 0; i < mask.length; i++)
            if (mask[i]) {
                any = true;
                break;
            }
        if (!any)
            continue;
        writes.push({ cel: out.createCel(f), mask: mask });
    }
    if (writes.length === 0) {
        console.log("outline: nothing to outline (the layer is empty or fills the canvas)");
        return;
    }

    // Write every frame's outline in a single undo step.
    app.transaction(() => {
        for (const w of writes) {
            const img = w.cel.image;
            const sc = img.stride, bpp = sc / img.width;
            const buf = new Uint8Array(sc * H);   // 0 = transparent (RGB)
            if (indexed)
                buf.fill(trans);                 // the palette's transparent index
            const mask = w.mask;
            for (let y = 0; y < H; y++)
                for (let x = 0; x < W; x++) {
                    if (!mask[y * W + x])
                        continue;
                    const o = y * sc + x * bpp;
                    if (indexed)
                        buf[o] = black;
                    else {
                        buf[o] = 0;
                        buf[o + 1] = 0;
                        buf[o + 2] = 0;
                        buf[o + 3] = 255;
                    }
                }
            img.putImageData(buf);
        }
    });
})();

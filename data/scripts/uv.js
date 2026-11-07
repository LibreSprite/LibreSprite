// LibreSprite
// Copyright (C) 2026 LibreSprite contributors

const col = app.pixelColor;
const img = app.activeImage;
const h = img.height;
const w = img.width;

for (var y = 0; y < h; ++y) {
    for (var x = 0; x < w; ++x) {
        const u = 255 * x / (w - 1);
        const v = 255 * y / (h - 1);
        img.putPixel(x, y,
            col.rgba(u, v, 0, 255)
        )
    }
}

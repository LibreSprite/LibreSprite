// LibreSprite
// Copyright (C) 2024 LibreSprite contributors

const col = app.pixelColor;
const img = app.activeImage;
const h = img.height;
const w = img.width;

for (var y = 0; y < h; ++y) {
    for (var x = 0; x < w; ++x) {
        const c = Math.random() * 256 >>> 0;
        img.putPixel(x, y, col.rgba(c, c, c, 255))
    }
}

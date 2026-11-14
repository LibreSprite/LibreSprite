// Aseprite
// Copyright (C) 2015-2016 by David Capello

const col = app.pixelColor
const img = app.activeImage

for (let y=0; y<img.height; ++y) {
  for (let x=0; x<img.width; ++x) {
    const c = img.getPixel(x, y)
    const v = (col.rgbaR(c)+
             col.rgbaG(c)+
             col.rgbaB(c))/3

    img.putPixel(x, y,
                 col.rgba(col.rgbaR(c),
                          col.rgbaG(c),
                          col.rgbaB(c),
                          255-v))
  }
}

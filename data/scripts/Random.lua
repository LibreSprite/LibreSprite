-- LibreSprite
-- Copyright (C) 2021 LibreSprite contributors

local col = app.pixelColor()
local img = app.activeImage()

math.randomseed(os.time())

for y=0, img.height() - 1 do
  for x=0, img.width() - 1 do
    local c = math.random(256)
    img.putPixel(x, y, col.rgba(c,c,c,255))
  end
end

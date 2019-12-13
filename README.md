# LibreSprite

*Copyright (C) 2001-2016 David Capello* <br />
*Copyright (C) 2016-2018 LibreSprite contributors* <br />

## Looking for new maintainers

This fork of Aseprite (see below) was only started for archiving purposes.
It was later renamed to LibreSprite to avoid confusion with the proprietary
Aseprite which is still being developed.

There is currently no active developer/maintainer for this project. If you
are interested in taking over the reins, please make yourself known :)

## Introduction

**LibreSprite** is a free and open source program to create animated sprites.
Its main features are:

* Sprites are composed by [**layers** &amp; **frames**](http://www.aseprite.org/docs/timeline/) (as separated concepts).
* Supported [color modes](http://www.aseprite.org/docs/color/): **RGBA**, **Indexed** (palettes up to 256
  colors), and Grayscale.
* Load/save sequence of **PNG** files and **GIF** animations (and
  FLC, FLI, JPG, BMP, PCX, TGA).
* Export/import animations to/from **Sprite Sheets**.
* **Tiled** drawing mode, useful to draw **patterns** and textures.
* **Undo/Redo** for every operation.
* Real-time **animation preview**.
* [**Multiple editors**](http://www.aseprite.org/docs/workspace/#drag-and-drop-tabs) support.
* Pixel-art specific tools like filled **Contour**, **Polygon**, [**Shading**](http://www.aseprite.org/docs/shading/) mode, etc.
* **Onion skinning**

## History

LibreSprite originated as a fork of the [Aseprite](https://www.aseprite.org/)
application, developed by [David Capello](https://github.com/dacap).
Aseprite used to be distributed under the [GNU General Public License](LICENSE.txt)
version 2, but was moved to a proprietary license on
[August 26th, 2016](https://github.com/aseprite/aseprite/commit/5ecc356a41c8e29977f8608d8826489d24f5fa6c).

This fork was made on the [last commit](https://github.com/aseprite/aseprite/commit/03be4aa23db465219962f4c62410f628e7392545)
covered by the GPL-2.0+ license, and is now developed independently of the
proprietary Aseprite.

## Credits

LibreSprite includes color palettes created by:

* [Richard "DawnBringer" Fhager](http://pixeljoint.com/p/23821.htm) palettes, [16 colors](http://pixeljoint.com/forum/forum_posts.asp?TID=12795),  [32 colors](http://pixeljoint.com/forum/forum_posts.asp?TID=16247).
* [Arne Niklas Jansson](http://androidarts.com/) palettes, [16 colors](http://androidarts.com/palette/16pal.htm), [32 colors](http://wayofthepixel.net/index.php?topic=15824.msg144494).

It tries to replicate some pixel-art algorithms:

* [RotSprite](http://forums.sonicretro.org/index.php?showtopic=8848&st=15&p=159754&#entry159754) by Xenowhirl.
* [Pixel perfect drawing algorithm](http://deepnight.net/pixel-perfect-drawing/) by [Sébastien Bénard](https://twitter.com/deepnightfr) and [Carduus](https://twitter.com/CarduusHimself/status/420554200737935361).

And it uses the following third-party libraries:

* [Allegro 4](http://alleg.sourceforge.net/) - [allegro4 license](https://github.com/LibreSprite/tree/master/docs/licenses/allegro4-LICENSE.txt)
* [FreeType](http://www.freetype.org/) - [FTL license](https://github.com/LibreSprite/tree/master/docs/licenses/FTL.txt)
* [Google Test](https://github.com/google/googletest) - [gtest license](https://github.com/LibreSprite/tree/master/docs/licenses/gtest-LICENSE.txt)
* [XFree86](http://www.x.org/) - [XFree86 license](https://github.com/LibreSprite/tree/master/docs/licenses/XFree86-LICENSE.txt)
* [curl](http://curl.haxx.se/) - [curl license](https://github.com/LibreSprite/tree/master/docs/licenses/curl-LICENSE.txt)
* [duktape](http://duktape.org/) - [MIT license](https://github.com/LibreSprite/tree/master/third_party/duktape/LICENSE.txt)
* [giflib](http://sourceforge.net/projects/giflib/) - [giflib license](https://github.com/LibreSprite/tree/master/docs/licenses/giflib-LICENSE.txt)
* [libjpeg](http://www.ijg.org/) - [libjpeg license](https://github.com/LibreSprite/tree/master/docs/licenses/libjpeg-LICENSE.txt)
* [libpng](http://www.libpng.org/pub/png/) - [libpng license](https://github.com/LibreSprite/tree/master/docs/licenses/libpng-LICENSE.txt)
* [libwebp](https://developers.google.com/speed/webp/) - [libwebp license](https://chromium.googlesource.com/webm/libwebp/+/master/COPYING)
* [loadpng](http://tjaden.strangesoft.net/loadpng/) - [zlib license](https://github.com/LibreSprite/tree/master/docs/licenses/ZLIB.txt)
* [modp_b64](https://github.com/LibreSprite/tree/master/third_party/modp_b64/modp_b64.h) - [BSD license](https://github.com/LibreSprite/tree/master/third_party/modp_b64/LICENSE)
* [pixman](http://www.pixman.org/) - [MIT license](http://cgit.freedesktop.org/pixman/plain/COPYING)
* [simpleini](https://github.com/aseprite/simpleini/) - [MIT license](https://github.com/aseprite/simpleini/blob/aseprite/LICENCE.txt)
* [tinyxml](http://www.sourceforge.net/projects/tinyxml) - [zlib license](https://github.com/LibreSprite/tree/master/docs/licenses/ZLIB.txt)
* [zlib](http://www.gzip.org/zlib/) - [ZLIB license](https://github.com/LibreSprite/tree/master/docs/licenses/ZLIB.txt)

## License

This program is distributed under the [GNU General Public License](LICENSE.txt)
version 2, which means that compiled versions can be generated under GPL terms.

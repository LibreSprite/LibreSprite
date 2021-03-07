# LibreSprite Source Code

If you are here is because you want to learn about LibreSprite source
code. We'll try to write in these `README.md` files a summary of each
module/library.

# Modules & Libraries

LibreSprite is separated in the following layers/modules:

## Level 0: Completely independent modules

These libraries are easy to be used and embedded in other software
because they don't depend on any other component.

  * [allegro](allegro/): Modified version of [Allegro](http://alleg.sourceforge.net/) library, used for keyboard/mouse input, and drawing 2D graphics on screen.
  * [base](base/): Core/basic stuff, multithreading, utf8, sha1, file system, memory, etc.
  * [clip](https://github.com/aseprite/clip): Clipboard library.
  * [css](css/): Pseudo-style sheet library.
  * [fixmath](fixmath/): Fixed point operations (original code from Allegro code by Shawn Hargreaves).
  * [flic](https://github.com/aseprite/flic): Library to load/save FLI/FLC files.
  * [gfx](gfx/): Abstract graphics structures like point, size, rectangle, region, color, etc.
  * [observable](https://github.com/dacap/observable): Signal/slot functions.
  * [scripting](scripting/): JavaScript engine.
  * [undo](https://github.com/aseprite/undo): Generic library to manage a history of undoable commands.
  * [wacom](wacom/): Wacom definitions to use Wintab API.

## Level 1

  * [cfg](cfg/) (base): Library to load/save .ini files.
  * [gen](gen/) (base): Helper utility to generate C++ files from different XMLs.
  * [net](net/) (base): Networking library to send HTTP requests.

## Level 2

  * [doc](doc/) (base, fixmath, gfx): Document model library.
  * [she](she/) (allegro, base, gfx, wacom): A wrapper for the Allegro library.

## Level 3

  * [filters](filters/) (base, doc, gfx): Effects for images.
  * [render](render/) (base, doc, gfx): Library to render documents.
  * [ui](ui/) (base, gfx, she): Portable UI library (buttons, windows, text fields, etc.)

## Level 4

  * [iff](iff/) (base, doc, render): Image File Formats library (load/save documents).

## Level 5

  * [app](app/) (allegro, base, doc, filters, fixmath, gfx, iff, pen, render, scripting, she, ui, undo)

## Level 6

  * [main](main/) (app, base, she, ui)

# Debugging Tricks

On Windows, you can use F5 to show the amount of used memory. Also
`Ctrl+Shift+Q` crashes the application in case that you want to test
the anticrash feature or your need a memory dump file.

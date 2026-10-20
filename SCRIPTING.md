# [class Sprite]
## Properties:
   - `palette`: read-only. Returns the sprite's palette.
   - `selection`: Placeholder. Do not use.
   - `height`: read+write. Returns and sets the height of the sprite.
   - `width`: read+write. Returns and sets the width of the sprite.
   - `filename`: read-only. Returns the file name of the sprite.
   - `colorMode`: read-only. Returns the sprite's ColorMode.
   - `layerCount`: read-only. Returns the amount of layers in the sprite.

## Methods:
   - `loadPalette(fileName)`:
     - fileName: The name of the palette file to load
      returns: Nothing
      loads a palette file.

   - `crop(x, y, width, height)`:
     - x: The left-most edge of the crop.
     - y: The top-most edge of the crop.
     - width: The width of the cropped area.
     - height: The height of the cropped area.
      returns: Nothing
      crops the sprite to the specified dimensions.

   - `saveAs(fileName, asCopy)`:
     - fileName: String. The new name of the file
     - asCopy: If true, the file is saved as a copy. Requires fileName to be specified.
      returns: Nothing
      saves the sprite.

   - `resize(width, height)`:
     - width: The new width.
     - height: The new height.
      returns: Nothing
      resizes the sprite.

   - `save()`:
      returns: Nothing
      saves the sprite.

   - `commit()`:
      returns: Nothing
      commits the current transaction.

   - `layer(layerNumber)`:
     - layerNumber: The number of they layer, starting with zero from the bottom.
      returns: a Layer object or null if invalid.
      allows you to access a given layer.



# global storage [class Storage]
## No Properties.

## Methods:
   - `decodeBase64()`:
      returns: Nothing

   - `get()`:
      returns: Nothing

   - `save()`:
      returns: Nothing

   - `set()`:
      returns: Nothing

   - `fetch()`:
      returns: Nothing

   - `load()`:
      returns: Nothing

   - `unload()`:
      returns: Nothing



# global filesystem [class fs]
## No Properties.

Relative paths resolve against the running script's directory; absolute
paths are used as-is.

## Methods:
    - `readFile(path, options)`:
      - path: The file to read.
      - options: Optional. `{encoding: "text" | "binary" | "base64"}` or `{binary: true}` / `{base64: true}`.
       returns: A Promise that resolves to a string (text), a Uint8Array (binary) or a base64 string (base64). Rejects on error.
       Reads a file.

    - `writeFile(path, content, options)`:
      - path: The file to write. Parent directories are created if missing.
      - content: A string or a Uint8Array.
      - options: Optional. `{encoding: "base64"}` or `{base64: true}` decodes the content as base64.
       returns: A Promise that resolves to the number of bytes written.
       Writes a file.

    - `stat(path)`:
      - path: The path to inspect.
       returns: A Promise that resolves to `{isFile, isDirectory, size, lastModified, path}`.
       Gets file or directory info.

    - `listDir(path)`:
      - path: The directory to list.
       returns: A Promise that resolves to an array of `{name, isFile, isDirectory, size, lastModified}`.
       Lists a directory.

    - `copyFile(srcPath, destPath, append)`:
      - srcPath: The source file.
      - destPath: The destination file. Parent directories are created if missing.
      - append: Optional. If true, appends to the destination.
       returns: A Promise that resolves to the number of bytes copied.
       Copies a file.

    - `deleteFile(path)`:
      - path: The file to delete.
       returns: A Promise that resolves to `{path, deleted: true}`.
       Deletes a file.

    - `shareFile(path)`:
      - path: The file to share.
       returns: true
       Sends the file to the system share sheet (Android only).
       Throws on other platforms.

    - `getCurrentPath()`:
       returns: The current working directory.

    - `getExecutablePath()`:
       returns: The app executable path ("" on the web build).

    - `getCachePath()`:
       returns: A writable cache directory.


# [class PalettelistboxWidget]
## Properties:
   - `selected`:
   - `id`:

## Methods:
   - `addPalette()`:
      returns: Nothing



# [class pixelColor]
## No Properties.

## Methods:
   - `grayaA(color)`:
     - color: A 32-bit color in 888 RGBA format
      returns: The alpha component of the color
      Extracts the alpha (opacity) from a 32-bit color

   - `grayaV(color)`:
     - color: A 32-bit color in 888 RGBA format
      returns: The luminance Value of the color
      Extracts the luminance from a 32-bit color

   - `rgbaA(color)`:
     - color: A 32-bit color in 8888 RGBA format
      returns: The alpha component of the color
      Extracts the alpha channel from a 32-bit color

   - `graya(gray, alpha)`:
     - gray: The luminance of color
     - alpha: The alpha (opacity) of the color)
      returns: The color with the given luminance/opacity

   - `rgba(r, g, b, a)`:
     - r: red, 0-255.
     - g: green, 0-255.
     - b: blue, 0-255.
     - a: alpha (opacity), 0-255.
      returns: A 32-bit color in 8888 RGBA format.
      Converts R, G, B, A values into a single 32-bit RGBA color.

   - `rgbaG(color)`:
     - color: A 32-bit color in 8888 RGBA format
      returns: The green component of the color
      Extracts the green channel from a 32-bit color

   - `rgbaB(color)`:
     - color: A 32-bit color in 8888 RGBA format
      returns: The blue component of the color
      Extracts the blue channel from a 32-bit color

   - `rgbaR(color)`:
     - color: A 32-bit color in 8888 RGBA format
      returns: The red component of the color
      Extracts the red channel from a 32-bit color



# [class LabelWidget]
## Properties:
   - `text`:
   - `id`:

## No Methods.



# [class TextBoxWidget]
## Properties:
   - `text`: read+write. The text to display (multiline supported).
   - `align`: read+write. The alignment flags as an int (e.g.
      LEFT = 0x00040000, TOP = 0x00200000). OR the WORDWRAP flag
      (0x02000000) to enable text wrapping.
   - `id`:

## No Methods.

A read-only multiline text display (like a multiline Label, not an
editable text box).


# [class LinkLabel]
## Properties:
   - `text`: read+write. The link text to display.
   - `url`: read+write. The URL to open in the OS handler when clicked.
   - `id`:

## No Methods.

A clickable text label. Clicking opens its URL in the OS handler (the
built-in behavior of the widget) and raises the `<id>_click` event.


# [class Layer]
## Properties: 
   - `flags`: read-only. Returns all flags OR'd together as an int
   - `isContinuous`: read-only. Prefer to link cels when the user copies them.
   - `celCount`: read-only. Returns the number of cels.
   - `isMovable`: read-only. Returns true if the layer is movable.
   - `isVisible`: read+write. Gets/sets whether the layer is visible or not.
   - `isTransparent`: read-only. Returns true if the layer is a non-background image layer.
   - `isBackground`: read-only. Returns true if the layer is a background layer.
   - `isImage`: read-only. Returns true if the layer is an image, false if it is a folder.
   - `isEditable`: read+write. Gets/sets whether the layer is editable (unlocked) or not (locked).
   - `name`: read+write. The name of the layer.

## Methods:
   - `cel(index)`:
     - index: The number of the Cel
      returns: A Cel object or null if an invalid index is passed
      retrieves a Cel

   - `createCel(frame)`:
     - frame: The number of the frame to create the cel in
      returns: The new Cel object
      Creates an empty (transparent) cel at the given frame.
      Throws if the layer is a folder, the frame is out of range,
      or a cel already exists at that frame.



# [class IntentryWidget]
## Properties:
   - `value`:
   - `min`:
   - `max`:
   - `id`:

## No Methods.


# [class Image]
The wrapper re-resolves its site's image on every call (the site is the
document, layer and frame the image was taken from): a stroke's cel trim or a
canvas resize may replace (or remove) the image object underneath. When the
site no longer has an image, every method throws.

`putPixel`, `putImageData` and `clear` are raw, format-dependent writes and
are undoable: each call is one undo step (`command.Undo` reverts it), and a
loop wrapped in `app.transaction(fn)` commits as one undo step. For drawing
(blending, brushes, shapes) use `app.tools`. The bulk idiom is
`getImageData()` -> mutate the array in JS -> `putImageData(data)` (one undo
step). A write into a non-active document's image makes that document active
(the same side effect as `sprite.save`); the undo lands in its history.

## Properties:
   - `format`: read-only. The PixelFormat of the image.
   - `stride`: read-only. The number of bytes per image row.
   - `height`: read-only. The height of the image.
   - `width`: read-only. The width of the image.

## Methods:
   - `putPixel(x, y, color)`:
     - x: integer
     - y: integer
     - color: a 32-bit color in 8888 RGBA format. On indexed images it is a
       palette index (raw write, no palette conversion).
      returns: Nothing
      writes the color onto the image at the given coordinate. Out of bounds
      is a no-op. One undo step; inside `app.transaction(fn)` the calls join
      the scope's transaction.

   - `getImageData()`:
      returns: All pixels in a Uint8Array
      creates an array containing all of the image's pixels.

   - `putImageData(data)`:
     - data: All of the pixels in the image.
      returns: Nothing
      writes the given pixels onto the image. Must be the same size as the
      image. One undo step; inside `app.transaction(fn)` the calls join the
      scope's transaction.

   - `getPNGData()`:
      returns: The image as a Base64-encoded PNG string.
      Encodes the image as a PNG.

   - `clear(color)`:
     - color: a 32-bit color in 8888 RGBA format.
      returns: Nothing
      clears the image with the specified color. One undo step; inside
      `app.transaction(fn)` the calls join the scope's transaction.

   - `getPixel(x, y)`:
     - x: integer
     - y: integer
      returns: a color value
      reads a color from the given coordinate of the image.



# [class PN2ui6DialogE]
## Properties:
   - `canClose`: write only. Determines if the user can close the dialog window.
   - `title`: read+write. Sets the title of the dialog window.
   - `id`:

## Methods:
   - `addBreak()`:
      returns: Nothing

   - `addPaletteListBox()`:
      returns: Nothing

   - `addColor(id, color)`:
     - id: The widget id (use it to retrieve the widget with get()).
     - color: The initial color, a 32-bit color in 8888 RGBA format.
      returns: The new Color object (the same instance as get(id)).
      Adds a color picker (a swatch that opens the color selection popup
      on click).

   - `addEntry()`:
      returns: Nothing

   - `addButton()`:
      returns: Nothing

   - `addTextBox(text, id)`:
     - text: The text to display (multiline supported).
     - id: The widget id (use it to retrieve the widget with get()).
      returns: The new TextBox object (the same instance as get(id)).
      Adds a read-only multiline text display (not editable; use Entry for
      editable text).

   - `addLinkLabel(text, url, id)`:
     - text: The link text to display.
     - url: The URL to open in the OS handler when clicked.
     - id: The widget id (use it to retrieve the widget with get()).
      returns: The new LinkLabel object (the same instance as get(id)).
      Adds a clickable text label that opens its URL in the OS handler
      when clicked (also raises the `<id>_click` event).

   - `addCheckBox()`:
      returns: Nothing

   - `add()`:
      returns: Nothing

   - `close()`:
      returns: Nothing

   - `addLabel()`:
      returns: Nothing

   - `addDropDown(id)`:
     - id: The widget id (use it to retrieve the widget with get()).
      returns: The new Dropdown object (the same instance as get(id)).
      Adds an empty dropdown (options are added via Dropdown.addOption()).

   - `addListBox(id)`:
     - id: The widget id (use it to retrieve the widget with get()).
      returns: The new ListBox object (the same instance as get(id)).
      Adds an empty list box (items are added via ListBox.add()).

   - `addSlider(id, min, max)`:
     - id: The widget id (use it to retrieve the widget with get()).
     - min: The minimum value (also the initial value).
     - max: The maximum value.
      returns: The new Slider object (the same instance as get(id)).
      Adds a slider with the given range; the value is persisted to storage
      (keyed by the widget id) and raises the `<id>_change` event when the
      user moves it.

   - `addIntEntry()`:
      returns: Nothing

   - `get()`:
      returns: Nothing



# global console [class Console]
## No Properties.

## Methods:
   - `assert()`:
      returns: Nothing

   - `log()`:
      returns: Nothing



# [class ButtonWidget]
## Properties:
   - `text`:
   - `id`:

## No Methods.


# [class Color]
## Properties:
   - `color`: read+write. A 32-bit color in 8888 RGBA format
      (the pixelColor.rgba() format). Setting it does not raise the change
      event; user edits do.
   - `id`:

## No Methods.

A color picker: a swatch showing the current color that opens the color
selection popup on click. The selected color is persisted to storage (keyed
by the widget id) and raises the `<id>_change` event when the user picks a
color.


# [class CheckBoxWidget]
## Properties:
   - `text`:
   - `id`:
   - `checked`:

## No Methods.


# [class Dropdown]
## Properties:
   - `id`:
   - `selected`: read+write. The selected option's index (-1 if none).
      Setting it persists to storage but does not raise the change event;
      user selection does.
   - `value`: read+write. The selected option's text ("" if none).
      Setting it selects the matching option without raising the change event.
   - `count`: read-only. The number of options.

## Methods:
   - `addOption(text)`:
     - text: The option text.
      returns: Nothing
      Appends a dropdown option.

The selected option's text is persisted to storage (keyed by the widget id)
and raises the `<id>_change` event when the selection changes.


# [class ListBox]
## Properties:
   - `id`:
   - `selectedIndex`: read+write. The selected item's index (-1 if none).
      Setting it persists to storage but does not raise the change event;
      user selection does.
   - `selected`: read only. The selected item's text ("" if none).
   - `count`: read-only. The number of items.

## Methods:
   - `add(text)`:
     - text: The item text.
      returns: The index of the new item.
      Appends a list item.

   - `remove(index)`:
     - index: The index of the item to remove.
      returns: Nothing

The selected item's text is persisted to storage (keyed by the widget id)
and raises the `<id>_change` event when the selection changes ("" if no item
is selected).


# [class Slider]
## Properties:
   - `id`:
   - `min`: read+write. The minimum value.
   - `max`: read+write. The maximum value.
   - `value`: read+write. The integer value. Setting it persists to storage
      but does not raise the change event; user edits do.

## No Methods.

The value is persisted to storage (keyed by the widget id) and raises the
`<id>_change` event when the user moves the slider.


# [class command]
## No Properties.

## Methods:
   - `Zoom()`:
      returns: Nothing
      Zoom in

   - `ToggleFullscreen()`:
      returns: Nothing
      Toggle Fullscreen

   - `Timeline()`:
      returns: Nothing
      Switch Timeline

   - `TiledMode()`:
      returns: Nothing
      Tiled Mode

   - `SymmetryMode()`:
      returns: Nothing
      Symmetry Mode

   - `SpriteSize()`:
      returns: Nothing
      Sprite Size

   - `SnapToGrid()`:
      returns: Nothing
      Snap to Grid

   - `ShowLayerEdges()`:
      returns: Nothing
      Show Layer Edges

   - `TogglePreview()`:
      returns: Nothing
      Toggle Preview

   - `ShowGrid()`:
      returns: Nothing
      Show Grid

   - `ShowExtras()`:
      returns: Nothing
      Show Extras

   - `ShowBrushPreview()`:
      returns: Nothing
      Show Brush Preview

   - `Share()`:
      returns: Nothing
      Share

   - `SetSameInk()`:
      returns: Nothing
      Same Ink in All Tools

   - `SetInkType()`:
      returns: Nothing
      Set Ink Type: Simple Ink

   - `SetColorSelector()`:
      returns: Nothing
      Set Color Selector: Color Spectrum

   - `SelectionAsGrid()`:
      returns: Nothing
      Selection as Grid

   - `SelectTile()`:
      returns: Nothing
      Select Tile

   - `Scroll()`:
      returns: Nothing
      Scroll 0 pixels left

   - `SavePalette()`:
      returns: Nothing
      Save Palette

   - `SaveFileCopyAs()`:
      returns: Nothing
      Save File Copy As

   - `SaveFileAs()`:
      returns: Nothing
      Save File As

   - `SaveFile()`:
      returns: Nothing
      Save File

   - `SetLoopSection()`:
      returns: Nothing
      Set Loop Section

   - `RunScript()`:
      returns: Nothing
      Run Script

   - `ReverseFrames()`:
      returns: Nothing
      Reverse Frames

   - `ReselectMask()`:
      returns: Nothing
      Reselect Mask

   - `RescanScripts()`:
      returns: Nothing
      Rescan Scripts

   - `ReplaceColor()`:
      returns: Nothing
      Replace Color

   - `RepeatLastExport()`:
      returns: Nothing
      Repeat Last Export

   - `PlayAnimation()`:
      returns: Nothing
      Play Animation

   - `PixelPerfectMode()`:
      returns: Nothing
      Switch Pixel Perfect Mode

   - `Options()`:
      returns: Nothing
      Options

   - `OpenFile()`:
      returns: Nothing
      Open Sprite

   - `NewSpriteFromSelection()`:
      returns: Nothing
      New Sprite From Selection

   - `MaskByColor()`:
      returns: Nothing
      Mask By Color

   - `NewFrameTag()`:
      returns: Nothing
      New Frame Tag

   - `NewFile()`:
      returns: Nothing
      New File

   - `UndoHistory()`:
      returns: Nothing
      Undo History

   - `GotoNextLayer()`:
      returns: Nothing
      Go to Next Layer

   - `NewBrush()`:
      returns: Nothing
      New Brush

   - `OpenInFolder()`:
      returns: Nothing
      Open In Folder

   - `ClearCel()`:
      returns: Nothing
      Clear Cel

   - `MoveMask()`:
      returns: Nothing
      Move Selection Boundaries 0 pixels left

   - `FrameTagProperties()`:
      returns: Nothing
      Frame Tag Properties

   - `AddColor()`:
      returns: Nothing
      Add Foreground Color to Palette

   - `MoveCel()`:
      returns: Nothing
      Move Cel

   - `MergeDownLayer()`:
      returns: Nothing
      Merge Down Layer

   - `MaskAll()`:
      returns: Nothing
      Mask All

   - `SaveMask()`:
      returns: Nothing
      Save Mask

   - `LoadMask()`:
      returns: Nothing
      LoadMask

   - `LayerProperties()`:
      returns: Nothing
      Layer Properties

   - `LayerFromBackground()`:
      returns: Nothing
      Layer From Background

   - `SetPaletteEntrySize()`:
      returns: Nothing
      Set Palette Entry Size

   - `Launch()`:
      returns: Nothing
      Launch

   - `KeyboardShortcuts()`:
      returns: Nothing
      Keyboard Shortcuts

   - `InvertMask()`:
      returns: Nothing
      Invert Mask

   - `InvertColor()`:
      returns: Nothing
      Invert Color

   - `GridSettings()`:
      returns: Nothing
      Grid Settings

   - `GotoPreviousTab()`:
      returns: Nothing
      Go to Previous tab

   - `ScrollCenter()`:
      returns: Nothing
      Scroll to center of canvas

   - `GotoPreviousLayer()`:
      returns: Nothing
      Go to Previous Layer

   - `GotoPreviousFrameWithSameTag()`:
      returns: Nothing
      Go to Previous Frame with same tag

   - `GotoPreviousFrame()`:
      returns: Nothing
      Go to Previous Frame

   - `GotoNextTab()`:
      returns: Nothing
      Go to Next Tab

   - `AutocropSprite()`:
      returns: Nothing
      Trim Sprite

   - `ImportSpriteSheet()`:
      returns: Nothing
      Import Sprite Sheet

   - `ShowPixelGrid()`:
      returns: Nothing
      Show Pixel Grid

   - `Home()`:
      returns: Nothing
      Home

   - `UnlinkCel()`:
      returns: Nothing
      Unlink Cel

   - `GotoNextFrameWithSameTag()`:
      returns: Nothing
      Go to Next Frame with same tag

   - `CropSprite()`:
      returns: Nothing
      Crop Sprite

   - `GotoLastFrame()`:
      returns: Nothing
      Go to Last Frame

   - `OpenWithApp()`:
      returns: Nothing
      Open With Associated Application

   - `GotoFirstFrame()`:
      returns: Nothing
      Go to First Frame

   - `RemoveFrameTag()`:
      returns: Nothing
      Remove Frame Tag

   - `NewFrame()`:
      returns: Nothing
      New Frame

   - `FullscreenPreview()`:
      returns: Nothing
      Fullscreen Preview

   - `SpriteProperties()`:
      returns: Nothing
      Sprite Properties

   - `NewLayer()`:
      returns: Nothing
      New Layer

   - `FrameProperties()`:
      returns: Nothing
      Frame Properties

   - `DeselectMask()`:
      returns: Nothing
      Deselect Mask

   - `AlternateTouchbar()`:
      returns: Nothing
      Alternate Touchbar

   - `ExportSpriteSheet()`:
      returns: Nothing
      Export Sprite Sheet

   - `NewLayerSet()`:
      returns: Nothing
      New Layer Set

   - `ModifySelection()`:
      returns: Nothing
      Expand Selection

   - `Paste()`:
      returns: Nothing
      Paste

   - `DiscardBrush()`:
      returns: Nothing
      Discard Brush

   - `BackgroundFromLayer()`:
      returns: Nothing
      BackgroundFromLayer

   - `DuplicateView()`:
      returns: Nothing
      Duplicate View

   - `About()`:
      returns: Nothing
      About

   - `DeveloperConsole()`:
      returns: Nothing
      Developer Console

   - `DuplicateSprite()`:
      returns: Nothing
      Duplicate Sprite

   - `LinkCels()`:
      returns: Nothing
      Links Cels

   - `CopyMerged()`:
      returns: Nothing
      Copy Merged

   - `MaskContent()`:
      returns: Nothing
      Mask Content

   - `DuplicateLayer()`:
      returns: Nothing
      Duplicate Layer

   - `CopyCel()`:
      returns: Nothing
      Copy Cel

   - `Refresh()`:
      returns: Nothing
      Refresh

   - `Copy()`:
      returns: Nothing
      Copy

   - `RemoveFrame()`:
      returns: Nothing
      Remove Frame

   - `SetPalette()`:
      returns: Nothing
      Set Palette

   - `OpenScriptsFolder()`:
      returns: Nothing
      Open Scripts Folder

   - `FlattenLayers()`:
      returns: Nothing
      Flatten Layers

   - `Eyedropper()`:
      returns: Nothing
      Eyedropper

   - `PaletteSize()`:
      returns: Nothing
      Palette Size

   - `ConvolutionMatrix()`:
      returns: Nothing
      Convolution Matrix

   - `info(name)`:
      - name: Command id, e.g. "OpenFile".
      returns: {name, friendlyName, params: [{name, description, required, default}]} or null.
      Describes the parameters a command reads.

   - `clearParameters()`:
      returns: Nothing

   - `Cut()`:
      returns: Nothing
      Cut

   - `PaletteEditor()`:
      returns: Nothing
      Palette Editor

   - `RemoveLayer()`:
      returns: Nothing
      Remove Layer

   - `Clear()`:
      returns: Nothing
      Clear

   - `Exit()`:
      returns: Nothing
      Exit

   - `ColorQuantization()`:
      returns: Nothing
      Create Palette from Current Sprite (Color Quantization)

   - `AlternateToolbar()`:
      returns: Nothing
      Alternate Toolbar

   - `ChangeColor()`:
      returns: Nothing
      Color

   - `ChangeBrush()`:
      returns: Nothing
      Brush

   - `Cancel()`:
      returns: Nothing
      Cancel Current Operation

   - `SwitchColors()`:
      returns: Nothing
      Switch Colors

   - `ShowOnionSkin()`:
      returns: Nothing
      Show Onion Skin

   - `ChangePixelFormat()`:
      returns: Nothing
      Change Pixel Format

   - `ColorCurve()`:
      returns: Nothing
      Color Curve

   - `PasteText()`:
      returns: Nothing
      Insert Text

   - `CelProperties()`:
      returns: Nothing
      Cel Properties

   - `Despeckle()`:
      returns: Nothing
      Despeckle

   - `CloseAllFiles()`:
      returns: Nothing
      Close All Files

   - `LoadPalette()`:
      returns: Nothing
      Load Palette

   - `CanvasSize()`:
      returns: Nothing
      Canvas Size

   - `Undo()`:
      returns: Nothing
      Undo

   - `LayerVisibility()`:
      returns: Nothing
      Layer Visibility

   - `Flip()`:
      returns: Nothing
      Flip Canvas Horizontal

   - `Rotate()`:
      returns: Nothing
      Rotate Sprite 0°

   - `Redo()`:
      returns: Nothing
      Redo

   - `AlternateTimeline()`:
      returns: Nothing
      Alternate Timeline

   - `ShowSelectionEdges()`:
      returns: Nothing
      Show Selection Edges

   - `GotoFrame()`:
      returns: Nothing
      Go to Frame

   - `CloseFile()`:
      returns: Nothing
      Close File

   - `ToggleTouchbar()`:
      returns: Nothing
      Toggle Touchbar

   - `GotoNextFrame()`:
      returns: Nothing
      Go to Next Frame

   - `AdvancedMode()`:
      returns: Nothing
      Advanced Mode

   - `setParameter()`:
      returns: Nothing



# [class EntryWidget]
## Properties:
   - `value`:
   - `maxsize`:
   - `id`:

## No Methods.


# [class Palette]
## Properties: 
   - `length`: 

## Methods: 
   - `set()`: 
      returns: Nothing

   - `get()`: 
      returns: Nothing



# [class Document]
## Properties: 
   - `sprite`:

## Methods:
   - `close()`:
      returns: Nothing



# global ColorMode [class ColorMode]
## Properties:
   - `BITMAP`:
   - `INDEXED`:
   - `GRAYSCALE`:
   - `RGB`:

## No Methods.


# [class Cel]
## Properties:
   - `frame`:
   - `image`: the cel's image (see class Image for its re-resolving behavior).
   - `y`:
   - `x`:

## Methods:
   - `setPosition()`:
      returns: Nothing



# global child_process [module]
## No Properties.

## Methods:
   - `spawn(path, args)`:
      path: Path of the executable to run. A relative path with a slash
       resolves against the script's directory; a bare name is searched
       in PATH.
      args: Array of string arguments (no shell).
      returns: A Process object.
      Starts a detached process that runs independently of the script and
      outlives the script run. The process inherits the environment and runs
      with the script's directory as its working directory. Throws if the
      process cannot be started.

   - `exec(path, args)`:
      path/args: as in spawn().
      returns: A Promise that resolves to {exitCode, output}.
      Runs the process, captures its merged stdout and stderr, and resolves
      when the process exits (without blocking the app). Rejects if the
      process cannot be started or its output exceeds 1 MiB.

   Not available in the web build (the functions throw).


# [class Process]
## Properties:
   - `pid`: read-only. The process id.
   - `exitCode`: read-only, can be null. null until the process exits,
      then its exit code.

## Methods:
   - `kill()`:
      returns: A bool.
      Terminates the process. False if it does not exist.


# global app [class App]
## Properties:
   - `platform`: read-only. Returns one of: emscripten, windows, macos, android, linux.
   - `version`: read-only. Returns LibreSprite's current version as a string.
   - `isHeadless`: read-only. True when running without a visible UI (--batch/--shell).
   - `activeDocument`: read-only. Returns the currently active Document.
   - `command`: read-only. Returns an object with functions for running commands.
   - `tools`: read-only. Returns the tools object (freehand drawing tools).
   - `activeSprite`: read-only. Returns the currently active Sprite.
   - `activeLayerNumber`: read-only. Returns the number of the current layer.
   - `activeImage`: read-only, can be null. Returns the current layer/frame's image (see class Image for its re-resolving behavior).
   - `pixelColor`: read-only. Returns an object with functions for color conversion.
   - `activeFrameNumber`: read-only. Returns the number of the currently active animation frame.

## Methods:
   - `launch()`:
      returns: Nothing

   - `open()`:
      returns: Nothing
      Opens a document for editing

   - `yield(event)`:
     - event: Name of the event to be raised. The default is yield.
      returns: Nothing
      Schedules a yield event on the next frame

   - `createDialog()`:
      returns: Nothing
      Creates a dialog window

   - `listFonts()`:
      returns: An array of absolute paths to the .ttf/.otf font files
      in the system font directories (the same set the GUI's font
      picker shows), sorted by file name. An empty array when no
      fonts are found. Pass a path to `tools.text`'s `font` option:

          tools.text({ x: 0, y: 0, string: "hi",
                       font: app.listFonts()[0], size: 12 });

   - `critical(fn)`:
     - fn: A sync or async function.
      returns: fn's return value (a Promise when fn is async), so callers
      can `await app.critical(...)`.
      Runs fn with the script's environment protected from teardown. While
      fn runs - including any async work it awaits - re-running the script
      is refused and the environment is not torn down, so an async
      continuation cannot outlive its environment. Nested `app.critical`
      calls are allowed (the protection ends when the outermost one
      settles). App exit is not blocked by an active critical section.

    - `transaction(fn)`:
      - fn: A sync or async function.
       returns: fn's return value (a Promise when fn is async), so callers
       can `await app.transaction(...)`.
       Runs fn with a shared transaction: all `tools.*` calls and the raw
       image writes (`image.putPixel`, `image.putImageData`, `image.clear`)
       inside fn join the transaction, so the whole group commits as one
       undo step when fn completes (`command.Undo` reverts every call at
       once). A throwing fn rolls back every grouped change (the document
       is exactly as before the call) and the error propagates.
       Only the drawing tools and the raw image writes are grouped: the
       other mutating APIs (`sprite.width =`, `selection.select`,
       `command.*`, ...) keep their own transactions and are not affected
       by the scope.
       The scope is bound to the document active at enter time: drawing
       into a different document inside the scope (e.g. after opening a new
       file) throws an error and the group is rolled back. The bound
       document must stay open while the scope is active: the GUI refuses
       to close it while a scope is open (close it once the scope settles,
       which happens when the returned promise settles or the script
       re-runs).
       Nested `app.transaction` calls are allowed: the inner scope joins
       the outer transaction (the group still commits as one undo step).
       Like `app.critical`, the scope must not outlive the script run: if
       the script is torn down mid-transaction, the open group is rolled
       back.

       Two strokes inside one transaction are one undo step:

           app.transaction(() => {
             tools.stroke({ points: [[5, 5], [10, 5]] });
             tools.stroke({ points: [[20, 5], [25, 5]] });
           });
           // One command.Undo() reverts both strokes.

   - `documentation()`:
      returns: Nothing
      Prints this text.


# global selection [class Selection]
## Properties:
   - `bounds`: read-only. The current selection's bounds as an object
     `{ x, y, width, height }` in sprite coordinates, or null when
     there is no (visible) selection.

## Methods:
   - `select(x, y, w, h)`:
     - x, y: The top-left corner of the rectangular selection, in
       sprite coordinates.
     - w, h: The selection size. Non-positive values deselect.
      returns: Nothing
      Replaces the selection with the given rectangle.

   - `selectAll()`:
     returns: Nothing
     Replaces the selection with the whole sprite bounds.

   - `deselect()`:
     returns: Nothing
     Clears the selection.

   - `lasso(points, options)`:
     - points: Required. An array of `[x, y]` in sprite coordinates
       (the same format as `tools.stroke`'s points, without the
       pressure value). At least one point.
     - options: Optional. An object with the `mode` and `frame` fields
       below.
      returns: Nothing
      Runs the lasso tool: the freehand loop through the points selects
      the enclosed region (contour and fill, like the GUI tool).

   - `polygon(points, options)`:
     - points: Required. An array of `[x, y]` polygon vertices, in
       sprite coordinates. At least three points.
     - options: Optional. An object with the `mode` and `frame` fields
       below.
      returns: Nothing
      Runs the polygonal lasso tool: the polygon through the points
      selects its interior (contour and fill, like the GUI tool).

   - `magicWand(x, y, options)`:
     - x, y: Required. The pixel to select, in sprite coordinates.
     - options: Optional. An object with the `mode`, `tolerance`,
       `contiguous`, `layer` and `frame` fields below.
     - tolerance: Optional. 0-255. How far a pixel's color may differ
       from the clicked color and still be selected. Defaults to the
       tool preference.
     - contiguous: Optional. A boolean. true selects only the region
       connected to (x, y); false selects every matching pixel.
       Defaults to the tool preference.
     - layer: Optional. The layer index (0 = bottom) whose pixels the
       wand reads (the floodfill source). Defaults to the active layer.
      returns: Nothing
      Runs the magic wand tool: selects the pixels of the clicked color
      (flood fill, like the GUI tool).

 All methods operate on the active document's selection and are one
 undo step each (`command.Undo` reverts them); inside
 `app.transaction(fn)` they join the scope's transaction. The
 selection is document-wide (it applies to every frame). The `mode`
 field (lasso, polygon and magicWand) is the selection operation:
 "replace" (default) replaces the current selection with the tool's
 region; "add" keeps it and adds the region; "subtract" keeps it and
 removes the region from it. The `frame` field (all three tool
 methods) is the frame index, defaulting to the active frame: for
 magicWand it selects the frame the wand reads, for lasso and polygon
 it only constrains the tool gesture (the selected region is
 frame-independent).

# global tools [class Tools]
## No Properties.

Runs a GUI drawing-tool gesture with synthetic pointer events, the same GUI
drawing pipeline as the mouse. Each method is one tool gesture and one undo
step (`command.Undo` reverts it); the target document becomes active after the
call. Coordinates are sprite-space integers (like `image.putPixel`); negative
values are allowed for shapes crossing the edge (the pipeline clips them like
the GUI). The layer must be visible and editable and the frame must be in
range. To group several calls into a single undo step, wrap them in
`app.transaction(fn)`. A stroke's commit trims the cel, which replaces (or
removes) the cel's image object; an `Image` wrapper re-resolves on every call
(see class Image).

Every method also accepts these common options:
   - tool: Optional. A tool id. Defaults to the method's tool (`stroke`
     defaults to the GUI's active tool).
   - ink: Optional. An ink id override (e.g. "paint", "eraser",
     "replace_fg_with_bg"). Defaults to the tool's ink.
   - brush: Optional. A brush object with the fields below. Defaults to the
     GUI's active brush for the tool.
   - color: Optional. A 32-bit color in 8888 RGBA format (the
     `pixelColor.rgba` convention). Sets the foreground color. Defaults to
     the GUI foreground color.
   - opacity: Optional. 0-255. Defaults to the tool preference.
   - layer: Optional. The layer index (0 = bottom). Defaults to the active
     layer.
   - frame: Optional. The frame index. Defaults to the active frame.

## Methods:
   - `stroke(options)`:
     - options: an object; only `points` is required, plus the common options.
     - tool: Optional. "pencil", "eraser", "spray", "contour", "blur", or
       "jumble". Defaults to the GUI's active tool (throws when the active
       tool is not a supported freehand tool).
     - filled: Optional. A boolean, for fill-optional tools (contour is
       always filled). Defaults to the tool preference.
     - points: Required. An array of `[x, y]` or `[x, y, pressure]` in sprite
       coordinates. `pressure` is 0-1 (default 1.0). At least one point.
      returns: Nothing
      Runs the freehand tool gesture. The defaults (tool, brush, color,
      opacity, filled) come from the current GUI state.

   - `line(options)`:
     - options: an object; `x1`, `y1`, `x2`, `y2` are required, plus the
       common options.
     - x1, y1: Required. The line start, in sprite coordinates.
     - x2, y2: Required. The line end, in sprite coordinates.
      returns: Nothing
      Runs the line tool.

   - `rectangle(options)`:
     - options: an object; `x1`, `y1`, `x2`, `y2` are required, plus the
       common options.
     - x1, y1: Required. One corner of the rectangle, in sprite coordinates.
     - x2, y2: Required. The opposite corner, in sprite coordinates.
     - filled: Optional. A boolean (default false). false draws the outline,
       true draws a solid rectangle.
      returns: Nothing
      Runs the rectangle tool.

   - `ellipse(options)`:
     - options: an object; `x1`, `y1`, `x2`, `y2` are required, plus the
       common options.
     - x1, y1: Required. One end of the ellipse bounding box, in sprite
       coordinates.
     - x2, y2: Required. The opposite end of the bounding box, in sprite
       coordinates.
     - filled: Optional. A boolean (default false). false draws the outline,
       true draws a solid ellipse.
      returns: Nothing
      Runs the ellipse tool.

   - `curve(options)`:
     - options: an object; `x1` through `y4` are required, plus the common
       options.
     - x1, y1: Required. The curve start, in sprite coordinates.
     - x2, y2: Required. The first control point.
     - x3, y3: Required. The second control point.
     - x4, y4: Required. The curve end.
      returns: Nothing
      Runs the curve tool: a cubic bezier from (x1,y1) to (x4,y4) with
      control points (x2,y2) and (x3,y3). The outline is drawn, not filled.

   - `fill(options)`:
     - options: an object; `x` and `y` are required, plus the common options.
     - x, y: Required. The pixel to fill, in sprite coordinates.
     - tolerance: Optional. 0-255. How far a pixel's color may differ from
       the clicked color and still be filled. Defaults to the tool
       preference.
     - contiguous: Optional. A boolean (default the tool preference). true
       fills only the region connected to (x, y); false fills every matching
       pixel in the image.
      returns: Nothing
      Runs the paint bucket tool.

    - `text(options)`:
      - options: an object; `x`, `y`, `string`, `font` and `size` are
        required, plus `color`, `antialias`, `layer` and `frame` (the
        `tool`, `ink`, `brush` and `opacity` options do not apply).
      - x, y: Required. Where the text's top-left lands, in sprite
        coordinates. Negative values are allowed; the text is clipped to
        the sprite bounds.
      - string: Required. The text to render (a single line: a newline
        does not start a new line).
      - font: Required. A TTF/OTF font file path. An absolute path is used
        as is; a relative path resolves against the script's directory
        (as in `fs.*`). There is no system-font lookup - the font must be
        a file (see `app.listFonts` for the available font files).
      - size: Required. The font size in pixels, 1-999.
      - color: Optional. A 32-bit color in 8888 RGBA format (the
        `pixelColor.rgba` convention). Defaults to the GUI foreground
        color.
      - antialias: Optional. A boolean. Defaults to the text tool
        preference.
      - layer, frame: As in the common options.
       returns: Nothing
       Rasterizes the text (the same rendering as the GUI's Insert Text
       command) and composites it into the target layer's cel. One undo
       step (`command.Undo` reverts it); inside `app.transaction(fn)` the
       call joins the scope's transaction. The write is clipped to the
       sprite bounds, and with a visible selection only the selected
       pixels receive the text. Text fully outside the sprite draws
       nothing. For an indexed sprite the text is converted to the
       sprite's palette (best fit, no dithering).

The `brush` field is an object:
   - `type`: "circle", "square", "line", or "image".
   - `size`: 1-256, the brush diameter (ignored for `image`).
   - `angle`: 0-359, in degrees (for square/line brushes).
   - `pattern`: "aligned_to_source", "aligned_to_destination", or "paint_brush".
   - `image`: An `Image` object (a cel image or a loaded file); required when
     `type` is "image". The image is stamped along the stroke.


# Script Events

Scripts register for events in the leading `///` comment block at the top
of the file: each `///listen` line adds one event name to the script.
Events are delivered to the event handler, which is set with
`setEventHandler(fn)` (the script's `onEvent` function is registered
automatically for backward compatibility). The handler receives the event
name as its first argument, followed by the event's payload.

   - `init`:
     Raised once, when the script is loaded. If a `///listen` script is
     loaded by an incoming event (its first event arrives before the
     script has run), `init` is raised before that event.

   - `app_boot`:
     Raised once per process, at startup. Scripts that declare
     `///listen app_boot` are loaded at startup and receive the event
     before the files from the command line are opened. The event
     carries no payload (the script can read `app.isHeadless` and
     `LS.version`).

Other events carry their own payload (for example, dialog buttons raise
`<id>_click` with a modifier-mask argument).

# Script Permissions

Scripts that use privileged APIs must declare them in the script header,
using `///permission` lines in the leading `///` comment block (the same
block that carries `///listen` lines). These lines must come before any
other line in the file.

    ///permission fs.read
    ///permission net.fetch
    // The rest of the script...

On the first run of a script, LibreSprite shows a dialog listing the
permissions the script requests that are not granted yet. The user picks
which to grant (all are checked by default). Cancelling the dialog stops
the script from running; the next run asks again. The grants are saved
per script path and can be changed or removed from
Preferences > Script Permissions (removing a record makes the dialog
appear again the next time the script runs).

## Permissions
   - `fs.read`:
     `fs.readFile`, `fs.stat`, `fs.listDir`, `fs.shareFile`.

   - `fs.write`:
     `fs.writeFile`, `fs.copyFile`, `fs.deleteFile`.

   - `net.fetch`:
     `fetch` and `storage.fetch` (network requests).

   - `net.serve`:
     `HTTPServer` (listening on a port). Serving files from
     `staticFilePath` additionally requires `fs.read`.

   - `app.launch`:
     `app.launch` (opens a file/URL with the operating system),
     `child_process.spawn` and `child_process.exec` (launch processes).

The `fs.getCurrentPath`, `fs.getExecutablePath` and `fs.getCachePath`
functions are not gated.

A gated call made by a script without the permission fails with an error
like `Permission denied: 'fs.read' permission not granted to this
script` (the promise rejects for `fs`/`fetch`/`child_process.exec`,
`child_process.spawn` throws, `app.launch` returns false, `storage.fetch`
logs to the console, and `HTTPServer.start()` returns `0` with `server.error`
set for `net.serve`).

Headless runs (`--script` without a user interface) cannot show the
dialog: the requested permissions are granted automatically with a
warning printed to stdout.

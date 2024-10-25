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
     - fileName: String. The new name of the file.
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
     - layerNumber: The number of the layer, starting with zero from the bottom.
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



# [class IntentryWidget]
## Properties:
   - `value`:
   - `min`:
   - `max`:
   - `id`:

## No Methods.


# [class Image]
## Properties:
   - `format`: read-only. The PixelFormat of the image.
   - `stride`: read-only. The number of bytes per image row.
   - `height`: read-only. The height of the image.
   - `width`: read-only. The width of the image.

## Methods:
   - `putPixel(x, y, color)`:
     - x: integer
     - y: integer
     - color: a 32-bit color in 8888 RGBA format.
      returns: Nothing
      writes the color onto the image at the the given coordinate.

   - `getImageData()`:
      returns: All pixels in a Uint8Array
      creates an array containing all of the image's pixels.

   - `putImageData(data)`:
     - data: All of the pixels in the image.
      returns: Nothing
      writes the given pixels onto the image. Must be the same size as the image.

   - `getPNGData()`:
      returns: The image as a Base64-encoded PNG string.
      Encodes the image as a PNG.

   - `clear(color)`:
     - color: a 32-bit color in 8888 RGBA format.
      returns: Nothing
      clears the image with the specified color.

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

   - `addEntry()`:
      returns: Nothing

   - `addButton()`:
      returns: Nothing

   - `add()`:
      returns: Nothing

   - `close()`:
      returns: Nothing

   - `addLabel()`:
      returns: Nothing

   - `addDropDown()`:
      returns: Nothing

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
      Go to Previous Frame with the same tag

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
      Go to Next Frame with the same tag

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
   - `image`:
   - `y`:
   - `x`:

## Methods:
   - `setPosition()`:
      returns: Nothing



# global app [class App]
## Properties:
   - `platform`: read-only. Returns one of: emscripten, windows, macos, android, linux.
   - `version`: read-only. Returns LibreSprite's current version as a string.
   - `activeDocument`: read-only. Returns the currently active Document.
   - `command`: read-only. Returns an object with functions for running commands.
   - `activeSprite`: read-only. Returns the currently active Sprite.
   - `activeLayerNumber`: read-only. Returns the number of the current layer.
   - `activeImage`: read-only, can be null. Returns the current layer/frame's image.
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

   - `documentation()`:
      returns: Nothing
      Prints this text.
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
     - layerNumber: The number of the layer, starting with zero from the bottom.
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
     - alpha: The alpha (opacity) of the color
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



# [class IntentryWidget]
## Properties:
   - `value`:
   - `min`:
   - `max`:
   - `id`:

## No Methods.


# [class Image]
## Properties:
   - `format`: read-only. The PixelFormat of the image.
   - `stride`: read-only. The number of bytes per image row.
   - `height`: read-only. The height of the image.
   - `width`: read-only. The width of the image.

## Methods:
   - `putPixel(x, y, color)`:
     - x: integer
     - y: integer
     - color: a 32-bit color in 8888 RGBA format.
      returns: Nothing
      writes the color onto the image at the the given coordinate.

   - `getImageData()`:
      returns: All pixels in a Uint8Array
      creates an array containing all of the image's pixels.

   - `putImageData(data)`:
     - data: All of the pixels in the image.
      returns: Nothing
      writes the given pixels onto the image. Must be the same size as the image.

   - `getPNGData()`:
      returns: The image as a Base64-encoded PNG string.
      Encodes the image as a PNG.

   - `clear(color)`:
     - color: a 32-bit color in 8888 RGBA format.
      returns: Nothing
      clears the image with the specified color.

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

   - `addEntry()`:
      returns: Nothing

   - `addButton()`:
      returns: Nothing

   - `add()`:
      returns: Nothing

   - `close()`:
      returns: Nothing

   - `addLabel()`:
      returns: Nothing

   - `addDropDown()`:
      returns: Nothing

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
   - `image`:
   - `y`:
   - `x`:

## Methods:
   - `setPosition()`:
      returns: Nothing



# global app [class App]
## Properties:
   - `platform`: read-only. Returns one of: emscripten, windows, macos, android, linux.
   - `version`: read-only. Returns LibreSprite's current version as a string.
   - `activeDocument`: read-only. Returns the currently active Document.
   - `command`: read-only. Returns an object with functions for running commands.
   - `activeSprite`: read-only. Returns the currently active Sprite.
   - `activeLayerNumber`: read-only. Returns the number of the current layer.
   - `activeImage`: read-only, can be null. Returns the current layer/frame's image.
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

   - `documentation()`:
      returns: Nothing
      Prints this text.

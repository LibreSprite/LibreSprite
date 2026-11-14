// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document_access.h"
#include "app/document_api.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/transaction.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_decorator.h"
#include "app/ui/editor/select_box_state.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/workspace.h"
#include "base/bind.h"
#include "base/log.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "import_sprite_sheet.xml.h"

namespace app {

using namespace ui;

class ImportSpriteSheetWindow : public app::gen::ImportSpriteSheet
                              , public SelectBoxDelegate {
public:
  ImportSpriteSheetWindow(Context* context)
    : m_context(context)
    , m_document(NULL)
    , m_editor(NULL)
    , m_fileOpened(false)
    , m_docPref(nullptr) {
    import()->setEnabled(false);

    static_assert(
      (int)app::SpriteSheetType::Horizontal == 1 &&
      (int)app::SpriteSheetType::Vertical == 2 &&
      (int)app::SpriteSheetType::Rows == 3 &&
      (int)app::SpriteSheetType::Columns == 4,
      "SpriteSheetType enum changed");

    sheetType()->addItem("Horizontal Strip");
    sheetType()->addItem("Vertical Strip");
    sheetType()->addItem("By Rows");
    sheetType()->addItem("By Columns");
    sheetType()->setSelectedItemIndex((int)app::SpriteSheetType::Rows-1);

    sheetType()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onSheetTypeChange, this));
    x()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    y()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    width()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    height()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    selectFile()->Click.connect(base::Bind<void>(&ImportSpriteSheetWindow::onSelectFile, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "ImportSpriteSheet");

    if (m_context->activeDocument()) {
      selectActiveDocument();
      m_fileOpened = false;
    }
  }

  ~ImportSpriteSheetWindow() {
    releaseEditor();
  }

  SpriteSheetType sheetTypeValue() const {
    return (app::SpriteSheetType)(sheetType()->getSelectedItemIndex()+1);
  }

  bool partialTilesValue() const {
    return partialTiles()->isSelected();
  }

  bool ok() const {
    return closer() == import();
  }

  Document* document() const {
    return m_document;
  }

  DocumentPreferences* docPref() const {
    return m_docPref;
  }

  gfx::Rect frameBounds() const {
    return m_rect;
  }

protected:

  void onSheetTypeChange() {
    updateGridState();
  }

  void onSelectFile() {
    Document* oldActiveDocument = m_context->activeDocument();
    Command* openFile = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
    Params params;
    params.set("filename", "");
    openFile->loadParams(params);
    openFile->execute(m_context);

    // The user have selected another document.
    if (oldActiveDocument != m_context->activeDocument()) {
      selectActiveDocument();
      m_fileOpened = true;
    }
  }

  gfx::Rect getRectFromEntries() {
    int w = width()->textInt();
    int h = height()->textInt();

    return gfx::Rect(
      x()->textInt(),
      y()->textInt(),
      std::max<int>(1, w),
      std::max<int>(1, h));
  }

  void onEntriesChange() {
    m_rect = getRectFromEntries();

    // Redraw new rulers position
    if (m_editor) {
      EditorStatePtr state = m_editor->getState();
      if (SelectBoxState* boxState = dynamic_cast<SelectBoxState*>(state.get())) {
        boxState->setBoxBounds(m_rect);
        m_editor->invalidate();
      }
    }
  }

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {
      case kCloseMessage:
        save_window_pos(this, "ImportSpriteSheet");
        break;
    }
    return Window::onProcessMessage(msg);
  }

  void onBroadcastMouseMessage(WidgetsList& targets) override {
    Window::onBroadcastMouseMessage(targets);

    // Add the editor as receptor of mouse events too.
    if (m_editor)
      targets.push_back(View::getView(m_editor));
  }

  // SelectBoxDelegate impleentation
  void onChangeRectangle(const gfx::Rect& rect) override {
    m_rect = rect;

    x()->setTextf("%d", m_rect.x);
    y()->setTextf("%d", m_rect.y);
    width()->setTextf("%d", m_rect.w);
    height()->setTextf("%d", m_rect.h);
  }

  std::string onGetContextBarHelp() override {
    return "Select bounds to identify sprite frames";
  }

private:
  void selectActiveDocument() {
    Document* oldDocument = m_document;
    m_document = m_context->activeDocument();

    // If the user already have selected a file, we have to destroy
    // that file in order to select the new one.
    if (oldDocument) {
      releaseEditor();

      if (m_fileOpened) {
        DocumentDestroyer destroyer(m_context, oldDocument, 100);
        destroyer.destroyDocument();
      }
    }

    captureEditor();

    import()->setEnabled(m_document ? true: false);

    if (m_document) {
      m_docPref = &Preferences::instance().document(m_document);

      if (m_docPref->importSpriteSheet.type() >= app::SpriteSheetType::Horizontal &&
          m_docPref->importSpriteSheet.type() <= app::SpriteSheetType::Columns)
        sheetType()->setSelectedItemIndex((int)m_docPref->importSpriteSheet.type()-1);
      else
        sheetType()->setSelectedItemIndex((int)app::SpriteSheetType::Rows-1);

      gfx::Rect defBounds = m_docPref->importSpriteSheet.bounds();
      if (defBounds.isEmpty())
        defBounds = m_docPref->grid.bounds();
      onChangeRectangle(defBounds);

      partialTiles()->setSelected(m_docPref->importSpriteSheet.partialTiles());
      onEntriesChange();
    }
  }

  void captureEditor() {
    ASSERT(m_editor == NULL);

    if (m_document && !m_editor) {
      m_rect = getRectFromEntries();
      m_editor = current_editor;
      m_editorState.reset(
        new SelectBoxState(
          this, m_rect,
          SelectBoxState::Flags(
            int(SelectBoxState::Flags::Rulers) |
            int(SelectBoxState::Flags::Grid))));

      m_editor->setState(m_editorState);
      updateGridState();
    }
  }

  void updateGridState() {
    if (!m_editorState)
      return;

    int flags = int(SelectBoxState::Flags::Rulers);
    switch (sheetTypeValue()) {
      case SpriteSheetType::Horizontal:
        flags |= int(SelectBoxState::Flags::HGrid);
        break;
      case SpriteSheetType::Vertical:
        flags |= int(SelectBoxState::Flags::VGrid);
        break;
      case SpriteSheetType::Rows:
      case SpriteSheetType::Columns:
        flags |= int(SelectBoxState::Flags::Grid);
        break;
    }

    static_cast<SelectBoxState*>(m_editorState.get())->setFlags(SelectBoxState::Flags(flags));
    m_editor->invalidate();
  }

  void releaseEditor() {
    if (m_editor) {
      m_editor->backToPreviousState();
      m_editor = NULL;
    }
  }

  Context* m_context;
  Document* m_document;
  Editor* m_editor;
  EditorStatePtr m_editorState;
  gfx::Rect m_rect;

  // True if the user has been opened the file (instead of selecting
  // the current document).
  bool m_fileOpened;

  DocumentPreferences* m_docPref;
};

class ImportSpriteSheetCommand : public Command {
public:
  ImportSpriteSheetCommand();
  Command* clone() const override { return new ImportSpriteSheetCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  std::vector<CommandParamInfo> onDescribeParams() const override;
  void onExecute(Context* context) override;

private:
  // Cuts the tiles out of the sheet and applies them to the document
  // (shared by the dialog OK path and the param path). Returns false
  // when the rectangle does not create any tile.
  bool importSheet(Context* context, Document* document,
                   DocumentPreferences& docPref, gfx::Rect frameBounds,
                   bool partialTiles, SpriteSheetType sheetType) const;

  std::string m_filename; // Sheet to open; empty = the current document
  int m_x, m_y;           // Tile origin (0 when not specified)
  int m_width, m_height;  // Tile size (0 = not specified)
  int m_sheetType;        // -1 = not specified, else a SpriteSheetType
  int m_partialTiles;     // -1 = not specified, else 0/1
};

ImportSpriteSheetCommand::ImportSpriteSheetCommand()
  : Command("ImportSpriteSheet",
            "Import Sprite Sheet",
            CmdRecordableFlag)
{
  m_x = m_y = m_width = m_height = 0;
  m_sheetType = -1;
  m_partialTiles = -1;
}

void ImportSpriteSheetCommand::onLoadParams(const Params& params)
{
  m_filename = params.get("filename");

  m_x = m_y = m_width = m_height = 0;
  if (params.has_param("x"))      m_x = params.get_as<int>("x");
  if (params.has_param("y"))      m_y = params.get_as<int>("y");
  if (params.has_param("width"))  m_width = params.get_as<int>("width");
  if (params.has_param("height")) m_height = params.get_as<int>("height");

  std::string sheet_type = params.get("sheet-type");
  if (sheet_type == "horizontal")
    m_sheetType = (int)SpriteSheetType::Horizontal;
  else if (sheet_type == "vertical")
    m_sheetType = (int)SpriteSheetType::Vertical;
  else if (sheet_type == "rows")
    m_sheetType = (int)SpriteSheetType::Rows;
  else if (sheet_type == "columns")
    m_sheetType = (int)SpriteSheetType::Columns;
  else
    m_sheetType = -1;

  std::string partial_tiles = params.get("partial-tiles");
  if (partial_tiles == "true")
    m_partialTiles = 1;
  else if (partial_tiles == "false")
    m_partialTiles = 0;
  else
    m_partialTiles = -1;
}

std::vector<CommandParamInfo> ImportSpriteSheetCommand::onDescribeParams() const
{
  return {
    {"filename", "Sheet file to open. Empty uses the current document.", false, ""},
    {"x", "Tile x coordinate in the sheet.", false, "0"},
    {"y", "Tile y coordinate in the sheet.", false, "0"},
    {"width", "Tile width in pixels. With height, imports without the dialog.", false, ""},
    {"height", "Tile height in pixels. With width, imports without the dialog.", false, ""},
    {"sheet-type", "Tile layout: horizontal, vertical, rows or columns. Uses the document preference when empty.", false, ""},
    {"partial-tiles", "Import tiles cut by the sprite edges: true or false. Uses the document preference when empty.", false, ""},
  };
}

void ImportSpriteSheetCommand::onExecute(Context* context)
{
  // Non-interactive: the tile bounds are given, so the dialog is
  // skipped.
  if (m_width > 0 && m_height > 0) {
    Document* document = context->activeDocument();

    if (!m_filename.empty()) {
      // Open the sheet through the OpenFile command (like app.open).
      Command* openFile = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
      Params params;
      params.set("filename", m_filename.c_str());
      context->executeCommand(openFile, params);

      // The sheet was not opened (e.g. the file does not exist).
      if (document == context->activeDocument()) {
        Console console(context);
        console.printf("ImportSpriteSheet: the file %s was not opened\n", m_filename.c_str());
        return;
      }
      document = context->activeDocument();
    }

    if (!document) {
      Console console(context);
      console.printf("ImportSpriteSheet: no document to import the sheet from\n");
      return;
    }

    DocumentPreferences& docPref = Preferences::instance().document(document);

    SpriteSheetType sheetType;
    if (m_sheetType >= 0)
      sheetType = (SpriteSheetType)m_sheetType;
    else if (docPref.importSpriteSheet.type() >= SpriteSheetType::Horizontal &&
             docPref.importSpriteSheet.type() <= SpriteSheetType::Columns)
      sheetType = docPref.importSpriteSheet.type();
    else
      sheetType = SpriteSheetType::Rows;

    bool partialTiles = (m_partialTiles >= 0 ? m_partialTiles == 1
                                             : docPref.importSpriteSheet.partialTiles());

    if (!importSheet(context, document, docPref,
                     gfx::Rect(m_x, m_y,
                               std::max<int>(1, m_width),
                               std::max<int>(1, m_height)),
                     partialTiles, sheetType)) {
      Console console(context);
      console.printf("ImportSpriteSheet: the specified rectangle does not create any tile\n");
    }
    return;
  }

  // Interactive only: the window would block headless.
  if (!App::instance()->isGui()) {
    LOG("ImportSpriteSheet: width and height params are required in headless mode\n");
    Console console(context);
    console.printf("ImportSpriteSheet: width and height params are required in headless mode\n");
    return;
  }

  ImportSpriteSheetWindow window(context);

  window.openWindowInForeground();
  if (!window.ok())
    return;

  Document* document = window.document();
  DocumentPreferences* docPref = window.docPref();

  ASSERT(document);
  ASSERT(docPref);
  if (!document)
    return;

  if (!importSheet(context, document, *docPref,
                   window.frameBounds(), window.partialTilesValue(),
                   window.sheetTypeValue())) {
    Alert::show("Import Sprite Sheet"
      "<<The specified rectangle does not create any tile."
      "<<Select a rectangle inside the sprite region."
      "||&OK");
  }
}

bool ImportSpriteSheetCommand::importSheet(
  Context* context,
  Document* document,
  DocumentPreferences& docPref,
  gfx::Rect frameBounds,
  bool partialTiles,
  SpriteSheetType sheetType) const
{
  // The list of frames imported from the sheet
  std::vector<ImageRef> animation;

  try {
    Sprite* sprite = document->sprite();
    frame_t currentFrame = context->activeSite().frame();
    render::Render render;

    // Each sprite in the sheet
    std::vector<gfx::Rect> tileRects;
    int widthStop = sprite->width();
    int heightStop = sprite->height();
    if (partialTiles) {
      widthStop += frameBounds.w-1;
      heightStop += frameBounds.h-1;
    }

    switch (sheetType) {
      case app::SpriteSheetType::Horizontal:
        for (int x=frameBounds.x; x+frameBounds.w<=widthStop; x += frameBounds.w) {
          tileRects.push_back(gfx::Rect(x, frameBounds.y, frameBounds.w, frameBounds.h));
        }
        break;
      case app::SpriteSheetType::Vertical:
        for (int y=frameBounds.y; y+frameBounds.h<=heightStop; y += frameBounds.h) {
          tileRects.push_back(gfx::Rect(frameBounds.x, y, frameBounds.w, frameBounds.h));
        }
        break;
      case app::SpriteSheetType::Rows:
        for (int y=frameBounds.y; y+frameBounds.h<=heightStop; y += frameBounds.h) {
          for (int x=frameBounds.x; x+frameBounds.w<=widthStop; x += frameBounds.w) {
            tileRects.push_back(gfx::Rect(x, y, frameBounds.w, frameBounds.h));
          }
        }
        break;
      case app::SpriteSheetType::Columns:
        for (int x=frameBounds.x; x+frameBounds.w<=sprite->width(); x += frameBounds.w) {
          for (int y=frameBounds.y; y+frameBounds.h<=sprite->height(); y += frameBounds.h) {
            tileRects.push_back(gfx::Rect(x, y, frameBounds.w, frameBounds.h));
          }
        }
        break;
    }

    // As first step, we cut each tile and add them into "animation" list.
    for (const auto& tileRect : tileRects) {
      ImageRef resultImage(
        Image::create(
          sprite->pixelFormat(), tileRect.w, tileRect.h));

      // Render the portion of sheet.
      render.renderSprite(
        resultImage.get(), sprite, currentFrame,
        gfx::Clip(0, 0, tileRect));

      animation.push_back(resultImage);
    }

    if (animation.size() == 0)
      return false;

    // The following steps modify the sprite, so we wrap all
    // operations in a undo-transaction.
    ContextWriter writer(context);
    Transaction transaction(writer.context(), "Import Sprite Sheet", ModifyDocument);
    DocumentApi api = document->getApi(transaction);

    // Add the layer in the sprite.
    LayerImage* resultLayer = api.newLayer(sprite, "Sprite Sheet");

    // Add all frames+cels to the new layer
    for (size_t i=0; i<animation.size(); ++i) {
      // Create the cel.
      auto resultCel = std::make_shared<Cel>(frame_t(i), animation[i]);

      // Add the cel in the layer.
      api.addCel(resultLayer, resultCel);
    }

    // Copy the list of layers (because we will modify it in the iteration).
    LayerList layers = sprite->folder()->getLayersList();

    // Remove all other layers
    for (LayerIterator it=layers.begin(), end=layers.end(); it!=end; ++it) {
      if (*it != resultLayer)
        api.removeLayer(*it);
    }

    // Change the number of frames
    api.setTotalFrames(sprite, frame_t(animation.size()));

    // Set the size of the sprite to the tile size.
    api.setSpriteSize(sprite, frameBounds.w, frameBounds.h);

    transaction.commit();

    docPref.importSpriteSheet.type(sheetType);
    docPref.importSpriteSheet.bounds(frameBounds);
    docPref.importSpriteSheet.partialTiles(partialTiles);
  }
  catch (...) {
    throw;
  }

  update_screen_for_document(document);
  return true;
}

Command* CommandFactory::createImportSpriteSheetCommand()
{
  return new ImportSpriteSheetCommand;
}

} // namespace app

// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/color.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/input_chain_element.h"
#include "app/ui/palette_view.h"
#include "base/connection.h"
#include "base/signal.h"
#include "base/unique_ptr.h"
#include "doc/context_observer.h"
#include "doc/document_observer.h"
#include "doc/documents_observer.h"
#include "doc/pixel_format.h"
#include "doc/sort_palette.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/splitter.h"
#include "ui/tooltips.h"
#include "ui/view.h"

namespace app {

  class ColorButton;
  class ColorSpectrum;
  class ColorTintShadeTone;
  class ColorWheel;
  class CommandExecutionEvent;
  class PaletteIndexChangeEvent;
  class PalettePopup;
  class PalettesLoader;

  class ColorBar : public ui::Box
                 , public PaletteViewDelegate
                 , public doc::ContextObserver
                 , public doc::DocumentObserver
                 , public app::InputChainElement {
    static ColorBar* m_instance;
  public:
    enum class ColorSelector {
      NONE,
      SPECTRUM,
      RGB_WHEEL,
      RYB_WHEEL,
      TINT_SHADE_TONE,
    };

    static ColorBar* instance() { return m_instance; }

    ColorBar(int align);
    ~ColorBar();

    void setPixelFormat(PixelFormat pixelFormat);

    app::Color getFgColor();
    app::Color getBgColor();
    void setFgColor(const app::Color& color);
    void setBgColor(const app::Color& color);

    PaletteView* getPaletteView();

    ColorSelector getColorSelector();
    void setColorSelector(ColorSelector selector);

    // Used by the Palette Editor command to change the status of button
    // when the visibility of the dialog changes.
    void setPaletteEditorButtonState(bool state);

    // ContextObserver impl
    void onActiveSiteChange(const doc::Site& site) override;

    // DocumentObserver impl
    void onGeneralUpdate(doc::DocumentEvent& ev) override;

    // InputChainElement impl
    void onNewInputPriority(InputChainElement* element) override;
    bool onCanCut(Context* ctx) override;
    bool onCanCopy(Context* ctx) override;
    bool onCanPaste(Context* ctx) override;
    bool onCanClear(Context* ctx) override;
    bool onCut(Context* ctx) override;
    bool onCopy(Context* ctx) override;
    bool onPaste(Context* ctx) override;
    bool onClear(Context* ctx) override;
    void onCancel(Context* ctx) override;

    base::Signal0<void> ChangeSelection;

  protected:
    void onAppPaletteChange();
    void onFocusPaletteView();
    void onBeforeExecuteCommand(CommandExecutionEvent& ev);
    void onAfterExecuteCommand(CommandExecutionEvent& ev);
    void onPaletteButtonClick();
    void onRemapButtonClick();
    void onPaletteIndexChange(PaletteIndexChangeEvent& ev);
    void onFgColorChangeFromPreferences();
    void onBgColorChangeFromPreferences();
    void onFgColorButtonChange(const app::Color& color);
    void onBgColorButtonChange(const app::Color& color);
    void onColorButtonChange(const app::Color& color);
    void onPickSpectrum(const app::Color& color, ui::MouseButtons buttons);
    void onReverseColors();
    void onSortBy(doc::SortPaletteBy channel);
    void onGradient();
    void onFixWarningClick(ColorButton* colorButton, ui::Button* warningIcon);
    void setAscending(bool ascending);

    // PaletteViewDelegate impl
    void onPaletteViewIndexChange(int index, ui::MouseButtons buttons) override;
    void onPaletteViewModification(const doc::Palette* newPalette, PaletteViewModification mod) override;
    void onPaletteViewChangeSize(int boxsize) override;
    void onPaletteViewPasteColors(const Palette* fromPal, const doc::PalettePicks& from, const doc::PalettePicks& to) override;
    app::Color onPaletteViewGetForegroundIndex() override;
    app::Color onPaletteViewGetBackgroundIndex() override;

  private:
    void showRemap();
    void hideRemap();
    void setPalette(const doc::Palette* newPalette, const std::string& actionText);
    void setTransparentIndex(int index);
    void updateWarningIcon(const app::Color& color, ui::Button* warningIcon);
    static void fixColorIndex(ColorButton& color);

    class ScrollableView : public ui::View {
    public:
      ScrollableView();
    protected:
      void onPaint(ui::PaintEvent& ev) override;
    };

    class WarningIcon;

    ui::TooltipManager m_tooltips;
    std::shared_ptr<ButtonSet> m_buttons = inject<Widget>{"ButtonSet"};
    base::UniquePtr<PalettePopup> m_palettePopup;
    ui::Splitter m_splitter;
    ui::VBox m_palettePlaceholder;
    ui::VBox m_selectorPlaceholder;
    ScrollableView m_scrollableView;
    PaletteView m_paletteView;
    ui::Button m_remapButton;
    ColorSelector m_selector;
    ColorTintShadeTone* m_tintShadeTone;
    ColorSpectrum* m_spectrum;
    ColorWheel* m_wheel;
    ColorButton m_fgColor;
    ColorButton m_bgColor;
    WarningIcon* m_fgWarningIcon;
    WarningIcon* m_bgWarningIcon;
    bool m_lock;
    bool m_syncingWithPref;
    base::UniquePtr<doc::Palette> m_oldPalette;
    doc::Document* m_lastDocument;
    bool m_ascending;
    base::ScopedConnection m_beforeCmdConn;
    base::ScopedConnection m_afterCmdConn;
    base::ScopedConnection m_fgConn;
    base::ScopedConnection m_bgConn;
    base::ScopedConnection m_appPalChangeConn;
    ui::MouseButtons m_lastButtons;
  };

} // namespace app

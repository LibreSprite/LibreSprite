// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C)      2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/disable_copying.h"
#include "base/injection.h"
#include "base/weak_set.h"
#include "base/safe_ptr.h"
#include "gfx/border.h"
#include "gfx/color.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "gfx/size.h"
#include "ui/base.h"
#include "ui/graphics.h"
#include "ui/property.h"
#include "ui/widget_type.h"
#include "ui/widgets_list.h"

#include <climits>
#include <map>
#include <memory>
#include <string>

#define ASSERT_VALID_WIDGET(widget) ASSERT((widget) != NULL)

namespace she {
  class Font;
}

namespace ui {

  class InitThemeEvent;
  class LoadLayoutEvent;
  class Manager;
  class Message;
  class MouseMessage;
  class PaintEvent;
  class SizeHintEvent;
  class ResizeEvent;
  class SaveLayoutEvent;
  class Theme;
  class Window;

  /* Widgets are the basic visual object in LibreSprite, such as menus and grids.

  Widgets are non-copyable */
  class Widget : public Injectable<Widget>, public std::enable_shared_from_this<Widget> {
  protected:
    // Widget and derivatives should always have protected constructors!
    Widget(WidgetType type = kGenericWidget);

  public:
    void postInject() override;
    virtual ~Widget();

    base::safe_ptr<Widget> safePtr{this};
    Widget* holdUntilAdded() {
      m_hold = shared_from_this();
      return this;
    }

    // Safe way to delete a widget when it is not in the manager message
    // queue anymore.
    void deferDelete();

    static base::weak_set<Widget>& getAll();

    // Properties handler

    typedef std::map<std::string, PropertyPtr> Properties;

    PropertyPtr getProperty(const std::string& name) const;
    void setProperty(PropertyPtr property);

    const Properties& getProperties() const;

    // Main properties.

    WidgetType type() const { return m_type; }
    void setType(WidgetType type) { m_type = type; } // TODO remove this function

    const std::string& id() const { return m_id; }
    void setId(const char* id) { m_id = id; }

    int flags() const { return m_flags; }
    bool hasFlags(int flags) const { return ((m_flags & flags) == flags); }
    void enableFlags(int flags) { m_flags |= flags; }
    void disableFlags(int flags) { m_flags &= ~flags; }

    int align() const { return (m_flags & ALIGN_MASK); }
    void setAlign(int align) {
      m_flags = ((m_flags & PROPERTIES_MASK) |
                 (align & ALIGN_MASK));
    }

    // Text property.

    bool hasText() const { return hasFlags(HAS_TEXT); }

    const std::string& text() const { return m_text; }
    int textInt() const;
    double textDouble() const;
    int textLength() const;
    void setText(const std::string& text);
    void setTextf(const char* text, ...);
    void setTextQuiet(const std::string& text);

    int textWidth() const;
    int textHeight() const;

    gfx::Size textSize() const {
      return gfx::Size(textWidth(), textHeight());
    }

    // ===============================================================
    // COMMON PROPERTIES
    // ===============================================================

    // True if this widget and all its ancestors are visible.
    bool isVisible() const;
    void setVisible(bool state);

    // True if this widget can receive user input (is not disabled).
    bool isEnabled() const;
    void setEnabled(bool state);

    // True if this widget is selected (pushed in case of a button, or
    // checked in the case of a check-box).
    bool isSelected() const;
    void setSelected(bool state);

    // True if this widget wants more space when it's inside a Box
    // parent.
    bool isExpansive() const;
    void setExpansive(bool state);

    // True if this is a decorative widget created by the current
    // theme. Decorative widgets are arranged by the theme instead that
    // the parent's widget.
    bool isDecorative() const;
    void setDecorative(bool state);

    // True if this widget can receive the keyboard focus.
    bool isFocusStop() const;
    void setFocusStop(bool state);

    // True if this widget wants the focus by default when it's shown by
    // first time (e.g. when its parent window is opened).
    void setFocusMagnet(bool state);
    bool isFocusMagnet() const;

    // ===============================================================
    // LOOK & FEEL
    // ===============================================================

    she::Font* font() const;
    void resetFont(she::Font* font = nullptr);

    // Gets the background color of the widget.
    gfx::Color bgColor() const {
      if (gfx::geta(m_bgColor) == 0 && m_parent)
        return m_parent->bgColor();
      else
        return m_bgColor;
    }

    // Sets the background color of the widget
    void setBgColor(gfx::Color color);

    Theme* theme() const { return m_theme; }
    void setTheme(Theme* theme);
    void initTheme();

    // ===============================================================
    // PARENTS & CHILDREN
    // ===============================================================

    Window* window();
    Widget* parent() { return m_parent; }
    Manager* manager();

    // Returns a list of parents, if "ascendant" is true the list is
    // build from child to parents, else the list is from parent to
    // children.
    void getParents(bool ascendant, WidgetsList& parents);

    // Returns a list of children.
    const WidgetsList& children() const { return m_children; }

    Widget* at(int index) { return m_children[index]; }

    // Returns the first/last child or NULL if it doesn't exist.
    Widget* firstChild() {
      return (!m_children.empty() ? m_children.front(): NULL);
    }
    Widget* lastChild() {
      return (!m_children.empty() ? m_children.back(): NULL);
    }

    // Returns the next or previous siblings.
    Widget* nextSibling();
    Widget* previousSibling();

    Widget* pick(const gfx::Point& pt, bool checkParentsVisibility = true);
    bool hasChild(Widget* child);
    bool hasAncestor(Widget* ancestor);
    Widget* findChild(const char* id);

    // Returns a widget in the same window that is located "sibling".
    Widget* findSibling(const char* id);

    // Finds a child with the specified ID and dynamic-casts it to type
    // T.
    template<class T>
    T* findChildT(const char* id) {
      return dynamic_cast<T*>(findChild(id));
    }

    template<class T>
    T* findFirstChildByType() {
      for (auto child : m_children) {
        if (T* specificChild = dynamic_cast<T*>(child))
          return specificChild;
      }
      return NULL;
    }

    void addChild(std::shared_ptr<Widget> child);
    void addChild(Widget* child);

    void removeChild(std::shared_ptr<Widget> child);
    void removeChild(Widget* child);

    void removeAllChildren();
    void replaceChild(Widget* oldChild, Widget* newChild);
    void insertChild(int index, Widget* child);

    // ===============================================================
    // LAYOUT & CONSTRAINT
    // ===============================================================

    void layout();
    void loadLayout();
    void saveLayout();

    void setDecorativeWidgetBounds();

    // ===============================================================
    // POSITION & GEOMETRY
    // ===============================================================

    gfx::Rect bounds() const { return m_bounds; }
    gfx::Point origin() const { return m_bounds.origin(); }
    gfx::Size size() const { return m_bounds.size(); }

    gfx::Rect clientBounds() const {
      return gfx::Rect(0, 0, m_bounds.w, m_bounds.h);
    }

    gfx::Rect childrenBounds() const;
    gfx::Rect clientChildrenBounds() const;

    // Sets the bounds of the widget generating a onResize() event.
    void setBounds(const gfx::Rect& rc);

    // Sets the bounds of the widget without generating any kind of
    // event. This member function must be used if you override
    // onResize() and want to change the size of the widget without
    // generating recursive onResize() events.
    void setBoundsQuietly(const gfx::Rect& rc);
    void offsetWidgets(int dx, int dy);

    const gfx::Size& minSize() const { return m_minSize; }
    const gfx::Size& maxSize() const { return m_maxSize; }
    void setMinSize(const gfx::Size& sz);
    void setMaxSize(const gfx::Size& sz);

    const gfx::Border& border() const { return m_border; }
    void setBorder(const gfx::Border& border);

    int childSpacing() const { return m_childSpacing; }
    void setChildSpacing(int childSpacing);

    void noBorderNoChildSpacing();

    // Flags for getDrawableRegion()
    enum DrawableRegionFlags {
      kCutTopWindows = 1, // Cut areas where are windows on top.
      kUseChildArea = 2,  // Use areas where are children.
    };

    void getRegion(gfx::Region& region);
    void getDrawableRegion(gfx::Region& region, DrawableRegionFlags flags);

    gfx::Point toClient(const gfx::Point& pt) const {
      return pt - m_bounds.origin();
    }
    gfx::Rect toClient(const gfx::Rect& rc) const {
      return gfx::Rect(rc).offset(-m_bounds.x, -m_bounds.y);
    }

    void getTextIconInfo(
      gfx::Rect* box,
      gfx::Rect* text = NULL,
      gfx::Rect* icon = NULL,
      int icon_align = 0, int icon_w = 0, int icon_h = 0);

    // ===============================================================
    // REFRESH ISSUES
    // ===============================================================

    bool isDoubleBuffered() const;
    void setDoubleBuffered(bool doubleBuffered);

    bool isTransparent() const;
    void setTransparent(bool transparent);

    void invalidate();
    void invalidateRect(const gfx::Rect& rect);
    void invalidateRegion(const gfx::Region& region);

    // Returns the region to generate PaintMessages. It's cleared
    // after flushRedraw() is called.
    const gfx::Region& getUpdateRegion() const {
      return m_updateRegion;
    }

    // Generates paint messages for the current update region.
    void flushRedraw();

    GraphicsPtr getGraphics(const gfx::Rect& clip);

    // ===============================================================
    // GUI MANAGER
    // ===============================================================

    bool sendMessage(Message* msg);
    void closeWindow();

    void broadcastMouseMessage(WidgetsList& targets);

    // ===============================================================
    // SIZE & POSITION
    // ===============================================================

    gfx::Size sizeHint();
    gfx::Size sizeHint(const gfx::Size& fitIn);
    void setSizeHint(const gfx::Size& fixedSize);
    void setSizeHint(int fixedWidth, int fixedHeight);

    // ===============================================================
    // MOUSE, FOCUS & KEYBOARD
    // ===============================================================

    void requestFocus();
    void releaseFocus();
    void captureMouse();
    void releaseMouse();

    bool hasFocus();
    bool hasMouse();
    bool hasMouseOver();
    bool hasCapture();

    // Offer the capture to widgets of the given type. Returns true if
    // the capture was passed to other widget.
    bool offerCapture(ui::MouseMessage* mouseMsg, int widget_type);

    // Returns lower-case letter that represet the mnemonic of the widget
    // (the underscored character, i.e. the letter after & symbol).
    int mnemonicChar() const;

  protected:
    // ===============================================================
    // MESSAGE PROCESSING
    // ===============================================================

    virtual bool onProcessMessage(Message* msg);

    // ===============================================================
    // EVENTS
    // ===============================================================

    virtual void onInvalidateRegion(const gfx::Region& region);
    virtual void onSizeHint(SizeHintEvent& ev);
    virtual void onLoadLayout(LoadLayoutEvent& ev);
    virtual void onSaveLayout(SaveLayoutEvent& ev);
    virtual void onResize(ResizeEvent& ev);
    virtual void onPaint(PaintEvent& ev);
    virtual void onBroadcastMouseMessage(WidgetsList& targets);
    virtual void onInitTheme(InitThemeEvent& ev);
    virtual void onSetDecorativeWidgetBounds();
    virtual void onEnable();
    virtual void onDisable();
    virtual void onSelect();
    virtual void onDeselect();
    virtual void onSetText();
    virtual void onSetBgColor();

    void setManager(Manager* manager);

  private:
    void removeChild(const WidgetsList::iterator& it);
    void paint(Graphics* graphics, const gfx::Region& drawRegion);
    bool paintEvent(Graphics* graphics);

    // TODO: Remove when widget can own all children
    std::vector<std::shared_ptr<Widget>> m_ownedChildren;
    bool m_wasInjected = false;
    std::shared_ptr<Widget> m_hold;

    WidgetType m_type;                             // Widget's type
    std::string m_id;                              // Widget's id
    int m_flags = 0;                               // Special boolean properties (see flags in ui/base.h)
    Theme* m_theme = nullptr;                      // Widget's theme
    std::string m_text;                            // Widget text
    mutable she::Font* m_font = nullptr;           // Cached font returned by the theme
    gfx::Color m_bgColor = gfx::ColorNone;         // Background color
    gfx::Rect m_bounds;
    gfx::Region m_updateRegion;                    // Region to be redrawed.
    WidgetsList m_children;                        // Sub-widgets
    Widget* m_parent = nullptr;                    // Who is the parent?
    Manager* m_manager = nullptr;
    std::unique_ptr<gfx::Size> m_sizeHint;
    Properties m_properties;

    // Widget size limits
    gfx::Size m_minSize{0, 0};
    gfx::Size m_maxSize{INT_MAX, INT_MAX};

    gfx::Border m_border;                          // Border separation with the parent
    int m_childSpacing = 0;                        // Separation between children

    DISABLE_COPYING(Widget);
  };

  WidgetType register_widget_type();

} // namespace ui

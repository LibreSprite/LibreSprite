// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "gfx/region.h"
#include "ui/keys.h"
#include "ui/message_type.h"
#include "ui/mouse_buttons.h"
#include "ui/pointer_type.h"
#include "ui/widget.h"

namespace she {
  class Display;
  class EventQueue;
}

namespace ui {

  class LayoutIO;
  class Timer;
  class Window;

  class Manager : public Widget {
  public:
    static Manager* getDefault() {
      return m_defaultManager;
    }

    Manager();
    ~Manager();

    she::Display* getDisplay() { return m_display; }

    void setDisplay(she::Display* display);

    // Executes the main message loop.
    void run();

    // Refreshes the real display with the UI content.
    void flipDisplay();

    // Returns true if there are messages in the queue to be
    // distpatched through jmanager_dispatch_messages().
    bool generateMessages();
    void dispatchMessages();
    void enqueueMessage(std::shared_ptr<Message> msg);

    void addToGarbage(Widget* widget);
    void collectGarbage();

    Window* getTopWindow();
    Window* getForegroundWindow();

    Widget* getFocus();
    Widget* getMouse();
    Widget* getCapture();

    void setFocus(Widget* widget);
    void setMouse(Widget* widget);
    void setCapture(Widget* widget);
    void attractFocus(Widget* widget);
    void focusFirstChild(Widget* widget);
    void freeFocus();
    void freeMouse();
    void freeCapture();
    void freeWidget(Widget* widget);
    void removeMessagesFor(Widget* widget);
    void removeMessagesFor(Widget* widget, MessageType type);

    void addMessageFilter(int message, Widget* widget);
    void removeMessageFilter(int message, Widget* widget);

    void invalidateDisplayRegion(const gfx::Region& region);

    LayoutIO* getLayoutIO();

    bool isFocusMovementKey(Message* msg);

    // Returns the invalid region in the screen to being updated with
    // PaintMessages. This region is cleared when each widget receives
    // a paint message.
    const gfx::Region& getInvalidRegion() const {
      return m_invalidRegion;
    }

    void addInvalidRegion(const gfx::Region& b) {
      m_invalidRegion |= b;
    }

    void removeInvalidRegion(const gfx::Region& b) {
      m_invalidRegion -= b;
    }

    // Mark the given rectangle as a area to be flipped to the real
    // screen
    void dirtyRect(const gfx::Rect& bounds);

    void _openWindow(Window* window);
    void _closeWindow(Window* window, bool redraw_background);

  protected:
    bool onProcessMessage(Message* msg) override;
    void onResize(ResizeEvent& ev) override;
    void onPaint(PaintEvent& ev) override;
    void onSizeHint(SizeHintEvent& ev) override;
    void onBroadcastMouseMessage(WidgetsList& targets) override;
    virtual LayoutIO* onGetLayoutIO();
    virtual void onNewDisplayConfiguration();

    void postInject() override {
      setManager(this);
      Widget::postInject();
    }

  private:
    void generateSetCursorMessage(const gfx::Point& mousePos,
                                  KeyModifiers modifiers,
                                  PointerType pointerType);
    void generateMessagesFromSheEvents();
    void handleMouseMove(const gfx::Point& mousePos,
                         MouseButtons mouseButtons,
                         KeyModifiers modifiers,
                         PointerType pointerType);
    void handleMouseDown(const gfx::Point& mousePos,
                         MouseButtons mouseButtons,
                         KeyModifiers modifiers,
                         PointerType pointerType);
    void handleMouseUp(const gfx::Point& mousePos,
                       MouseButtons mouseButtons,
                       KeyModifiers modifiers,
                       PointerType pointerType);
    void handleMouseDoubleClick(const gfx::Point& mousePos,
                                MouseButtons mouseButtons,
                                KeyModifiers modifiers,
                                PointerType pointerType);
    void handleMouseWheel(const gfx::Point& mousePos,
                          MouseButtons mouseButtons,
                          KeyModifiers modifiers,
                          PointerType pointerType,
                          const gfx::Point& wheelDelta,
                          bool preciseWheel);
    void handleTouchMagnify(const gfx::Point& mousePos,
                            const KeyModifiers modifiers,
                            const double magnification);
    void handleWindowZOrder();

    void pumpQueue();
    static void removeWidgetFromRecipients(Widget* widget, Message* msg);
    static bool someParentIsFocusStop(Widget* widget);
    static Widget* findMagneticWidget(Widget* widget);
    static std::shared_ptr<Message> newMouseMessage(
      MessageType type,
      Widget* widget, const gfx::Point& mousePos,
      PointerType pointerType,
      MouseButtons buttons,
      KeyModifiers modifiers,
      const gfx::Point& wheelDelta = gfx::Point(0, 0),
      bool preciseWheel = false);
    void broadcastKeyMsg(std::shared_ptr<Message> msg);

    static Manager* m_defaultManager;
    static gfx::Region m_dirtyRegion;

    WidgetsList m_garbage;
    she::Display* m_display;
    she::EventQueue* m_eventQueue;
    gfx::Region m_invalidRegion;  // Invalid region (we didn't receive paint messages yet for this).

    // This member is used to make freeWidget() a no-op when we
    // restack a window if the user clicks on it.
    Widget* m_lockedWindow;

    // Current pressed buttons.
    MouseButtons m_mouseButtons;
  };

} // namespace ui

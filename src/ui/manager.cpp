// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

// #define REPORT_EVENTS
// #define DEBUG_PAINT_EVENTS
// #define LIMIT_DISPATCH_TIME

#include <deque>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/manager.h"

#include "base/iterator.h"
#include "base/scoped_value.h"
#include "base/time.h"
#include "she/display.h"
#include "she/event.h"
#include "she/event_queue.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/intern.h"
#include "ui/ui.h"

#ifdef DEBUG_PAINT_EVENTS
#include "base/thread.h"
#endif

#ifdef REPORT_EVENTS
#include <iostream>
#endif

#include <limits>
#include <list>
#include <vector>

namespace ui {

#define ACCEPT_FOCUS(widget)                                    \
  ((((widget)->flags() & (FOCUS_STOP |                          \
                          DISABLED |                            \
                          HIDDEN |                              \
                          DECORATIVE)) == FOCUS_STOP) &&        \
   ((widget)->isVisible()))

static const int NFILTERS = (int)(kFirstRegisteredMessage+1);

struct Filter {
  int message;
  base::safe_ptr<Widget> widget;
  Filter(int message, Widget* widget) : message(message), widget(widget->safePtr) {}
};

typedef std::list<Filter> Filters;

Manager* Manager::m_defaultManager = NULL;
gfx::Region Manager::m_dirtyRegion;

static WidgetsList new_windows; // Windows that we should show
static WidgetsList mouse_widgets_list; // List of widgets to send mouse events
static std::deque<std::shared_ptr<Message>> msg_queue;             // Messages queue
static std::array<Filters, NFILTERS> msg_filters; // Filters for every enqueued message

static base::safe_ptr<Widget> focus_widget;    // The widget with the focus
static Widget* mouse_widget;    // The widget with the mouse
static Widget* capture_widget;  // The widget that captures the mouse

static bool first_time = true;    // true when we don't enter in poll yet

/* keyboard focus movement stuff */
static bool move_focus(Manager* manager, Message* msg);
static int count_widgets_accept_focus(Widget* widget);
static bool childs_accept_focus(Widget* widget, bool first);
static Widget* next_widget(Widget* widget);
static int cmp_left(Widget* widget, int x, int y);
static int cmp_right(Widget* widget, int x, int y);
static int cmp_up(Widget* widget, int x, int y);
static int cmp_down(Widget* widget, int x, int y);

Manager::Manager()
  : Widget(kManagerWidget)
  , m_display(NULL)
  , m_eventQueue(NULL)
  , m_lockedWindow(NULL)
  , m_mouseButtons(kButtonNone)
{
  if (!m_defaultManager) {
    // Empty lists
    ASSERT(msg_queue.empty());
    ASSERT(new_windows.empty());
    mouse_widgets_list.clear();

    // Reset variables
    focus_widget.reset();
    mouse_widget = NULL;
    capture_widget = NULL;
  }

  setBounds(gfx::Rect(0, 0, ui::display_w(), ui::display_h()));
  setVisible(true);

  m_dirtyRegion = bounds();

  // Default manager is the first one (and is always visible).
  if (!m_defaultManager)
    m_defaultManager = this;
}

Manager::~Manager()
{
  // There are some messages in queue? Dispatch everything.
  dispatchMessages();
  collectGarbage();

  // Finish the main manager.
  if (m_defaultManager == this) {
    // No more cursor
    set_mouse_cursor(kNoCursor);

    // Destroy filters
    for (auto& filters : msg_filters) {
      filters.clear();
    }

    // No more default manager
    m_defaultManager = nullptr;

    // Shutdown system
    ASSERT(msg_queue.empty());
    ASSERT(new_windows.empty());
    mouse_widgets_list.clear();
  }
}

void Manager::setDisplay(she::Display* display)
{
  m_display = display;
  m_eventQueue = she::instance()->eventQueue();

  onNewDisplayConfiguration();
}

void Manager::run()
{
  MessageLoop loop(this);

  if (first_time) {
    first_time = false;

    Manager::getDefault()->invalidate();
    set_mouse_cursor(kArrowCursor);
  }

  while (!children().empty())
    loop.pumpMessages();
}

void Manager::flipDisplay()
{
  if (!m_display)
    return;

  OverlayManager* overlays = OverlayManager::instance();

  update_cursor_overlay();

  // Draw overlays.
  overlays->captureOverlappedAreas();
  overlays->drawOverlays();

  // Flip dirty region.
  {
    m_dirtyRegion.createIntersection(
      m_dirtyRegion,
      gfx::Region(gfx::Rect(0, 0, ui::display_w(), ui::display_h())));

    for (auto& rc : m_dirtyRegion)
      m_display->flip(rc);

    m_dirtyRegion.clear();
  }

  overlays->restoreOverlappedAreas();
}

bool Manager::generateMessages()
{
  // First check: there are windows to manage?
  if (children().empty())
    return false;

  // New windows to show?
  if (!new_windows.empty()) {
    for (auto window : new_windows) {
      // Relayout
      window->layout();

      // Dirty the entire window and show it
      window->setVisible(true);
      window->invalidate();

      // Attract the focus to the magnetic widget...
      // 1) get the magnetic widget
      Widget* magnet = findMagneticWidget(window->window());
      // 2) if magnetic widget exists and it doesn't have the focus
      if (magnet)
        setFocus(magnet);
      // 3) if not, put the focus in the first child
      else if (static_cast<Window*>(window)->isWantFocus())
        focusFirstChild(window);
    }

    new_windows.clear();
  }

  generateMessagesFromSheEvents();

  // Generate messages for timers
  Timer::pollTimers();

  // Generate redraw events.
  flushRedraw();

  if (!msg_queue.empty())
    return true;
  else
    return false;
}

void Manager::generateSetCursorMessage(const gfx::Point& mousePos,
                                       KeyModifiers modifiers,
                                       PointerType pointerType)
{
  if (get_mouse_cursor() == kOutsideDisplay)
    return;

  Widget* dst = (capture_widget ? capture_widget: mouse_widget);
  if (dst)
    enqueueMessage(
      newMouseMessage(
        kSetCursorMessage, dst,
        mousePos,
        pointerType,
        _internal_get_mouse_buttons(),
        modifiers));
  else
    set_mouse_cursor(kArrowCursor);
}

static MouseButtons mouse_buttons_from_she_to_ui(const she::Event& sheEvent)
{
  switch (sheEvent.button()) {
    case she::Event::LeftButton:   return kButtonLeft; break;
    case she::Event::RightButton:  return kButtonRight; break;
    case she::Event::MiddleButton: return kButtonMiddle; break;
    default: return kButtonNone;
  }
}

void Manager::generateMessagesFromSheEvents() {
  she::Event lastMouseMoveEvent;
  she::Event sheEvent;
  for (;;) {
    bool canWait = false;

    m_eventQueue->getEvent(sheEvent, canWait);
    if (sheEvent.type() == she::Event::None)
      break;

    switch (sheEvent.type()) {
      case she::Event::CloseDisplay: {
        auto msg = std::make_shared<Message>(kCloseDisplayMessage);
        msg->broadcastToChildren(this);
        enqueueMessage(msg);
        break;
      }

      case she::Event::ResizeDisplay: {
        auto msg = std::make_shared<Message>(kResizeDisplayMessage);
        msg->broadcastToChildren(this);
        enqueueMessage(msg);
        break;
      }

      case she::Event::DropFiles: {
        auto msg = std::make_shared<DropFilesMessage>(sheEvent.files());
        msg->addRecipient(this);
        enqueueMessage(msg);
        break;
      }

      case she::Event::KeyDown:
      case she::Event::KeyUp: {
        auto keymsg = std::make_shared<KeyMessage>(
          (sheEvent.type() == she::Event::KeyDown ? kKeyDownMessage : kKeyUpMessage),
          sheEvent.scancode(),
          sheEvent.modifiers(),
          sheEvent.unicodeChar(),
          sheEvent.repeat());
        auto msg = std::static_pointer_cast<Message>(keymsg);
        broadcastKeyMsg(msg);
        enqueueMessage(msg);
        break;
      }

      case she::Event::MouseEnter: {
        _internal_set_mouse_position(sheEvent.position());
        set_mouse_cursor(kArrowCursor);
        lastMouseMoveEvent = sheEvent;
        break;
      }

      case she::Event::MouseLeave: {
        set_mouse_cursor(kOutsideDisplay);
        setMouse(NULL);

        _internal_no_mouse_position();

        // To avoid calling kSetCursorMessage when the mouse leaves
        // the window.
        lastMouseMoveEvent = she::Event();
        break;
      }

      case she::Event::MouseMove: {
        _internal_set_mouse_position(sheEvent.position());
        handleMouseMove(
          sheEvent.position(),
          m_mouseButtons,
          sheEvent.modifiers(),
          sheEvent.pointerType());
        lastMouseMoveEvent = sheEvent;
        break;
      }

      case she::Event::MouseDown: {
        MouseButtons pressedButton = mouse_buttons_from_she_to_ui(sheEvent);
        m_mouseButtons = (MouseButtons)((int)m_mouseButtons | (int)pressedButton);
        _internal_set_mouse_buttons(m_mouseButtons);

        handleMouseDown(
          sheEvent.position(),
          pressedButton,
          sheEvent.modifiers(),
          sheEvent.pointerType());
        break;
      }

      case she::Event::MouseUp: {
        MouseButtons releasedButton = mouse_buttons_from_she_to_ui(sheEvent);
        m_mouseButtons = (MouseButtons)((int)m_mouseButtons & ~(int)releasedButton);
        _internal_set_mouse_buttons(m_mouseButtons);

        handleMouseUp(
          sheEvent.position(),
          releasedButton,
          sheEvent.modifiers(),
          sheEvent.pointerType());
        break;
      }

      case she::Event::MouseDoubleClick: {
        MouseButtons clickedButton = mouse_buttons_from_she_to_ui(sheEvent);
        handleMouseDoubleClick(
          sheEvent.position(),
          clickedButton,
          sheEvent.modifiers(),
          sheEvent.pointerType());
        break;
      }

      case she::Event::MouseWheel: {
        handleMouseWheel(sheEvent.position(), m_mouseButtons,
                         sheEvent.modifiers(),
                         sheEvent.pointerType(),
                         sheEvent.wheelDelta(),
                         sheEvent.preciseWheel());
        break;
      }

      case she::Event::TouchMagnify: {
        _internal_set_mouse_position(sheEvent.position());

        handleTouchMagnify(sheEvent.position(),
                           sheEvent.modifiers(),
                           sheEvent.magnification());
        break;
      }

    }
  }

  // Generate just one kSetCursorMessage for the last mouse position
  if (lastMouseMoveEvent.type() != she::Event::None) {
    sheEvent = lastMouseMoveEvent;
    generateSetCursorMessage(sheEvent.position(),
                             sheEvent.modifiers(),
                             sheEvent.pointerType());
  }
}

void Manager::handleMouseMove(const gfx::Point& mousePos,
                              MouseButtons mouseButtons,
                              KeyModifiers modifiers,
                              PointerType pointerType)
{
  // Get the list of widgets to send mouse messages.
  mouse_widgets_list.clear();
  broadcastMouseMessage(mouse_widgets_list);

  // Get the widget under the mouse
  Widget* widget = nullptr;
  for (auto mouseWidget : mouse_widgets_list) {
    widget = mouseWidget->pick(mousePos);
    if (widget)
      break;
  }

  // Fixup "mouse" flag
  if (widget != mouse_widget) {
    if (!widget)
      freeMouse();
    else
      setMouse(widget);
  }

  // Send the mouse movement message
  Widget* dst = (capture_widget ? capture_widget: mouse_widget);
  enqueueMessage(
    newMouseMessage(
      kMouseMoveMessage, dst,
      mousePos,
      pointerType,
      mouseButtons,
      modifiers));
}

void Manager::handleMouseDown(const gfx::Point& mousePos,
                              MouseButtons mouseButtons,
                              KeyModifiers modifiers,
                              PointerType pointerType)
{
  handleWindowZOrder();

  enqueueMessage(
    newMouseMessage(
      kMouseDownMessage,
      (capture_widget ? capture_widget: mouse_widget),
      mousePos,
      pointerType,
      mouseButtons,
      modifiers));
}

void Manager::handleMouseUp(const gfx::Point& mousePos,
                            MouseButtons mouseButtons,
                            KeyModifiers modifiers,
                            PointerType pointerType)
{
  enqueueMessage(
    newMouseMessage(
      kMouseUpMessage,
      (capture_widget ? capture_widget: mouse_widget),
      mousePos,
      pointerType,
      mouseButtons,
      modifiers));
}

void Manager::handleMouseDoubleClick(const gfx::Point& mousePos,
                                     MouseButtons mouseButtons,
                                     KeyModifiers modifiers,
                                     PointerType pointerType)
{
  Widget* dst = (capture_widget ? capture_widget: mouse_widget);
  if (dst) {
    enqueueMessage(
      newMouseMessage(
        kDoubleClickMessage,
        dst, mousePos, pointerType,
        mouseButtons, modifiers));
  }
}

void Manager::handleMouseWheel(const gfx::Point& mousePos,
                               MouseButtons mouseButtons,
                               KeyModifiers modifiers,
                               PointerType pointerType,
                               const gfx::Point& wheelDelta,
                               bool preciseWheel)
{
  enqueueMessage(newMouseMessage(
      kMouseWheelMessage,
      capture_widget ?: mouse_widget,
      mousePos, pointerType, mouseButtons, modifiers,
      wheelDelta, preciseWheel));
}

void Manager::handleTouchMagnify(const gfx::Point& mousePos,
                                 const KeyModifiers modifiers,
                                 const double magnification)
{
  Widget* widget = capture_widget ?: mouse_widget;
  if (!widget)
    return;
  auto msg = std::make_shared<TouchMessage>(
    kTouchMagnifyMessage,
    modifiers,
    mousePos,
    magnification);
  msg->addRecipient(widget);
  enqueueMessage(msg);
}

// Handles Z order: Send the window to top (only when you click in a
// window that aren't the desktop).
void Manager::handleWindowZOrder()
{
  if (capture_widget || !mouse_widget)
    return;

  // The clicked window
  Window* window = mouse_widget->window();
  Manager* win_manager = window ? window->manager() : nullptr;

  if ((window) &&
    // We cannot change Z-order of desktop windows
    (!window->isDesktop()) &&
    // We cannot change Z order of foreground windows because a
    // foreground window can launch other background windows
    // which should be kept on top of the foreground one.
    (!window->isForeground()) &&
    // If the window is not already the top window of the manager.
    (window != win_manager->getTopWindow())) {
    base::ScopedValue<Widget*> scoped(m_lockedWindow, window, NULL);

    // Put it in the top of the list
    win_manager->removeChild(window);

    if (window->isOnTop())
      win_manager->insertChild(0, window);
    else {
      int pos = (int)win_manager->children().size();
      for (auto child : base::reverse(win_manager->children())) {
        if (static_cast<Window*>(child)->isOnTop())
          break;
        --pos;
      }
      win_manager->insertChild(pos, window);
    }

    window->invalidate();
  }

  // Put the focus
  setFocus(mouse_widget);
}

void Manager::dispatchMessages()
{
  pumpQueue();
  flipDisplay();
}

void Manager::addToGarbage(Widget* widget)
{
  m_garbage.push_back(widget);
}

/**
 * @param msg You can't use the this message after calling this
 *            routine. The message will be automatically freed through
 *            @ref jmessage_free
 */
void Manager::enqueueMessage(std::shared_ptr<Message> msg) {
  // Check if this message must be filtered by some widget before
  int c = msg->type() % msg_filters.size();
  for (auto& filter : base::reverse(msg_filters[c])) {
    if (filter.widget && msg->type() == filter.message) {
      msg->prependRecipient(filter.widget);
    }
  }

  if (msg->hasRecipients())
    msg_queue.push_back(msg);
}

Window* Manager::getTopWindow()
{
  return static_cast<Window*>(firstChild());
}

Window* Manager::getForegroundWindow()
{
  for (auto child : children()) {
    Window* window = static_cast<Window*>(child);
    if (window->isForeground() ||
        window->isDesktop())
      return window;
  }
  return nullptr;
}

Widget* Manager::getFocus()
{
  return focus_widget;
}

Widget* Manager::getMouse()
{
  return mouse_widget;
}

Widget* Manager::getCapture()
{
  return capture_widget;
}

void Manager::setFocus(Widget* widget)
{
  if (focus_widget == widget)
    return;

  if (!widget ||
      (!widget->hasFlags(DISABLED) &&
       !widget->hasFlags(HIDDEN) &&
       !widget->hasFlags(DECORATIVE) &&
        someParentIsFocusStop(widget))) {
    WidgetsList widget_parents;
    Widget* common_parent = NULL;

    if (widget)
      widget->getParents(false, widget_parents);

    // Fetch the focus
    if (focus_widget) {
      WidgetsList focus_parents;
      focus_widget->getParents(true, focus_parents);

      auto msg = std::make_shared<Message>(kFocusLeaveMessage);

      for (Widget* parent1 : focus_parents) {
        if (widget) {
          for (Widget* parent2 : widget_parents) {
            if (parent1 == parent2) {
              common_parent = parent1;
              break;
            }
          }
          if (common_parent)
            break;
        }

        if (parent1->hasFocus()) {
          parent1->disableFlags(HAS_FOCUS);
          msg->addRecipient(parent1);
        }
      }

      enqueueMessage(msg);
      focus_widget.reset();
    }


    // Put the focus
    if (widget) {
      focus_widget = widget->safePtr;

      auto it = widget_parents.begin();
      if (common_parent) {
        it = std::find(widget_parents.begin(),
                       widget_parents.end(),
                       common_parent);
        ASSERT(it != widget_parents.end());
        ++it;
      }

      auto msg = std::make_shared<Message>(kFocusEnterMessage);

      for (; it != widget_parents.end(); ++it) {
        auto w = *it;
        if (w->hasFlags(FOCUS_STOP)) {
          w->enableFlags(HAS_FOCUS);
          msg->addRecipient(w);
        }
      }

      enqueueMessage(msg);
    }
  }
}

void Manager::setMouse(Widget* widget)
{
#ifdef REPORT_EVENTS
  std::cout << "Manager::setMouse ";
  if (widget) {
    std::cout << typeid(*widget).name();
    if (!widget->id().empty())
      std::cout << " (" << widget->id() << ")";
  }
  else {
    std::cout << "null";
  }
  std::cout << std::endl;
#endif

  if ((mouse_widget != widget) && (!capture_widget)) {
    WidgetsList widget_parents;
    Widget* common_parent = NULL;

    if (widget)
      widget->getParents(false, widget_parents);

    // Fetch the mouse
    if (mouse_widget) {
      WidgetsList mouse_parents;
      mouse_widget->getParents(true, mouse_parents);

      auto msg = std::make_shared<Message>(kMouseLeaveMessage);

      for (Widget* parent1 : mouse_parents) {
        if (widget) {
          for (Widget* parent2 : widget_parents) {
            if (parent1 == parent2) {
              common_parent = parent1;
              break;
            }
          }
          if (common_parent)
            break;
        }

        if (parent1->hasMouse()) {
          parent1->disableFlags(HAS_MOUSE);
          msg->addRecipient(parent1);
        }
      }

      enqueueMessage(msg);
    }

    // Put the mouse
    mouse_widget = widget;
    if (widget) {
      WidgetsList::iterator it;

      if (common_parent) {
        it = std::find(widget_parents.begin(),
                       widget_parents.end(),
                       common_parent);
        ASSERT(it != widget_parents.end());
        ++it;
      }
      else
        it = widget_parents.begin();

      auto msg = newMouseMessage(
        kMouseEnterMessage, NULL,
        get_mouse_position(),
        PointerType::Unknown,
        _internal_get_mouse_buttons(),
        kKeyUninitializedModifier);

      for (; it != widget_parents.end(); ++it) {
        (*it)->enableFlags(HAS_MOUSE);
        msg->addRecipient(*it);
      }

      enqueueMessage(msg);
      generateSetCursorMessage(get_mouse_position(),
                               kKeyUninitializedModifier,
                               PointerType::Unknown);
    }
  }
}

void Manager::setCapture(Widget* widget)
{
  widget->enableFlags(HAS_CAPTURE);
  capture_widget = widget;

  m_display->captureMouse();
}

// Sets the focus to the "magnetic" widget inside the window
void Manager::attractFocus(Widget* widget)
{
  // Get the magnetic widget
  Widget* magnet = findMagneticWidget(widget->window());

  // If magnetic widget exists and it doesn't have the focus
  if (magnet)
    setFocus(magnet);
}

void Manager::focusFirstChild(Widget* widget)
{
  for (Widget* it=widget->window(); it; it=next_widget(it)) {
    if (ACCEPT_FOCUS(it) && !(childs_accept_focus(it, true))) {
      setFocus(it);
      break;
    }
  }
}

void Manager::freeFocus()
{
  setFocus(nullptr);
}

void Manager::freeMouse()
{
  setMouse(nullptr);
}

void Manager::freeCapture()
{
  if (capture_widget) {
    capture_widget->disableFlags(HAS_CAPTURE);
    capture_widget = NULL;

    m_display->releaseMouse();
  }
}

void Manager::freeWidget(Widget* widget)
{
  if (widget->hasFocus() || (focus_widget == widget))
    freeFocus();

  // We shouldn't free widgets that are locked, it means, widgets that
  // will be re-added soon (e.g. when the stack of windows is
  // temporarily modified).
  if (m_lockedWindow == widget)
    return;

  // Break any relationship with the GUI manager
  if (widget->hasCapture() || (widget == capture_widget))
    freeCapture();

  if (widget->hasMouse() || (widget == mouse_widget))
    freeMouse();
}

void Manager::removeMessagesFor(Widget* widget, MessageType type)
{
  for (auto msg : msg_queue)
    if (msg->type() == type)
      msg->removeRecipient(widget);
}

void Manager::addMessageFilter(int message, Widget* widget)
{
  int c = message % msg_filters.size();
  auto& filters = msg_filters[c];
  filters.emplace_back(message, widget);
  for (auto it = filters.begin(); it != filters.end();) {
    if (!it->widget) {
      it = filters.erase(it);
    } else {
      ++it;
    }
  }
}

void Manager::removeMessageFilter(int message, Widget* widget)
{
  int c = message % msg_filters.size();
  auto& filters = msg_filters[c];
  for (auto it=filters.begin(); it != filters.end(); ) {
    auto& filter = *it;
    if (!filter.widget || filter.widget == widget) {
      it = filters.erase(it);
    } else {
      ++it;
    }
  }
}

bool Manager::isFocusMovementKey(Message* msg)
{
  switch (static_cast<KeyMessage*>(msg)->scancode()) {

    case kKeyTab:
    case kKeyLeft:
    case kKeyRight:
    case kKeyUp:
    case kKeyDown:
      return true;
  }
  return false;
}

void Manager::dirtyRect(const gfx::Rect& bounds)
{
  m_dirtyRegion.createUnion(m_dirtyRegion, gfx::Region(bounds));
}

/* configures the window for begin the loop */
void Manager::_openWindow(Window* window)
{
  // Free all widgets of special states.
  if (window->isWantFocus()) {
    freeCapture();
    freeMouse();
    freeFocus();
  }

  // Add the window to manager.
  insertChild(0, window);

  // Broadcast the open message.
  auto msg = std::make_shared<Message>(kOpenMessage);
  msg->addRecipient(window);
  enqueueMessage(msg);

  // Update the new windows list to show.
  new_windows.push_back(window);
}

void Manager::_closeWindow(Window* window, bool redraw_background)
{
  if (!hasChild(window))
    return;

  gfx::Region reg1;
  if (redraw_background)
    window->getRegion(reg1);

  // Close all windows to this desktop
  if (window->isDesktop()) {
    while (!children().empty()) {
      Window* child = static_cast<Window*>(children().front());
      if (child == window)
        break;
      else {
        gfx::Region reg2;
        window->getRegion(reg2);
        reg1.createUnion(reg1, reg2);

        _closeWindow(child, false);
      }
    }
  }

  // Free all widgets of special states.
  if (capture_widget && capture_widget->window() == window)
    freeCapture();

  if (mouse_widget && mouse_widget->window() == window)
    freeMouse();

  if (focus_widget && focus_widget->window() == window)
    freeFocus();

  // Hide window.
  window->setVisible(false);

  // Close message.
  auto msg = std::make_shared<Message>(kCloseMessage);
  msg->addRecipient(window);
  enqueueMessage(msg);

  // Update manager list stuff.
  removeChild(window);

  // Redraw background.
  invalidateRegion(reg1);

  // Maybe the window is in the "new_windows" list.
  WidgetsList::iterator it =
    std::find(new_windows.begin(), new_windows.end(), window);
  if (it != new_windows.end())
    new_windows.erase(it);

  // Update mouse widget (as it can be a widget below the
  // recently closed window).
  Widget* widget = pick(ui::get_mouse_position());
  if (widget)
    setMouse(widget);
}

bool Manager::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kResizeDisplayMessage:
      onNewDisplayConfiguration();
      break;

    case kKeyDownMessage:
    case kKeyUpMessage: {
      KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
      keymsg->setPropagateToChildren(true);
      keymsg->setPropagateToParent(false);

      // Continue sending the message to the children of all windows
      // (until a desktop or foreground window).
      Window* win = nullptr;
      for (auto manchild : children()) {
        win = static_cast<Window*>(manchild);

        // Send to the window.
        for (auto winchild : win->children())
          if (winchild->sendMessage(msg))
            return true;

        if (win->isForeground() ||
            win->isDesktop())
          break;
      }

      // Check the focus movement for foreground (non-desktop) windows.
      if (win && win->isForeground()) {
        if (msg->type() == kKeyDownMessage)
          move_focus(this, msg);
        return true;
      }
      else
        return false;
    }

  }

  return Widget::onProcessMessage(msg);
}

void Manager::onResize(ResizeEvent& ev)
{
  gfx::Rect old_pos = bounds();
  gfx::Rect new_pos = ev.bounds();
  setBoundsQuietly(new_pos);

  // The whole manager area is invalid now.
  m_invalidRegion = gfx::Region(new_pos);

  int dx = new_pos.x - old_pos.x;
  int dy = new_pos.y - old_pos.y;
  int dw = new_pos.w - old_pos.w;
  int dh = new_pos.h - old_pos.h;

  for (auto child : children()) {
    Window* window = static_cast<Window*>(child);
    if (window->isDesktop()) {
      window->setBounds(new_pos);
      break;
    }

    gfx::Rect cpos = window->bounds();
    int cx = cpos.x+cpos.w/2;
    int cy = cpos.y+cpos.h/2;

    if (cx > old_pos.x+old_pos.w*3/5) {
      cpos.x += dw;
    }
    else if (cx > old_pos.x+old_pos.w*2/5) {
      cpos.x += dw / 2;
    }

    if (cy > old_pos.y+old_pos.h*3/5) {
      cpos.y += dh;
    }
    else if (cy > old_pos.y+old_pos.h*2/5) {
      cpos.y += dh / 2;
    }

    cpos.offset(dx, dy);
    window->setBounds(cpos);
  }
}

void Manager::onPaint(PaintEvent& ev)
{
  theme()->paintDesktop(ev);
}

void Manager::onBroadcastMouseMessage(WidgetsList& targets)
{
  // Ask to the first window in the "children" list to know how to
  // propagate mouse messages.
  if (Widget* widget = firstChild())
    widget->broadcastMouseMessage(targets);
}

LayoutIO* Manager::onGetLayoutIO()
{
  return NULL;
}

void Manager::onNewDisplayConfiguration()
{
  if (m_display) {
    int w = m_display->width() / m_display->scale();
    int h = m_display->height() / m_display->scale();
    if ((bounds().w != w ||
         bounds().h != h)) {
      setBounds(gfx::Rect(0, 0, w, h));
    }
  }

  _internal_set_mouse_display(m_display);
  invalidate();
  flushRedraw();
}

void Manager::onSizeHint(SizeHintEvent& ev)
{
  int w = 0, h = 0;

  if (!parent()) {        // hasn' parent?
    w = bounds().w;
    h = bounds().h;
  }
  else {
    gfx::Rect pos = parent()->childrenBounds();

    for (auto child : children()) {
      gfx::Rect cpos = child->bounds();
      pos = pos.createUnion(cpos);
    }

    w = pos.w;
    h = pos.h;
  }

  ev.setSizeHint(gfx::Size(w, h));
}

void Manager::pumpQueue()
{
#ifdef LIMIT_DISPATCH_TIME
  base::tick_t t = base::current_tick();
#endif

  while (!msg_queue.empty()) {
#ifdef LIMIT_DISPATCH_TIME
    if (base::current_tick()-t > 250)
      break;
#endif
    auto msg = msg_queue.front();
    msg_queue.pop_front();
    msg->send();
  }
}

void Manager::invalidateDisplayRegion(const gfx::Region& region)
{
  // TODO intersect with getDrawableRegion()???
  gfx::Region reg1;
  reg1.createIntersection(region, gfx::Region(bounds()));

  // Redraw windows from top to background.
  bool withDesktop = false;
  for (auto child : children()) {
    ASSERT(dynamic_cast<Window*>(child));
    ASSERT(child->type() == kWindowWidget);
    Window* window = static_cast<Window*>(child);

    // Invalidate regions of this window
    window->invalidateRegion(reg1);

    // There is desktop?
    if (window->isDesktop()) {
      withDesktop = true;
      break;                                    // Work done
    }

    // Clip this window area for the next window.
    gfx::Region reg3;
    window->getRegion(reg3);
    reg1.createSubtraction(reg1, reg3);
  }

  // Invalidate areas outside windows (only when there are not a
  // desktop window).
  if (!withDesktop)
    Widget::invalidateRegion(reg1);
}

LayoutIO* Manager::getLayoutIO()
{
  return onGetLayoutIO();
}

void Manager::collectGarbage()
{
  if (m_garbage.empty())
    return;

  for (WidgetsList::iterator
         it = m_garbage.begin(),
         end = m_garbage.end(); it != end; ++it) {
    delete *it;
  }
  m_garbage.clear();
}

/**********************************************************************
                        Internal routines
 **********************************************************************/

// static
bool Manager::someParentIsFocusStop(Widget* widget)
{
  if (widget->isFocusStop())
    return true;

  if (widget->parent())
    return someParentIsFocusStop(widget->parent());
  else
    return false;
}

// static
Widget* Manager::findMagneticWidget(Widget* widget)
{
  Widget* found;

  for (auto child : widget->children()) {
    found = findMagneticWidget(child);
    if (found)
      return found;
  }

  if (widget->isFocusMagnet())
    return widget;
  else
    return NULL;
}

// static
std::shared_ptr<Message> Manager::newMouseMessage(
  MessageType type,
  Widget* widget,
  const gfx::Point& mousePos,
  PointerType pointerType,
  MouseButtons buttons,
  KeyModifiers modifiers,
  const gfx::Point& wheelDelta,
  bool preciseWheel)
{
#ifdef __APPLE__
  // Convert Ctrl+left click -> right-click
  if (widget &&
      widget->isVisible() &&
      widget->isEnabled() &&
      widget->hasFlags(CTRL_RIGHT_CLICK) &&
      (modifiers & kKeyCtrlModifier) &&
      (buttons == kButtonLeft)) {
    modifiers = KeyModifiers(int(modifiers) & ~int(kKeyCtrlModifier));
    buttons = kButtonRight;
  }
#endif

  auto msg = std::make_shared<MouseMessage>(type, pointerType, buttons, modifiers, mousePos, wheelDelta, preciseWheel);

  if (widget)
    msg->addRecipient(widget);

  return msg;
}

// static
void Manager::broadcastKeyMsg(std::shared_ptr<Message> msg)
{
  // Send the message to the widget with capture
  if (capture_widget) {
    msg->addRecipient(capture_widget);
  }
  // Send the msg to the focused widget
  else if (focus_widget) {
    msg->addRecipient(focus_widget);
  }
  // Finally, send the message to the manager, it'll know what to do
  else {
    msg->addRecipient(this);
  }
}

/***********************************************************************
                            Focus Movement
 ***********************************************************************/

static bool move_focus(Manager* manager, Message* msg)
{
  int (*cmp)(Widget*, int, int) = NULL;
  Widget* focus = NULL;
  Widget* it;
  bool ret = false;
  Window* window = NULL;
  int c, count;

  // Who have the focus
  if (focus_widget) {
    window = focus_widget->window();
  }
  else if (!manager->children().empty()) {
    window = manager->getTopWindow();
  }

  if (!window)
    return false;

  // How many child-widget want the focus in this widget?
  count = count_widgets_accept_focus(window);

  // One at least
  if (count > 0) {
    std::vector<Widget*> list(count);

    c = 0;

    /* list's 1st element is the focused widget */
    for (it=focus_widget; it; it=next_widget(it)) {
      if (ACCEPT_FOCUS(it) && !(childs_accept_focus(it, true)))
        list[c++] = it;
    }

    for (it = window; focus_widget != it; it = next_widget(it)) {
      if (ACCEPT_FOCUS(it) && !(childs_accept_focus(it, true)))
        list[c++] = it;
    }

    // Depending on the pressed key...
    switch (static_cast<KeyMessage*>(msg)->scancode()) {

      case kKeyTab:
        // Reverse tab
        if ((msg->modifiers() & (kKeyShiftModifier | kKeyCtrlModifier | kKeyAltModifier)) != 0) {
          focus = list[count-1];
        }
        // Normal tab
        else if (count > 1) {
          focus = list[1];
        }
        ret = true;
        break;

      // Arrow keys
      case kKeyLeft:  if (!cmp) cmp = cmp_left;
      case kKeyRight: if (!cmp) cmp = cmp_right;
      case kKeyUp:    if (!cmp) cmp = cmp_up;
      case kKeyDown:  if (!cmp) cmp = cmp_down;

        // More than one widget
        if (count > 1) {
          int i, j, x, y;

          // Position where the focus come
          x = ((focus_widget) ? focus_widget->bounds().x+focus_widget->bounds().x2():
                                window->bounds().x+window->bounds().x2())
            / 2;
          y = ((focus_widget) ? focus_widget->bounds().y+focus_widget->bounds().y2():
                                window->bounds().y+window->bounds().y2())
            / 2;

          c = focus_widget ? 1: 0;

          // Rearrange the list
          for (i=c; i<count-1; i++) {
            for (j=i+1; j<count; j++) {
              // Sort the list in ascending order
              if ((*cmp) (list[i], x, y) > (*cmp) (list[j], x, y)) {
                Widget* tmp = list[i];
                list[i] = list[j];
                list[j] = tmp;
              }
            }
          }

          // Check if the new widget to put the focus is not in the wrong way.
          if ((*cmp) (list[c], x, y) < std::numeric_limits<int>::max())
            focus = list[c];
        }
        // If only there are one widget, put the focus in this
        else
          focus = list[0];

        ret = true;
        break;
    }

    if (focus)
      Manager::getDefault()->setFocus(focus);
  }

  return ret;
}

static int count_widgets_accept_focus(Widget* widget)
{
  ASSERT(widget != NULL);

  int count = 0;

  for (auto child : widget->children())
    count += count_widgets_accept_focus(child);

  if ((count == 0) && (ACCEPT_FOCUS(widget)))
    count++;

  return count;
}

static bool childs_accept_focus(Widget* widget, bool first)
{
  for (auto child : widget->children())
    if (childs_accept_focus(child, false))
      return true;

  return (first ? false: ACCEPT_FOCUS(widget));
}

static Widget* next_widget(Widget* widget)
{
  if (!widget->children().empty())
    return widget->firstChild();

  while (widget->parent()->type() != kManagerWidget) {
    WidgetsList::const_iterator begin = widget->parent()->children().begin();
    WidgetsList::const_iterator end = widget->parent()->children().end();
    WidgetsList::const_iterator it = std::find(begin, end, widget);

    ASSERT(it != end);

    if ((it+1) != end)
      return *(it+1);
    else
      widget = widget->parent();
  }

  return NULL;
}

static int cmp_left(Widget* widget, int x, int y)
{
  int z = x - (widget->bounds().x+widget->bounds().w/2);
  if (z <= 0)
    return std::numeric_limits<int>::max();
  return z + ABS((widget->bounds().y+widget->bounds().h/2) - y) * 8;
}

static int cmp_right(Widget* widget, int x, int y)
{
  int z = (widget->bounds().x+widget->bounds().w/2) - x;
  if (z <= 0)
    return std::numeric_limits<int>::max();
  return z + ABS((widget->bounds().y+widget->bounds().h/2) - y) * 8;
}

static int cmp_up(Widget* widget, int x, int y)
{
  int z = y - (widget->bounds().y+widget->bounds().h/2);
  if (z <= 0)
    return std::numeric_limits<int>::max();
  return z + ABS((widget->bounds().x+widget->bounds().w/2) - x) * 8;
}

static int cmp_down(Widget* widget, int x, int y)
{
  int z = (widget->bounds().y+widget->bounds().h/2) - y;
  if (z <= 0)
    return std::numeric_limits<int>::max();
  return z + ABS((widget->bounds().x+widget->bounds().w/2) - x) * 8;
}

} // namespace ui

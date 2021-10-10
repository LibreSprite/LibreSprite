// Aseprite    | Copyright (C) 2001-2016  David Capello
// LibreSprite | Copyright (C)      2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

// #define REPORT_SIGNALS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/widget.h"

#include "base/memory.h"
#include "base/string.h"
#include "she/display.h"
#include "she/font.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/init_theme_event.h"
#include "ui/intern.h"
#include "ui/layout_io.h"
#include "ui/load_layout_event.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/move_region.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/resize_event.h"
#include "ui/save_layout_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/window.h"

#include <cctype>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <queue>
#include <sstream>

static base::weak_set<ui::Widget> widgets;

namespace ui {

using namespace gfx;

static inline void mark_dirty_flag(Widget* widget)
{
  while (widget) {
    widget->enableFlags(DIRTY);
    widget = widget->parent();
  }
}

WidgetType register_widget_type()
{
  static int type = (int)kFirstUserWidget;
  return (WidgetType)type++;
}

Widget::Widget(WidgetType type) : m_type(type),
                                  m_theme(CurrentTheme::get()) {
  getAll().insert(safePtr); // to-do: use postInject below instead
}

void Widget::postInject() {
  m_wasInjected = true;
//   getAll().insert(shared_from_this());
}

Widget::~Widget()
{
  // Break relationship with the manager.
  if (this->type() != kManagerWidget)
    manager()->freeWidget(this);

  // Remove from parent
  if (m_parent)
    m_parent->removeChild(this);

  while (!m_ownedChildren.empty())
    removeChild(m_ownedChildren.back().get());

  // Remove children. The ~Widget dtor modifies the parent's
  // m_children.
  while (!m_children.empty())
    delete m_children.front();
}

void Widget::deferDelete() {
  manager()->addToGarbage(this);
}

base::weak_set<Widget>& Widget::getAll() {
  return widgets;
}

PropertyPtr Widget::getProperty(const std::string& name) const
{
  auto it = m_properties.find(name);
  if (it != m_properties.end())
    return it->second;
  else
    return PropertyPtr();
}

void Widget::setProperty(PropertyPtr property)
{
  m_properties[property->getName()] = property;
}

const Widget::Properties& Widget::getProperties() const
{
  return m_properties;
}

void Widget::initTheme()
{
  InitThemeEvent ev(this, m_theme);
  onInitTheme(ev);
}

int Widget::textInt() const
{
  return strtol(m_text.c_str(), NULL, 10);
}

double Widget::textDouble() const
{
  return strtod(m_text.c_str(), NULL);
}

int Widget::textLength() const
{
  return base::utf8_length(text());
}

void Widget::setText(const std::string& text)
{
  setTextQuiet(text);
  onSetText();
}

void Widget::setTextf(const char *format, ...)
{
  // formatted string
  if (format) {
    va_list ap;
    va_start(ap, format);
    char buf[4096];
    vsprintf(buf, format, ap);
    va_end(ap);

    setText(buf);
  }
  // empty string
  else {
    setText("");
  }
}

void Widget::setTextQuiet(const std::string& text)
{
  m_text = text;
  enableFlags(HAS_TEXT);
}

she::Font* Widget::font() const
{
  if (!m_font) {
    ASSERT(m_theme);
    m_font = m_theme->getWidgetFont(this);
  }
  return m_font;
}

void Widget::resetFont(she::Font* font)
{
  m_font = font;
}

void Widget::setBgColor(gfx::Color color)
{
  m_bgColor = color;
  onSetBgColor();
}

void Widget::setTheme(Theme* theme)
{
  m_theme = theme;
  m_font = nullptr;
}

// ===============================================================
// COMMON PROPERTIES
// ===============================================================

void Widget::setVisible(bool state)
{
  if (state) {
    if (hasFlags(HIDDEN)) {
      disableFlags(HIDDEN);
      invalidate();
    }
  }
  else {
    if (!hasFlags(HIDDEN)) {
      manager()->freeWidget(this); // Free from manager
      enableFlags(HIDDEN);
    }
  }
}

void Widget::setEnabled(bool state)
{
  if (state) {
    if (hasFlags(DISABLED)) {
      disableFlags(DISABLED);
      invalidate();

      onEnable();
    }
  }
  else {
    if (!hasFlags(DISABLED)) {
      manager()->freeWidget(this); // Free from the manager

      enableFlags(DISABLED);
      invalidate();

      onDisable();
    }
  }
}

void Widget::setSelected(bool state)
{
  if (state) {
    if (!hasFlags(SELECTED)) {
      enableFlags(SELECTED);
      invalidate();

      onSelect();
    }
  }
  else {
    if (hasFlags(SELECTED)) {
      disableFlags(SELECTED);
      invalidate();

      onDeselect();
    }
  }
}

void Widget::setExpansive(bool state)
{
  if (state)
    enableFlags(EXPANSIVE);
  else
    disableFlags(EXPANSIVE);
}

void Widget::setDecorative(bool state)
{
  if (state)
    enableFlags(DECORATIVE);
  else
    disableFlags(DECORATIVE);
}

void Widget::setFocusStop(bool state)
{
  if (state)
    enableFlags(FOCUS_STOP);
  else
    disableFlags(FOCUS_STOP);
}

void Widget::setFocusMagnet(bool state)
{
  if (state)
    enableFlags(FOCUS_MAGNET);
  else
    disableFlags(FOCUS_MAGNET);
}

bool Widget::isVisible() const
{
  const Widget* widget = this;
  const Widget* lastWidget = nullptr;

  do {
    if (widget->hasFlags(HIDDEN))
      return false;

    lastWidget = widget;
    widget = widget->m_parent;
  } while (widget);

  // The widget is visible if it's inside a visible manager
  return (lastWidget ? lastWidget->type() == kManagerWidget: false);
}

bool Widget::isEnabled() const
{
  const Widget* widget = this;

  do {
    if (widget->hasFlags(DISABLED))
      return false;

    widget = widget->m_parent;
  } while (widget);

  return true;
}

bool Widget::isSelected() const
{
  return hasFlags(SELECTED);
}

bool Widget::isExpansive() const
{
  return hasFlags(EXPANSIVE);
}

bool Widget::isDecorative() const
{
  return hasFlags(DECORATIVE);
}

bool Widget::isFocusStop() const
{
  return hasFlags(FOCUS_STOP);
}

bool Widget::isFocusMagnet() const
{
  return hasFlags(FOCUS_MAGNET);
}

// ===============================================================
// PARENTS & CHILDREN
// ===============================================================

Window* Widget::window() {
  Widget* widget = this;

  while (widget) {
    if (widget->type() == kWindowWidget)
      return static_cast<Window*>(widget);

    widget = widget->m_parent;
  }

  return NULL;
}

Manager* Widget::manager() {
  return m_manager ?: Manager::getDefault();
}

void Widget::setManager(Manager* manager) {
  if (manager == m_manager)
    return;
  m_manager = manager;
  for (auto child : children()) {
    child->setManager(manager);
  }
}

void Widget::getParents(bool ascendant, WidgetsList& parents)
{
  for (Widget* widget=this; widget; widget=widget->m_parent) {
    // append parents in tail
    if (ascendant)
      parents.push_back(widget);
    // append parents in head
    else
      parents.insert(parents.begin(), widget);
  }
}

Widget* Widget::nextSibling()
{
  if (!m_parent)
    return NULL;

  WidgetsList::iterator begin = m_parent->m_children.begin();
  WidgetsList::iterator end = m_parent->m_children.end();
  WidgetsList::iterator it = std::find(begin, end, this);

  if (it == end)
    return NULL;

  if (++it == end)
    return NULL;

  return *it;
}

Widget* Widget::previousSibling()
{
  if (!m_parent)
    return NULL;

  WidgetsList::iterator begin = m_parent->m_children.begin();
  WidgetsList::iterator end = m_parent->m_children.end();
  WidgetsList::iterator it = std::find(begin, end, this);

  if (it == begin || it == end)
    return NULL;

  return *(++it);
}

Widget* Widget::pick(const gfx::Point& pt, bool checkParentsVisibility)
{
  Widget* inside, *picked = nullptr;

  // isVisible() checks visibility of widget's parent.
  if (((checkParentsVisibility && isVisible()) ||
       (!checkParentsVisibility && !hasFlags(HIDDEN))) &&
      (bounds().contains(pt))) {
    picked = this;

    for (Widget* child : m_children) {
      inside = child->pick(pt, false);
      if (inside) {
        picked = inside;
        break;
      }
    }
  }

  return picked;
}

bool Widget::hasChild(Widget* child)
{
  ASSERT_VALID_WIDGET(child);

  return std::find(m_children.begin(), m_children.end(), child) != m_children.end();
}

bool Widget::hasAncestor(Widget* ancestor)
{
  for (Widget* widget=m_parent; widget; widget=widget->m_parent) {
    if (widget == ancestor)
      return true;
  }
  return false;
}

Widget* Widget::findChild(const char* id)
{
  for (auto child : m_children) {
    if (child->id() == id)
      return child;
  }

  for (auto child : m_children) {
    auto subchild = child->findChild(id);
    if (subchild)
      return subchild;
  }

  return nullptr;
}

Widget* Widget::findSibling(const char* id)
{
  return window()->findChild(id);
}

// TODO: Remove when widget can own all children
void Widget::addChild(Widget* child) {
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);

  if (child->m_wasInjected)
    m_ownedChildren.push_back(child->shared_from_this());
  child->m_hold.reset();

  m_children.push_back(child);
  child->m_parent = this;
  child->setManager(manager());
}

void Widget::addChild(std::shared_ptr<Widget> child) {
  m_ownedChildren.push_back(child);
  child->m_hold.reset();

  m_children.push_back(child.get());
  child->m_parent = this;
  child->setManager(manager());
}

void Widget::removeChild(const WidgetsList::iterator& it)
{
  Widget* child = *it;

  ASSERT(it != m_children.end());
  if (it == m_children.end())
    return;

  m_children.erase(it);

  for (auto it = m_ownedChildren.begin(); it != m_ownedChildren.end(); ++it) {
    if (it->get() == child) {
      m_ownedChildren.erase(it);
      break;
    }
  }

  // Free from manager
  Manager* manager = this->manager();
  if (manager)
    manager->freeWidget(child);

  child->m_parent = NULL;
}

// TODO: Remove when widget can own all children
void Widget::removeChild(Widget* child) {
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);
  removeChild(std::find(m_children.begin(), m_children.end(), child));
}

void Widget::removeChild(std::shared_ptr<Widget> child) {
  removeChild(child.get());
}

void Widget::removeAllChildren()
{
  while (!m_children.empty())
    removeChild(--m_children.end());
}

void Widget::replaceChild(Widget* oldChild, Widget* newChild)
{
  ASSERT_VALID_WIDGET(oldChild);
  ASSERT_VALID_WIDGET(newChild);

  WidgetsList::iterator before =
    std::find(m_children.begin(), m_children.end(), oldChild);
  if (before == m_children.end()) {
    ASSERT(false);
    return;
  }
  int index = before - m_children.begin();

  removeChild(oldChild);

  if (newChild->m_wasInjected)
    m_ownedChildren.push_back(newChild->shared_from_this());
  newChild->m_hold.reset();

  m_children.insert(m_children.begin()+index, newChild);
  newChild->m_parent = this;
}

void Widget::insertChild(int index, Widget* child)
{
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);

  if (child->m_wasInjected)
    m_ownedChildren.push_back(child->shared_from_this());
  child->m_hold.reset();

  m_children.insert(m_children.begin()+index, child);
  child->m_parent = this;
}

// ===============================================================
// LAYOUT & CONSTRAINT
// ===============================================================

void Widget::layout()
{
  setBounds(bounds());
  invalidate();
}

void Widget::loadLayout()
{
  if (!m_id.empty()) {
    LayoutIO* io = manager()->getLayoutIO();
    if (io) {
      std::string layout = io->loadLayout(this);
      if (!layout.empty()) {
        std::stringstream s(layout);
        LoadLayoutEvent ev(this, s);
        onLoadLayout(ev);
      }
    }
  }

  // Do for all children
  for (auto child : m_children)
    child->loadLayout();
}

void Widget::saveLayout()
{
  if (!m_id.empty()) {
    LayoutIO* io = manager()->getLayoutIO();
    if (io) {
      std::stringstream s;
      SaveLayoutEvent ev(this, s);
      onSaveLayout(ev);

      std::string layout = s.str();
      if (!layout.empty())
        io->saveLayout(this, layout);
    }
  }

  // Do for all children
  for (auto child : m_children)
    child->saveLayout();
}

void Widget::setDecorativeWidgetBounds()
{
  onSetDecorativeWidgetBounds();
}

// ===============================================================
// POSITION & GEOMETRY
// ===============================================================

Rect Widget::childrenBounds() const
{
  return Rect(m_bounds.x + border().left(),
              m_bounds.y + border().top(),
              m_bounds.w - border().width(),
              m_bounds.h - border().height());
}

Rect Widget::clientChildrenBounds() const
{
  return Rect(border().left(),
              border().top(),
              m_bounds.w - border().width(),
              m_bounds.h - border().height());
}

void Widget::setBounds(const Rect& rc)
{
  ResizeEvent ev(this, rc);
  onResize(ev);
}

void Widget::setBoundsQuietly(const gfx::Rect& rc)
{
  if (m_bounds != rc) {
    m_bounds = rc;

    // Remove all paint messages for this widget.
    if (Manager* manager = this->manager())
      manager->removeMessagesFor(this, kPaintMessage);
  }

  invalidate();
}

void Widget::setBorder(const Border& br)
{
  m_border = br;
}

void Widget::setChildSpacing(int childSpacing)
{
  m_childSpacing = childSpacing;
}

void Widget::noBorderNoChildSpacing()
{
  m_border = gfx::Border(0, 0, 0, 0);
  m_childSpacing = 0;
}

void Widget::getRegion(gfx::Region& region)
{
  if (type() == kWindowWidget)
    theme()->getWindowMask(this, region);
  else
    region = bounds();
}

void Widget::getDrawableRegion(gfx::Region& region, DrawableRegionFlags flags)
{
  Widget* window, *manager, *view;

  getRegion(region);

  // Cut the top windows areas
  if (flags & kCutTopWindows) {
    window = this->window();
    manager = (window ? window->manager(): nullptr);

    while (manager) {
      const WidgetsList& windows_list = manager->children();
      WidgetsList::const_reverse_iterator it =
        std::find(windows_list.rbegin(), windows_list.rend(), window);

      if (!windows_list.empty() &&
          window != windows_list.front() &&
          it != windows_list.rend()) {
        // Subtract the rectangles
        for (++it; it != windows_list.rend(); ++it) {
          if (!(*it)->isVisible())
            continue;

          Region reg1;
          (*it)->getRegion(reg1);
          region.createSubtraction(region, reg1);
        }
      }

      window = manager->window();
      manager = (window ? window->manager(): nullptr);
    }
  }

  // Clip the areas where are children
  if (!(flags & kUseChildArea) && !children().empty()) {
    Region reg1;
    Region reg2(childrenBounds());

    for (auto child : children()) {
      if (child->isVisible()) {
        Region reg3;
        child->getRegion(reg3);

        if (child->hasFlags(DECORATIVE)) {
          reg1 = bounds();
          reg1.createIntersection(reg1, reg3);
        }
        else {
          reg1.createIntersection(reg2, reg3);
        }
        region.createSubtraction(region, reg1);
      }
    }
  }

  // Intersect with the parent area
  if (!hasFlags(DECORATIVE)) {
    Widget* p = this->parent();
    while (p) {
      region.createIntersection(
        region, Region(p->childrenBounds()));
      p = p->parent();
    }
  }
  else {
    Widget* p = parent();
    if (p) {
      region.createIntersection(
        region, Region(p->bounds()));
    }
  }

  // Limit to the manager area
  window = this->window();
  manager = (window ? window->manager(): nullptr);

  while (manager) {
    view = View::getView(manager);

    Rect cpos;
    if (view) {
      cpos = static_cast<View*>(view)->viewportBounds();
    }
    else
      cpos = manager->childrenBounds();

    region.createIntersection(region, Region(cpos));

    window = manager->window();
    manager = (window ? window->manager(): nullptr);
  }
}

int Widget::textWidth() const
{
  return Graphics::measureUIStringLength(text().c_str(), font());
}

int Widget::textHeight() const
{
  return font()->height();
}

void Widget::getTextIconInfo(
  gfx::Rect* box,
  gfx::Rect* text,
  gfx::Rect* icon,
  int icon_align, int icon_w, int icon_h)
{
#define SETRECT(r)                              \
  if (r) {                                      \
    r->x = r##_x;                               \
    r->y = r##_y;                               \
    r->w = r##_w;                               \
    r->h = r##_h;                               \
  }

  gfx::Rect bounds = clientBounds();
  int box_x, box_y, box_w, box_h, icon_x, icon_y;
  int text_x, text_y, text_w, text_h;

  text_x = text_y = 0;

  // Size of the text
  if (hasText()) {
    text_w = textWidth();
    text_h = textHeight();
  }
  else {
    text_w = text_h = 0;
  }

  // Box size
  if (icon_align & CENTER) {   // With the icon in the center
    if (icon_align & MIDDLE) { // With the icon inside the text
      box_w = MAX(icon_w, text_w);
      box_h = MAX(icon_h, text_h);
    }
    // With the icon in the top or bottom
    else {
      box_w = MAX(icon_w, text_w);
      box_h = icon_h + (hasText() ? childSpacing(): 0) + text_h;
    }
  }
  // With the icon in left or right that doesn't care by now
  else {
    box_w = icon_w + (hasText() ? childSpacing(): 0) + text_w;
    box_h = MAX(icon_h, text_h);
  }

  // Box position
  if (align() & RIGHT)
    box_x = bounds.x2() - box_w - border().right();
  else if (align() & CENTER)
    box_x = (bounds.x+bounds.x2())/2 - box_w/2;
  else
    box_x = bounds.x + border().left();

  if (align() & BOTTOM)
    box_y = bounds.y2() - box_h - border().bottom();
  else if (align() & MIDDLE)
    box_y = (bounds.y+bounds.y2())/2 - box_h/2;
  else
    box_y = bounds.y + border().top();

  // With text
  if (hasText()) {
    // Text/icon X position
    if (icon_align & RIGHT) {
      text_x = box_x;
      icon_x = box_x + box_w - icon_w;
    }
    else if (icon_align & CENTER) {
      text_x = box_x + box_w/2 - text_w/2;
      icon_x = box_x + box_w/2 - icon_w/2;
    }
    else {
      text_x = box_x + box_w - text_w;
      icon_x = box_x;
    }

    // Text Y position
    if (icon_align & BOTTOM) {
      text_y = box_y;
      icon_y = box_y + box_h - icon_h;
    }
    else if (icon_align & MIDDLE) {
      text_y = box_y + box_h/2 - text_h/2;
      icon_y = box_y + box_h/2 - icon_h/2;
    }
    else {
      text_y = box_y + box_h - text_h;
      icon_y = box_y;
    }
  }
  // Without text
  else {
    // Icon X/Y position
    icon_x = box_x;
    icon_y = box_y;
  }

  SETRECT(box);
  SETRECT(text);
  SETRECT(icon);
}

void Widget::setMinSize(const gfx::Size& sz)
{
  m_minSize = sz;
}

void Widget::setMaxSize(const gfx::Size& sz)
{
  m_maxSize = sz;
}

void Widget::flushRedraw() {
  std::queue<Widget*> processing;
  if (hasFlags(DIRTY)) {
    disableFlags(DIRTY);
    processing.push(this);
  }

  Manager* manager = this->manager();
  ASSERT(manager);

  while (!processing.empty()) {
    Widget* widget = processing.front();
    processing.pop();

    ASSERT_VALID_WIDGET(widget);

    // If the widget is hidden
    if (!widget->isVisible())
      continue;

    for (auto child : widget->children()) {
      if (child->hasFlags(DIRTY)) {
        child->disableFlags(DIRTY);
        processing.push(child);
      }
    }

    if (!widget->m_updateRegion.isEmpty()) {
      // Intersect m_updateRegion with drawable area.
      {
        Region drawable;
        widget->getDrawableRegion(drawable, kCutTopWindows);
        widget->m_updateRegion &= drawable;
      }

      std::size_t c, nrects = widget->m_updateRegion.size();
      Region::const_iterator it = widget->m_updateRegion.begin();

      // Draw the widget
      int count = nrects-1;
      for (c=0; c<nrects; ++c, ++it, --count) {
        // Create the draw message
        auto msg = std::make_shared<PaintMessage>(count, *it);
        msg->addRecipient(widget);
        manager->enqueueMessage(msg);
      }

      manager->addInvalidRegion(widget->m_updateRegion);
      widget->m_updateRegion.clear();
    }
  }
}

void Widget::paint(Graphics* graphics, const gfx::Region& drawRegion)
{
  if (drawRegion.isEmpty())
    return;

  std::queue<Widget*> processing;
  processing.push(this);

  while (!processing.empty()) {
    Widget* widget = processing.front();
    processing.pop();

    ASSERT_VALID_WIDGET(widget);

    // If the widget is hidden
    if (!widget->isVisible())
      continue;

    for (auto child : widget->children())
      processing.push(child);

    // Intersect drawRegion with widget's drawable region.
    Region region;
    widget->getDrawableRegion(region, kCutTopWindows);
    region.createIntersection(region, drawRegion);

    Graphics graphics2(
      graphics->getInternalSurface(),
      widget->bounds().x,
      widget->bounds().y);
    graphics2.setFont(widget->font());

    for (Region::const_iterator
           it = region.begin(),
           end = region.end(); it != end; ++it) {
      IntersectClip clip(&graphics2, Rect(*it).offset(
          -widget->bounds().x,
          -widget->bounds().y));
      widget->paintEvent(&graphics2);
    }
  }
}

bool Widget::paintEvent(Graphics* graphics)
{
  // For transparent widgets we have to draw the parent first.
  if (isTransparent()) {
#if _DEBUG
    // In debug mode we can fill the area with Red so we know if
    // we are drawing the parent correctly.
    graphics->fillRect(gfx::rgba(255, 0, 0), clientBounds());
#endif

    enableFlags(HIDDEN);

    if (parent()) {
      gfx::Region rgn(parent()->bounds());
      rgn.createIntersection(
        rgn,
        gfx::Region(
          graphics->getClipBounds().offset(
            graphics->getInternalDeltaX(),
            graphics->getInternalDeltaY())));
      parent()->paint(graphics, rgn);
    }

    disableFlags(HIDDEN);
  }

  PaintEvent ev(this, graphics);
  onPaint(ev); // Fire onPaint event
  return ev.isPainted();
}

bool Widget::isDoubleBuffered() const
{
  return hasFlags(DOUBLE_BUFFERED);
}

void Widget::setDoubleBuffered(bool doubleBuffered)
{
  enableFlags(DOUBLE_BUFFERED);
}

bool Widget::isTransparent() const
{
  return hasFlags(TRANSPARENT);
}

void Widget::setTransparent(bool transparent)
{
  enableFlags(TRANSPARENT);
}

void Widget::invalidate()
{
  if (isVisible()) {
    m_updateRegion.clear();
    getDrawableRegion(m_updateRegion, kCutTopWindows);

    mark_dirty_flag(this);

    for (auto child : m_children)
      child->invalidate();
  }
}

void Widget::invalidateRect(const gfx::Rect& rect)
{
  if (isVisible())
    invalidateRegion(Region(rect));
}

void Widget::invalidateRegion(const Region& region)
{
  onInvalidateRegion(region);
}

class DeleteGraphicsAndSurface {
public:
  DeleteGraphicsAndSurface(const gfx::Rect& clip, she::Surface* surface)
    : m_pt(clip.origin()), m_surface(surface) {
  }

  void operator()(Graphics* graphics) {
    {
      she::Surface* dst = she::instance()->defaultDisplay()->getSurface();
      she::SurfaceLock lockSrc(m_surface);
      she::SurfaceLock lockDst(dst);
      m_surface->blitTo(
        dst, 0, 0, m_pt.x, m_pt.y,
        m_surface->width(), m_surface->height());
    }
    m_surface->dispose();
    delete graphics;
  }

private:
  gfx::Point m_pt;
  she::Surface* m_surface;
};

GraphicsPtr Widget::getGraphics(const gfx::Rect& clip)
{
  GraphicsPtr graphics;
  she::Surface* surface;
  she::Surface* defaultSurface = she::instance()->defaultDisplay()->getSurface();

  // In case of double-buffering, we need to create the temporary
  // buffer only if the default surface is the screen.
  if (isDoubleBuffered() && defaultSurface->isDirectToScreen()) {
    surface = she::instance()->createSurface(clip.w, clip.h);
    graphics.reset(new Graphics(surface, -clip.x, -clip.y),
      DeleteGraphicsAndSurface(clip, surface));
  }
  // In other case, we can draw directly onto the screen.
  else {
    surface = defaultSurface;
    graphics.reset(new Graphics(surface, bounds().x, bounds().y));
  }

  graphics->setFont(font());
  return graphics;
}

// ===============================================================
// GUI MANAGER
// ===============================================================

bool Widget::sendMessage(Message* msg)
{
  ASSERT(msg);
  return onProcessMessage(msg);
}

void Widget::closeWindow()
{
  if (Window* w = window())
    w->closeWindow(this);
}

void Widget::broadcastMouseMessage(WidgetsList& targets)
{
  onBroadcastMouseMessage(targets);
}

// ===============================================================
// SIZE & POSITION
// ===============================================================

/**
   Returns the preferred size of the Widget.

   It checks if the preferred size is static (it means when it was
   set through #setSizeHint before) or if it is dynamic (this is
   the default and is when the #onSizeHint is used to determined
   the preferred size).

   In another words, if you do not use #setSizeHint to set a
   <em>static preferred size</em> for the widget then #onSizeHint
   will be used to calculate it.

   @see setSizeHint, onSizeHint, #sizeHint(const Size &)
*/
Size Widget::sizeHint()
{
  if (m_sizeHint != NULL)
    return *m_sizeHint;
  else {
    SizeHintEvent ev(this, Size(0, 0));
    onSizeHint(ev);

    Size sz(ev.sizeHint());
    sz.w = MID(m_minSize.w, sz.w, m_maxSize.w);
    sz.h = MID(m_minSize.h, sz.h, m_maxSize.h);
    return sz;
  }
}

/**
   Returns the preferred size trying to fit in the specified size.
   Remember that if you use #setSizeHint this routine will
   return the static size which you specified manually.

   @param fitIn
       This can have both attributes (width and height) in
       zero, which means that it'll behave same as #sizeHint().
       If the width is great than zero the #onSizeHint will try to
       fit in that width (this is useful to fit Label or Edit controls
       in a specified width and calculate the height it could occupy).

   @see sizeHint
*/
Size Widget::sizeHint(const Size& fitIn)
{
  if (m_sizeHint != NULL)
    return *m_sizeHint;
  else {
    SizeHintEvent ev(this, fitIn);
    onSizeHint(ev);

    Size sz(ev.sizeHint());
    sz.w = MID(m_minSize.w, sz.w, m_maxSize.w);
    sz.h = MID(m_minSize.h, sz.h, m_maxSize.h);
    return sz;
  }
}

/**
   Sets a fixed preferred size specified by the user.
   Widget::sizeHint() will return this value if it's setted.
*/
void Widget::setSizeHint(const Size& fixedSize)
{
  m_sizeHint.reset(new Size(fixedSize));
}

void Widget::setSizeHint(int fixedWidth, int fixedHeight)
{
  setSizeHint(Size(fixedWidth, fixedHeight));
}

// ===============================================================
// FOCUS & MOUSE
// ===============================================================

void Widget::requestFocus()
{
  manager()->setFocus(this);
}

void Widget::releaseFocus()
{
  if (hasFocus())
    manager()->freeFocus();
}

/**
 * Captures the mouse to send all the future mouse messsages to the
 * specified widget (included the kMouseMoveMessage and kSetCursorMessage).
 */
void Widget::captureMouse()
{
  if (!manager()->getCapture()) {
    manager()->setCapture(this);
  }
}

/**
 * Releases the capture of the mouse events.
 */
void Widget::releaseMouse()
{
  if (manager()->getCapture() == this) {
    manager()->freeCapture();
  }
}

bool Widget::offerCapture(ui::MouseMessage* mouseMsg, int widget_type)
{
  if (hasCapture()) {
    Widget* pick = manager()->pick(mouseMsg->position());
    if (pick && pick != this && pick->type() == widget_type) {
      releaseMouse();
      auto mouseMsg2 = std::make_shared<MouseMessage>(
        kMouseDownMessage,
        mouseMsg->pointerType(),
        mouseMsg->buttons(),
        mouseMsg->modifiers(),
        mouseMsg->position());
      mouseMsg2->addRecipient(pick);
      manager()->enqueueMessage(mouseMsg2);
      return true;
    }
  }
  return false;
}

bool Widget::hasFocus()
{
  return hasFlags(HAS_FOCUS);
}

bool Widget::hasMouse()
{
  return hasFlags(HAS_MOUSE);
}

bool Widget::hasMouseOver()
{
  return (this == pick(get_mouse_position()));
}

bool Widget::hasCapture()
{
  return hasFlags(HAS_CAPTURE);
}

int Widget::mnemonicChar() const
{
  if (hasText()) {
    for (int c=0; m_text[c]; ++c)
      if ((m_text[c] == '&') && (m_text[c+1] != '&'))
        return tolower(m_text[c+1]);
  }
  return 0;
}

bool Widget::onProcessMessage(Message* msg)
{
  ASSERT(msg != NULL);

  switch (msg->type()) {

    case kOpenMessage:
    case kCloseMessage:
    case kWinMoveMessage:
      // Broadcast the message to the children.
      for (auto child : m_children)
        child->sendMessage(msg);
      break;

    case kPaintMessage: {
      const PaintMessage* ptmsg = static_cast<const PaintMessage*>(msg);
      ASSERT(ptmsg->rect().w > 0);
      ASSERT(ptmsg->rect().h > 0);

      GraphicsPtr graphics = getGraphics(toClient(ptmsg->rect()));
      return paintEvent(graphics.get());
    }

    case kKeyDownMessage:
    case kKeyUpMessage:
      if (static_cast<KeyMessage*>(msg)->propagateToChildren()) {
        // Broadcast the message to the children.
        for (auto child : m_children)
          if (child->sendMessage(msg))
            return true;
      }

      // Propagate the message to the parent.
      if (static_cast<KeyMessage*>(msg)->propagateToParent() &&
          parent()) {
        return parent()->sendMessage(msg);
      }
      else
        break;

    case kDoubleClickMessage: {
      // Convert double clicks into mouse down
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      MouseMessage mouseMsg2(kMouseDownMessage,
                             mouseMsg->pointerType(),
                             mouseMsg->buttons(),
                             mouseMsg->modifiers(),
                             mouseMsg->position(),
                             mouseMsg->wheelDelta());
      sendMessage(&mouseMsg2);
      break;
    }

    case kMouseDownMessage:
    case kMouseUpMessage:
    case kMouseMoveMessage:
    case kMouseWheelMessage:
      // Propagate the message to the parent.
      if (parent() != NULL)
        return parent()->sendMessage(msg);
      else
        break;

    case kSetCursorMessage:
      // Propagate the message to the parent.
      if (parent())
        return parent()->sendMessage(msg);
      else {
        set_mouse_cursor(kArrowCursor);
        return true;
      }

  }

  return false;
}

// ===============================================================
// EVENTS
// ===============================================================

void Widget::onInvalidateRegion(const Region& region)
{
  if (isVisible() && region.contains(bounds()) != Region::Out) {
    Region reg1;
    reg1.createUnion(m_updateRegion, region);
    {
      Region reg2;
      getDrawableRegion(reg2, kCutTopWindows);
      m_updateRegion.createIntersection(reg1, reg2);
    }
    reg1.createSubtraction(region, m_updateRegion);

    mark_dirty_flag(this);

    for (auto child : m_children)
      child->invalidateRegion(reg1);
  }
}

void Widget::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(m_minSize);
}

void Widget::onLoadLayout(LoadLayoutEvent& ev)
{
  // Do nothing
}

void Widget::onSaveLayout(SaveLayoutEvent& ev)
{
  // Do nothing
}

void Widget::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.bounds());

  // Set all the children to the same "cpos".
  gfx::Rect cpos = childrenBounds();
  for (auto child : m_children)
    child->setBounds(cpos);
}

void Widget::onPaint(PaintEvent& ev)
{
  // Do nothing
}

void Widget::onBroadcastMouseMessage(WidgetsList& targets)
{
  // Do nothing
}

void Widget::onInitTheme(InitThemeEvent& ev)
{
  if (m_theme) {
    m_theme->initWidget(this);

    if (!hasFlags(INITIALIZED))
      enableFlags(INITIALIZED);
  }
}

void Widget::onSetDecorativeWidgetBounds()
{
  if (m_theme) {
    m_theme->setDecorativeWidgetBounds(this);
  }
}

void Widget::onEnable()
{
  // Do nothing
}

void Widget::onDisable()
{
  // Do nothing
}

void Widget::onSelect()
{
  // Do nothing
}

void Widget::onDeselect()
{
  // Do nothing
}

void Widget::onSetText()
{
  invalidate();
}

void Widget::onSetBgColor()
{
  invalidate();
}

void Widget::offsetWidgets(int dx, int dy)
{
  m_updateRegion.offset(dx, dy);
  m_bounds.offset(dx, dy);

  // Remove all paint messages for this widget.
  if (Manager* manager = this->manager())
    manager->removeMessagesFor(this, kPaintMessage);

  for (auto child : m_children)
    child->offsetWidgets(dx, dy);
}

} // namespace ui

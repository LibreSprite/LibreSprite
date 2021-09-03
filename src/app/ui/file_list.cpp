// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/file_list.h"

#include "app/modules/gfx.h"
#include "app/thumbnail_generator.h"
#include "app/ui/skin/skin_theme.h"
#include "base/string.h"
#include "she/font.h"
#include "she/surface.h"
#include "ui/ui.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#define ISEARCH_KEYPRESS_INTERVAL_MSECS 500

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

FileList::FileList()
  : Widget(kGenericWidget)
  , m_generateThumbnailTimer(200, this)
  , m_monitoringTimer(50, this)
  , m_thumbnail(nullptr)
{
  setFocusStop(true);
  setDoubleBuffered(true);

  m_currentFolder = FileSystemModule::instance()->getRootFileItem();
  m_req_valid = false;
  m_selected = NULL;
  m_isearchClock = 0;

  m_itemToGenerateThumbnail = NULL;

  m_generateThumbnailTimer.Tick.connect(&FileList::onGenerateThumbnailTick, this);
  m_monitoringTimer.Tick.connect(&FileList::onMonitoringTick, this);
  m_monitoringTimer.start();

  regenerateList();
}

FileList::~FileList()
{
  // Stop timers.
  m_generateThumbnailTimer.stop();
  m_monitoringTimer.stop();

  // Stop workers creating thumbnails.
  ThumbnailGenerator::instance()->stopAllWorkers();
}

void FileList::setExtensions(const char* extensions)
{
  m_exts = extensions;

  // Refresh
  if (isVisible())
    setCurrentFolder(m_currentFolder);
}

void FileList::setCurrentFolder(IFileItem* folder)
{
  ASSERT(folder != NULL);
  ASSERT(folder->isBrowsable());

  m_currentFolder = folder;
  m_req_valid = false;
  m_selected = NULL;

  regenerateList();

  // select first folder
  if (!m_list.empty() && m_list.front()->isBrowsable())
    selectIndex(0);

  // Emit "CurrentFolderChanged" event.
  onCurrentFolderChanged();

  invalidate();
  View::getView(this)->updateView();

  requestFocus();
}

void FileList::goUp()
{
  IFileItem* folder = m_currentFolder;
  IFileItem* parent = folder->parent();
  if (parent) {
    setCurrentFolder(parent);
    m_selected = folder;

    // Make the selected item visible.
    makeSelectedFileitemVisible();
  }
}

bool FileList::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseDownMessage:
      captureMouse();

    case kMouseMoveMessage:
      if (hasCapture()) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        int th = textHeight();
        int y = bounds().y;
        IFileItem* old_selected = m_selected;
        m_selected = NULL;

        // rows
        for (FileItemList::iterator
               it=m_list.begin();
             it!=m_list.end(); ++it) {
          IFileItem* fi = *it;
          gfx::Size itemSize = getFileItemSize(fi);

          if (((mouseMsg->position().y >= y) &&
               (mouseMsg->position().y < y+th+4*guiscale())) ||
              (it == m_list.begin() && mouseMsg->position().y < y) ||
              (it == m_list.end()-1 && mouseMsg->position().y >= y+th+4*guiscale())) {
            m_selected = fi;
            makeSelectedFileitemVisible();
            break;
          }

          y += itemSize.h;
        }

        if (old_selected != m_selected) {
          generatePreviewOfSelectedItem();

          invalidate();

          // Emit "FileSelected" event.
          onFileSelected();
        }
      }
      break;

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();
      }
      break;

    case kKeyDownMessage:
      if (hasFocus()) {
        KeyMessage* keyMsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keyMsg->scancode();
        int unicodeChar = keyMsg->unicodeChar();
        int select = getSelectedIndex();
        View* view = View::getView(this);
        int bottom = m_list.size();

        switch (scancode) {

          case kKeyUp:
            if (select >= 0)
              select--;
            else
              select = 0;
            break;

          case kKeyDown:
            if (select >= 0)
              select++;
            else
              select = 0;
            break;

          case kKeyHome:
            select = 0;
            break;

          case kKeyEnd:
            select = bottom-1;
            break;

          case kKeyPageUp:
          case kKeyPageDown: {
            int sgn = (scancode == kKeyPageUp) ? -1: 1;
            gfx::Rect vp = view->viewportBounds();
            if (select < 0)
              select = 0;
            select += sgn * vp.h / (textHeight()+4*guiscale());
            break;
          }

          case kKeyLeft:
          case kKeyRight:
            if (select >= 0) {
              gfx::Rect vp = view->viewportBounds();
              int sgn = (scancode == kKeyLeft) ? -1: 1;
              gfx::Point scroll = view->viewScroll();
              scroll.x += vp.w/2*sgn;
              view->setViewScroll(scroll);
            }
            break;

          case kKeyEnter:
          case kKeyEnterPad:
            if (m_selected) {
              if (m_selected->isBrowsable()) {
                setCurrentFolder(m_selected);
                return true;
              }
              if (m_selected->isFolder()) {
                // Do nothing (is a folder but not browseable).
                return true;
              }
              else {
                // Emit "FileAccepted" event.
                onFileAccepted();
                return true;
              }
            }
            else
              return Widget::onProcessMessage(msg);

          case kKeyBackspace:
            goUp();
            return true;

          default:
            if (unicodeChar == ' ' ||
                (std::tolower(unicodeChar) >= 'a' &&
                 std::tolower(unicodeChar) <= 'z') ||
                (unicodeChar >= '0' &&
                 unicodeChar <= '9')) {
              if ((base::current_tick() - m_isearchClock) > ISEARCH_KEYPRESS_INTERVAL_MSECS)
                m_isearch.clear();

              m_isearch.push_back(unicodeChar);

              int i, chrs = m_isearch.size();
              FileItemList::iterator
                link = m_list.begin() + ((select >= 0) ? select: 0);

              for (i=MAX(select, 0); i<bottom; ++i, ++link) {
                IFileItem* fi = *link;
                if (base::utf8_icmp(fi->displayName(), m_isearch, chrs) == 0) {
                  select = i;
                  break;
                }
              }
              m_isearchClock = base::current_tick();
              // Go to selectIndex...
            }
            else
              return Widget::onProcessMessage(msg);
        }

        if (bottom > 0)
          selectIndex(MID(0, select, bottom-1));

        return true;
      }
      break;

    case kMouseWheelMessage: {
      View* view = View::getView(this);
      if (view) {
        gfx::Point scroll = view->viewScroll();
        scroll += static_cast<MouseMessage*>(msg)->wheelDelta() * 3*(textHeight()+4*guiscale());
        view->setViewScroll(scroll);
      }
      break;
    }

    case kDoubleClickMessage:
      if (m_selected) {
        if (m_selected->isBrowsable()) {
          setCurrentFolder(m_selected);
          return true;
        }
        else {
          onFileAccepted();         // Emit "FileAccepted" event.
          return true;
        }
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

int FileList::thumbnailY()
{
  int y = 0;
  for (IFileItem* fi : m_list) {
    gfx::Size itemSize = getFileItemSize(fi);
    if (fi == m_selected) {
      if (fi->getThumbnail())
        return y + itemSize.h/2;
      else
        break;
    }
    y += itemSize.h;
  }
  return 0;
}

void FileList::onPaint(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  gfx::Rect bounds = clientBounds();
  int x, y = bounds.y;
  int evenRow = 0;
  gfx::Color bgcolor;
  gfx::Color fgcolor;

  g->fillRect(theme->colors.background(), bounds);

  // rows
  m_thumbnail = nullptr;
  for (IFileItem* fi : m_list) {
    gfx::Size itemSize = getFileItemSize(fi);

    if (fi == m_selected) {
      fgcolor = theme->colors.filelistSelectedRowText();
      bgcolor = theme->colors.filelistSelectedRowFace();
    }
    else {
      bgcolor = evenRow ? theme->colors.filelistEvenRowFace():
                          theme->colors.filelistOddRowFace();

      if (fi->isFolder() && !fi->isBrowsable())
        fgcolor = theme->colors.filelistDisabledRowText();
      else
        fgcolor = evenRow ? theme->colors.filelistEvenRowText():
                            theme->colors.filelistOddRowText();
    }

    x = bounds.x+2*guiscale();

    // Item background
    g->fillRect(bgcolor, gfx::Rect(bounds.x, y, bounds.w, itemSize.h));

    if (fi->isFolder()) {
      int icon_w = font()->textLength("[+]");

      g->drawString("[+]", fgcolor, bgcolor, gfx::Point(x, y+2*guiscale()));
      x += icon_w+2*guiscale();
    }

    // item name
    g->drawString(
      fi->displayName().c_str(),
      fgcolor, bgcolor, gfx::Point(x, y+2*guiscale()));

    // draw progress bars
    double progress;
    ThumbnailGenerator::WorkerStatus workerStatus =
      ThumbnailGenerator::instance()->getWorkerStatus(fi, progress);

    if (workerStatus == ThumbnailGenerator::WorkingOnThumbnail) {
      int barw = 64*guiscale();

      theme->paintProgressBar(g,
        gfx::Rect(
          bounds.x2()-2*guiscale()-barw,
          y+itemSize.h/2-3*guiscale(),
          barw, 6*guiscale()),
        progress);
    }

    // Thumbnail position
    if (fi == m_selected)
      m_thumbnail = fi->getThumbnail();

    y += itemSize.h;
    evenRow ^= 1;
  }

  // Draw the thumbnail
  if (m_thumbnail) {
    gfx::Rect tbounds = thumbnailBounds();
    g->blit(m_thumbnail, 0, 0, tbounds.x, tbounds.y, tbounds.w, tbounds.h);
    g->drawRect(gfx::rgba(0, 0, 0), tbounds.enlarge(1));
  }
}

gfx::Rect FileList::thumbnailBounds()
{
  if (!m_selected ||
      !m_selected->getThumbnail())
    return gfx::Rect();

  she::Surface* thumbnail = m_selected->getThumbnail();
  View* view = View::getView(this);
  gfx::Rect vp = view->viewportBounds();
  int x = vp.x+vp.w - 2*guiscale() - thumbnail->width();
  int y = thumbnailY() - thumbnail->height()/2 + bounds().y;
  y = MID(vp.y+2*guiscale(), y, vp.y+vp.h-3*guiscale()-thumbnail->height());
  x -= bounds().x;
  y -= bounds().y;
  return gfx::Rect(x, y, thumbnail->width(), thumbnail->height());
}

void FileList::onSizeHint(SizeHintEvent& ev)
{
  if (!m_req_valid) {
    gfx::Size reqSize(0, 0);

    // rows
    for (FileItemList::iterator
           it=m_list.begin();
         it!=m_list.end(); ++it) {
      IFileItem* fi = *it;
      gfx::Size itemSize = getFileItemSize(fi);
      reqSize.w = MAX(reqSize.w, itemSize.w);
      reqSize.h += itemSize.h;
    }

    m_req_valid = true;
    m_req_w = reqSize.w;
    m_req_h = reqSize.h;
  }
  ev.setSizeHint(Size(m_req_w, m_req_h));
}

void FileList::onFileSelected()
{
  FileSelected();
}

void FileList::onFileAccepted()
{
  FileAccepted();
}

void FileList::onCurrentFolderChanged()
{
  CurrentFolderChanged();
}

void FileList::onMonitoringTick()
{
  if (ThumbnailGenerator::instance()->checkWorkers())
    invalidate();
}

void FileList::onGenerateThumbnailTick()
{
  m_generateThumbnailTimer.stop();

  IFileItem* fileitem = m_itemToGenerateThumbnail;
  if (fileitem)
    ThumbnailGenerator::instance()->addWorkerToGenerateThumbnail(fileitem);
}

gfx::Size FileList::getFileItemSize(IFileItem* fi) const
{
  int len = 0;

  if (fi->isFolder())
    len += font()->textLength("[+]") + 2*guiscale();

  len += font()->textLength(fi->displayName().c_str());

  return gfx::Size(len+4*guiscale(), textHeight()+4*guiscale());
}

void FileList::makeSelectedFileitemVisible()
{
  View* view = View::getView(this);
  gfx::Rect vp = view->viewportBounds();
  gfx::Point scroll = view->viewScroll();
  int th = textHeight();
  int y = bounds().y;

  // rows
  for (FileItemList::iterator
         it=m_list.begin();
       it!=m_list.end(); ++it) {
    IFileItem* fi = *it;
    gfx::Size itemSize = getFileItemSize(fi);

    if (fi == m_selected) {
      if (y < vp.y)
        scroll.y = y - bounds().y;
      else if (y > vp.y + vp.h - (th+4*guiscale()))
        scroll.y = y - bounds().y - vp.h + (th+4*guiscale());

      view->setViewScroll(scroll);
      break;
    }

    y += itemSize.h;
  }
}

void FileList::regenerateList()
{
  // get the children of the current folder
  m_list = m_currentFolder->children();

  // filter the list by the available extensions
  if (!m_exts.empty()) {
    for (FileItemList::iterator
           it=m_list.begin();
         it!=m_list.end(); ) {
      IFileItem* fileitem = *it;
      if (fileitem->isHidden())
        it = m_list.erase(it);
      else if (!fileitem->isFolder() &&
          !fileitem->hasExtension(m_exts.c_str())) {
        it = m_list.erase(it);
      }
      else
        ++it;
    }
  }
}

int FileList::getSelectedIndex()
{
  for (FileItemList::iterator
         it = m_list.begin();
       it != m_list.end(); ++it) {
    if (*it == m_selected)
      return it - m_list.begin();
  }

  return -1;
}

void FileList::selectIndex(int index)
{
  IFileItem* old_selected = m_selected;

  m_selected = m_list.at(index);
  if (old_selected != m_selected) {
    makeSelectedFileitemVisible();

    invalidate();

    // Emit "FileSelected" event.
    onFileSelected();
  }

  generatePreviewOfSelectedItem();
}

// Puts the selected file-item as the next item to be processed by the
// round-robin that generate thumbnails
void FileList::generatePreviewOfSelectedItem()
{
  if (m_selected &&
      !m_selected->isFolder() &&
      !m_selected->getThumbnail())
    {
      m_itemToGenerateThumbnail = m_selected;
      m_generateThumbnailTimer.start();
    }
}

} // namespace app

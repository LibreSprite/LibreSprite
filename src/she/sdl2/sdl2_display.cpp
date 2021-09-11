// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <cstddef>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/she.h"

#include "base/concurrent_queue.h"
#include "base/exception.h"
#include "base/string.h"
#include "base/unique_ptr.h"
#include "she/sdl2/sdl2_display.h"
#include "she/sdl2/sdl2_surface.h"
#include "she/common/system.h"
#include "she/logger.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <cassert>
#include <list>
#include <sstream>
#include <vector>
#include <iostream>
#include <unordered_map>

// void* get_osx_window();

namespace she {

  SDL2Display* unique_display = NULL;
  int display_scale;

  namespace sdl {
    she::SDL2Surface* screen;
    extern bool isMaximized;
    extern bool isMinimized;
    std::unordered_map<int, SDL2Display*> windowIdToDisplay;
  }

  SDL2Display::SDL2Display(int width, int height, int scale) :
    m_nativeSurface(nullptr),
    m_window(nullptr),
    m_renderer(nullptr),
    m_cursor(nullptr),
    m_surface(nullptr),
    m_scale(0),
    m_nativeCursor(kNoCursor),
    m_restoredWidth(0),
    m_restoredHeight(0) {

    unique_display = this;

    width = 800;
    height = 600;

    m_window = SDL_CreateWindow("",
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                width, height,
                                SDL_WINDOW_RESIZABLE);
    if (!m_window)
      throw DisplayCreationException(SDL_GetError());

    sdl::windowIdToDisplay[SDL_GetWindowID(m_window)] = this;
    m_width = width;
    m_height = height;

    SDL_ShowCursor(SDL_DISABLE);

    setScale(scale);
  }

  SDL2Display::~SDL2Display()
  {
    unique_display = NULL;
    sdl::windowIdToDisplay.erase(SDL_GetWindowID(m_window));
    m_surface->dispose();
  }

  void SDL2Display::dispose()
  {
    delete this;
  }

  int SDL2Display::width() const
  {
    return m_width;
  }

  int SDL2Display::height() const
  {
    return m_height;
  }

  void SDL2Display::setWidth(int newWidth)
  {
    std::cout << "New width: " << std::to_string(newWidth) << std::endl;
    m_width = newWidth;
  }

  void SDL2Display::setHeight(int newHeight)
  {
    std::cout << "New height: " << std::to_string(newHeight) << std::endl;
    m_height = newHeight;
  }

  int SDL2Display::originalWidth() const
  {
    return m_restoredWidth > 0 ? m_restoredWidth: width();
  }

  int SDL2Display::originalHeight() const
  {
    return m_restoredHeight > 0 ? m_restoredHeight: height();
  }

  void SDL2Display::setOriginalWidth(int width)
  {
    std::cout << "New Original width: " << std::to_string(width) << std::endl;
    m_restoredWidth = width;
  }

  void SDL2Display::setOriginalHeight(int height)
  {
    std::cout << "New Original height: " << std::to_string(height) << std::endl;
    m_restoredHeight = height;
  }

  int SDL2Display::scale() const
  {
    return m_scale;
  }

  void SDL2Display::setScale(int scale)
  {
    ASSERT(scale >= 1);
    if (m_scale == scale)
      return;

    m_scale = scale;
    recreateSurface();
  }

  void SDL2Display::recreateSurface()
  {
    auto  newSurface = new SDL2Surface(width() / m_scale, height() / m_scale, SDL2Surface::DeleteAndDestroy);
    if (m_surface) {
      m_surface->blitTo(newSurface, 0, 0, 0, 0, width(), height());
      m_surface->dispose();
    }
    m_dirty = true;
    m_surface = newSurface;
    she::sdl::screen = newSurface;
  }

  Surface* SDL2Display::getSurface()
  {
    return m_surface;
  }

  void SDL2Display::present() {
    if (!m_dirty)
      return;
    m_dirty = false;
    SDL_UpdateWindowSurface(m_window);
  }

  void SDL2Display::flip(const gfx::Rect& bounds)
  {
    m_dirty = true;

    auto nativeSurface = SDL_GetWindowSurface(m_window);
    if (m_nativeSurface != nativeSurface) {
      m_nativeSurface = nativeSurface;
      recreateSurface();
    }
    SDL_Rect rect {bounds.x, bounds.y, bounds.w, bounds.h};
    SDL_Rect dst  {bounds.x * m_scale, bounds.y * m_scale, bounds.w * m_scale, bounds.h * m_scale};
    SDL_BlitScaled((SDL_Surface*)m_surface->nativeHandle(), &rect, m_nativeSurface, &dst);
  }

  void SDL2Display::maximize()
  {
    SDL_MinimizeWindow(m_window);
  }

  bool SDL2Display::isMaximized() const
  {
    return sdl::isMaximized;
  }

  bool SDL2Display::isMinimized() const
  {
    return sdl::isMinimized;
  }

  void SDL2Display::setTitleBar(const std::string& title)
  {
    SDL_SetWindowTitle(m_window, title.c_str());
  }

  NativeCursor SDL2Display::nativeMouseCursor()
  {
    return m_nativeCursor;
  }

  bool SDL2Display::setNativeMouseCursor(NativeCursor cursor)
  {
    return false;

    if (cursor == m_nativeCursor) {
      return true;
    }

    if (m_cursor)
      SDL_FreeCursor(m_cursor);
    m_cursor = SDL_DISABLE;

    switch (cursor) {
    case kNoCursor: break;
    case kArrowCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW); break;
    case kIBeamCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM); break;
    case kWaitCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT); break;
    // case kHelpCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HELP); break;
    case kForbiddenCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO); break;
    case kMoveCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND); break;
    // case kLinkCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_LINK); break;
    case kSizeNSCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS); break;
    case kSizeWECursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE); break;
    // case kSizeNCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEN); break;
    // case kSizeNECursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENE); break;
    // case kSizeECursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEE); break;
    // case kSizeSECursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZESE); break;
    // case kSizeSCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZES); break;
    // case kSizeSWCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZESW); break;
    // case kSizeWCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEW); break;
    // case kSizeNWCursor: m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENW); break;
    default: break;
    }

    SDL_SetCursor(m_cursor);
    m_nativeCursor = cursor;
    return m_cursor ? true : false;
  }

  void SDL2Display::setMousePosition(const gfx::Point& position)
  {
    SDL_WarpMouseInWindow(
      m_window,
      m_scale * position.x,
      m_scale * position.y);
  }

  void SDL2Display::captureMouse()
  {
    SDL_CaptureMouse(SDL_TRUE);
  }

  void SDL2Display::releaseMouse()
  {
    SDL_CaptureMouse(SDL_FALSE);
  }

  std::string SDL2Display::getLayout()
  {
// #ifdef _WIN32
//   WINDOWPLACEMENT wp;
//   wp.length = sizeof(WINDOWPLACEMENT);
//   if (GetWindowPlacement((HWND)nativeHandle(), &wp)) {
//     std::ostringstream s;
//     s << 1 << ' '
//       << wp.flags << ' '
//       << wp.showCmd << ' '
//       << wp.ptMinPosition.x << ' '
//       << wp.ptMinPosition.y << ' '
//       << wp.ptMaxPosition.x << ' '
//       << wp.ptMaxPosition.y << ' '
//       << wp.rcNormalPosition.left << ' '
//       << wp.rcNormalPosition.top << ' '
//       << wp.rcNormalPosition.right << ' '
//       << wp.rcNormalPosition.bottom;
//     return s.str();
//   }
// #endif
    return "";
  }

  void SDL2Display::setLayout(const std::string& layout)
  {
// #ifdef _WIN32

//   WINDOWPLACEMENT wp;
//   wp.length = sizeof(WINDOWPLACEMENT);

//   std::istringstream s(layout);
//   int ver;
//   s >> ver;
//   if (ver == 1) {
//     s >> wp.flags
//       >> wp.showCmd
//       >> wp.ptMinPosition.x
//       >> wp.ptMinPosition.y
//       >> wp.ptMaxPosition.x
//       >> wp.ptMaxPosition.y
//       >> wp.rcNormalPosition.left
//       >> wp.rcNormalPosition.top
//       >> wp.rcNormalPosition.right
//       >> wp.rcNormalPosition.bottom;
//   }
//   else
//     return;

//   if (SetWindowPlacement((HWND)nativeHandle(), &wp)) {
//     // TODO use the return value
//   }
// #else
//   // Do nothing
// #endif
  }

  void* SDL2Display::nativeHandle()
  {
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version); /* initialize info structure with SDL version info */
    if (!SDL_GetWindowWMInfo(m_window, &info))
      return nullptr;

#if defined(SDL_VIDEO_DRIVER_WINDOWS)
    return reinterpret_cast<void*>(info.info.win.window);
#endif

#if defined(SDL_VIDEO_DRIVER_X11)
    return reinterpret_cast<void*>(info.info.x11.window);
#endif

#if defined(SDL_VIDEO_DRIVER_DIRECTFB)
    return reinterpret_cast<void*>(info.info.dfb.window);
#endif

#if defined(SDL_VIDEO_DRIVER_COCOA)
    return reinterpret_cast<void*>(info.info.cocoa.window);
#endif

#if defined(SDL_VIDEO_DRIVER_UIKIT)
    return reinterpret_cast<void*>(info.info.uikit.window);
#endif

#if defined(SDL_VIDEO_DRIVER_ANDROID)
    return reinterpret_cast<void*>(info.info.android.window);
#endif

    return nullptr;
  }

} // namespace she

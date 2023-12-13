// SHE library
// Copyright (C) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/she.h"

#include "base/concurrent_queue.h"
#include "base/exception.h"
#include "base/string.h"
#include "she/sdl2/sdl2_display.h"
#include "she/sdl2/sdl2_surface.h"
#include "she/common/system.h"
#include "she/logger.h"

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#else
#include <SDL.h>
#include <SDL_syswm.h>
#endif

#include <sstream>
#include <iostream>
#include <unordered_map>

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

    if (gpu)
      m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);

    sdl::windowIdToDisplay[SDL_GetWindowID(m_window)] = this;
    SDL_GetWindowSize(m_window, &width, &height);
    m_width = width;
    m_height = height;

    SDL_ShowCursor(SDL_DISABLE);

    setScale(scale);
  }

  SDL2Display::~SDL2Display()
  {
    unique_display = NULL;
    if (m_window) {
      sdl::windowIdToDisplay.erase(SDL_GetWindowID(m_window));
      m_surface->dispose();
      if (m_renderer)
        SDL_DestroyRenderer(m_renderer);
      SDL_DestroyWindow(m_window);
    }
  }

  void SDL2Display::dispose()
  {
    delete this;
  }

  void SDL2Display::toggleFullscreen()
  {
    m_isFullscreen = !m_isFullscreen;
    SDL_SetWindowFullscreen(m_window, m_isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  }

  bool SDL2Display::setIcon(Surface* surface) {
    SDL_SetWindowIcon(m_window, static_cast<SDL_Surface*>(static_cast<SDL2Surface*>(surface)->nativeHandle()));
    return true;
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
    m_width = newWidth;
  }

  void SDL2Display::setHeight(int newHeight)
  {
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
    m_restoredWidth = width;
  }

  void SDL2Display::setOriginalHeight(int height)
  {
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
    if (m_renderer)
      SDL_RenderPresent(m_renderer);
    else
      SDL_UpdateWindowSurface(m_window);
  }

  void SDL2Display::flip(const gfx::Rect& bounds)
  {
    m_dirty = true;

    SDL_Rect rect {bounds.x, bounds.y, bounds.w, bounds.h};
    if (m_renderer) {
      auto texture = static_cast<SDL2Surface*>(m_surface)->getTexture(rect);
      SDL_Rect dst {
        rect.x * m_scale, rect.y * m_scale,
        rect.w * m_scale, rect.h * m_scale
      };
      SDL_RenderCopy(m_renderer, texture, nullptr, nullptr);
    } else {
      auto nativeSurface = SDL_GetWindowSurface(m_window);
      SDL_Rect dst {
        rect.x * m_scale, rect.y * m_scale,
        rect.w * m_scale, rect.h * m_scale
      };
      SDL_BlitScaled((SDL_Surface*)m_surface->nativeHandle(), &rect, nativeSurface, &dst);
    }
  }

  void SDL2Display::maximize()
  {
    SDL_MaximizeWindow(m_window);
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
    int x, y;
    SDL_GetWindowPosition(m_window, &x, &y);
    return "2 " + std::to_string(x) + " " + std::to_string(y);
  }

  void SDL2Display::setLayout(const std::string& layout)
  {
    std::istringstream s(layout);
    int ver, x, y;
    s >> ver;
    if (ver == 2) {
      s >> x >> y;
      SDL_SetWindowPosition(m_window, x, y);
    }
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

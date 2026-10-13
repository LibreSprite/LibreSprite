// SHE library
// Copyright (C) 2021-2026 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/she.h"
#include "she/system.h"

#include "base/concurrent_queue.h"
#include "base/exception.h"
#include "base/string.h"
#include "she/common/system.h"
#include "she/logger.h"
#include "she/native_cursor.h"
#include "she/sdl3/sdl3_display.h"
#include "she/sdl3/sdl3_surface.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <vector>

namespace she {
  SDL3Display* unique_display = nullptr;
  int display_scale;

  namespace sdl {
    she::SDL3Surface* screen;
    she::SDL3Surface* tempSurface;
    extern bool isMaximized;
    extern bool isMinimized;
    std::unordered_map<unsigned int, SDL3Display*> windowIdToDisplay;
  }

  SDL3Display::SDL3Display(int width, int height, int scale) :
    m_window(nullptr),
    m_renderer(nullptr),
    m_surface(nullptr),
    m_scale(0),
    m_nativeCursor(kNoCursor),
    m_restoredWidth(0),
    m_restoredHeight(0) {

    unique_display = this;

    width = 800;
    height = 600;

    instance()->gfx([&]{
      m_window = SDL_CreateWindow("",
				  width, height,
				  SDL_WINDOW_RESIZABLE);
      if (!m_window)
	      throw DisplayCreationException(SDL_GetError());

      if (gpu)
	      m_renderer = SDL_CreateRenderer(m_window, nullptr);

      sdl::windowIdToDisplay[SDL_GetWindowID(m_window)] = this;
      SDL_GetWindowSize(m_window, &width, &height);
      m_width = width;
      m_height = height;

      SDL_HideCursor();
    }, true);

    setScale(scale);
  }

  SDL3Display::~SDL3Display()
  {
    unique_display = NULL;
    if (sdl::tempSurface)
        sdl::tempSurface->dispose();
    if (m_window) {
      sdl::windowIdToDisplay.erase(SDL_GetWindowID(m_window));
      m_surface->dispose();
      if (m_doublebuffer)
	  m_doublebuffer->dispose();
      if (m_renderer)
        SDL_DestroyRenderer(m_renderer);
      SDL_DestroyWindow(m_window);
    }
  }

  void SDL3Display::dispose()
  {
    delete this;
  }

  void SDL3Display::toggleFullscreen()
  {
    m_isFullscreen = !m_isFullscreen;
    SDL_SetWindowFullscreen(m_window, m_isFullscreen);
  }

  bool SDL3Display::setIcon(Surface* surface) {
    SDL_SetWindowIcon(m_window, static_cast<SDL_Surface*>(static_cast<SDL3Surface*>(surface)->nativeHandle()));
    return true;
  }

  int SDL3Display::width() const
  {
    return m_width;
  }

  int SDL3Display::height() const
  {
    return m_height;
  }

  void SDL3Display::setWidth(int newWidth)
  {
    m_width = newWidth;
  }

  void SDL3Display::setHeight(int newHeight)
  {
    m_height = newHeight;
  }

  int SDL3Display::originalWidth() const
  {
    return m_restoredWidth > 0 ? m_restoredWidth: width();
  }

  int SDL3Display::originalHeight() const
  {
    return m_restoredHeight > 0 ? m_restoredHeight: height();
  }

  void SDL3Display::setOriginalWidth(int width)
  {
    m_restoredWidth = width;
  }

  void SDL3Display::setOriginalHeight(int height)
  {
    m_restoredHeight = height;
  }

  int SDL3Display::scale() const
  {
    return m_scale;
  }

  void SDL3Display::setScale(int scale)
  {
    ASSERT(scale >= 1);
    if (m_scale == scale)
      return;

    m_scale = scale;
    recreateSurface();
  }

  void SDL3Display::recreateSurface()
  {
    if (!m_scale)
      return;
    auto  newSurface = new SDL3Surface(width() / m_scale, height() / m_scale, SDL3Surface::DeleteAndDestroy);
    if (m_surface) {
      m_surface->blitTo(newSurface, 0, 0, 0, 0, width() / m_scale, height() / m_scale);
      m_surface->dispose();
    }
    m_dirty = true;
    m_surface = newSurface;
    she::sdl::screen = newSurface;

    #ifdef EMSCRIPTEN
    newSurface = new SDL3Surface(width() / m_scale, height() / m_scale, SDL3Surface::DeleteAndDestroy);
    if (m_doublebuffer) {
      m_doublebuffer->blitTo(newSurface, 0, 0, 0, 0, width(), height());
      m_doublebuffer->dispose();
    }
    m_doublebuffer = newSurface;
    #endif
  }

  Surface* SDL3Display::getSurface()
  {
    return m_surface;
  }

  void SDL3Display::present()
  {
    if (!m_dirty || !she::instance()->isGfxThread() || !m_surface)
      return;
    m_dirty = false;

    if (m_renderer) {
      #ifdef EMSCRIPTEN
      auto texture = static_cast<SDL3Surface*>(m_doublebuffer)->getTexture(nullptr);
      #else
      SDL_Rect empty{0, 0, 0, 0};
      auto texture = static_cast<SDL3Surface*>(m_surface)->getTexture(&empty);
      #endif
      SDL_RenderTexture(m_renderer, texture, nullptr, nullptr);
      SDL_RenderPresent(m_renderer);
    } else
      SDL_UpdateWindowSurface(m_window);
  }

  void SDL3Display::flip(const gfx::Rect& bounds)
  {
    m_dirty = true;
    if (!she::instance()->isGfxThread()) {
      SDL_Rect rect {bounds.x, bounds.y, bounds.w, bounds.h};
      SDL_Rect dst { rect.x, rect.y, rect.w, rect.h };
      SDL_BlitSurfaceScaled((SDL_Surface*)m_surface->nativeHandle(), &rect,
		     (SDL_Surface*)m_doublebuffer->nativeHandle(), &dst, SDL_SCALEMODE_NEAREST);
      return;
    }

    SDL_Rect rect {bounds.x, bounds.y, bounds.w, bounds.h};
    if (m_renderer) {
      static_cast<SDL3Surface*>(m_surface)->getTexture(&rect);
      return;
    }

    auto nativeSurface = SDL_GetWindowSurface(m_window);
    SDL_Rect dst {
      rect.x * m_scale, rect.y * m_scale,
      rect.w * m_scale, rect.h * m_scale
    };
    SDL_BlitSurfaceScaled((SDL_Surface*)m_surface->nativeHandle(), &rect, nativeSurface, &dst, SDL_SCALEMODE_NEAREST);
  }

  void SDL3Display::maximize()
  {
    SDL_MaximizeWindow(m_window);
  }

  bool SDL3Display::isMaximized() const
  {
    return sdl::isMaximized;
  }

  bool SDL3Display::isMinimized() const
  {
    return sdl::isMinimized;
  }

  void SDL3Display::setTitleBar(const std::string& title)
  {
    SDL_SetWindowTitle(m_window, title.c_str());
  }

  NativeCursor SDL3Display::nativeMouseCursor()
  {
    return m_nativeCursor;
  }

  std::vector<std::shared_ptr<SDL_Cursor>> m_cursors;

  void applyCursor(SDL_SystemCursor id)
  {
    if (id >= m_cursors.size()) {
      m_cursors.resize(id + 1);
    }
    if (!m_cursors[id]) {
      auto ptr = SDL_CreateSystemCursor(id);
      m_cursors[id] = std::shared_ptr<SDL_Cursor>(ptr, [](SDL_Cursor* ptr){
        if (ptr) {
          SDL_DestroyCursor(ptr);
        }
      });
    }
    SDL_SetCursor(m_cursors[id].get());
    SDL_ShowCursor();
  }

  bool SDL3Display::setNativeMouseCursor(NativeCursor cursor)
  {
    switch (cursor) {
    case she::kArrowCursor: applyCursor(SDL_SYSTEM_CURSOR_DEFAULT); return true;
    case she::kIBeamCursor: applyCursor(SDL_SYSTEM_CURSOR_TEXT); return true;
    case she::kWaitCursor: applyCursor(SDL_SYSTEM_CURSOR_WAIT); return true;
    case she::kLinkCursor: applyCursor(SDL_SYSTEM_CURSOR_POINTER); return true;
    case she::kForbiddenCursor: applyCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED); return true;
    case she::kMoveCursor: applyCursor(SDL_SYSTEM_CURSOR_MOVE); return true;
    default: break;
    }
    SDL_HideCursor();
    return false;
  }

  void SDL3Display::setMousePosition(const gfx::Point& position)
  {
    SDL_WarpMouseInWindow(
      m_window,
      (int)(m_scale * position.x),
      (int)(m_scale * position.y));
  }

  void SDL3Display::captureMouse()
  {
    SDL_CaptureMouse(true);
  }

  void SDL3Display::releaseMouse()
  {
    SDL_CaptureMouse(false);
  }

  std::string SDL3Display::getLayout()
  {
    int x, y;
    SDL_GetWindowPosition(m_window, &x, &y);
    return "2 " + std::to_string(x) + " " + std::to_string(y);
  }

  void SDL3Display::setLayout(const std::string& layout)
  {
    std::istringstream s(layout);
    int ver, x, y;
    s >> ver;
    if (ver == 2) {
      s >> x >> y;
      SDL_SetWindowPosition(m_window, x, y);
    }
  }

  void* SDL3Display::nativeHandle()
  {
    SDL_PropertiesID props = SDL_GetWindowProperties(m_window);
    if (!props)
      return nullptr;

  #if defined(_WIN32)
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
  #elif defined(__APPLE__)
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
  #elif defined(ANDROID)
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
  #elif defined(__linux__)
    // Only meaningful under X11; a Wayland-native session has no numeric
    // handle here, callers must tolerate nullptr.
    Sint64 xid = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    return xid ? reinterpret_cast<void*>(static_cast<uintptr_t>(xid)) : nullptr;
  #else
    return nullptr;
  #endif
  }
} // namespace she

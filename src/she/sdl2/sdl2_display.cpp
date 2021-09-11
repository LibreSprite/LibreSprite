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

#include "she/sdl2/display_events.h"

#ifdef USE_KEY_POLLER
#include "she/sdl2/key_poller.h"
#endif

#ifdef USE_MOUSE_POLLER
#include "she/sdl2/mouse_poller.h"
#endif

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

  namespace {

// #if _WIN32

// wndproc_t base_wndproc = NULL;
// bool display_has_mouse = false;
// bool capture_mouse = false;

// static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
// {
//   // Don't process DDE messages here because we cannot get the first
//   // DDE message correctly. The problem is that the message pump
//   // starts before we can subclass the Allegro HWND, so we don't have
//   // enough time to inject the code to process this kind of message.
//   //
//   // For more info see "Once you go input-idle, your application is
//   // deemed ready to receive DDE messages":
//   //   https://blogs.msdn.microsoft.com/oldnewthing/20140620-00/?p=693
//   //
//   // Anyway a possible solution would be to avoid processing the
//   // message loop until we subclass the HWND. I've tested this and it
//   // doesn't work, maybe because the message pump on Allegro 4 isn't
//   // in the main thread, I really don't know. But it just crash the
//   // whole system (Windows 10).

//   switch (msg) {

//     case WM_DROPFILES: {
//       HDROP hdrop = (HDROP)(wparam);
//       Event::Files files;

//       int count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
//       for (int index=0; index<count; ++index) {
//         int length = DragQueryFile(hdrop, index, NULL, 0);
//         if (length > 0) {
//           std::vector<TCHAR> str(length+1);
//           DragQueryFile(hdrop, index, &str[0], str.size());
//           files.push_back(base::to_utf8(&str[0]));
//         }
//       }

//       DragFinish(hdrop);

//       Event ev;
//       ev.setType(Event::DropFiles);
//       ev.setFiles(files);
//       queue_event(ev);
//       break;
//     }

// #ifndef USE_MOUSE_POLLER

//     case WM_MOUSEMOVE: {
//       // Adjust capture
//       if (capture_mouse) {
//         if (GetCapture() != hwnd)
//           SetCapture(hwnd);
//       }
//       else {
//         if (GetCapture() == hwnd)
//           ReleaseCapture();
//       }

//       //LOG("GetCapture=%p hwnd=%p\n", GetCapture(), hwnd);

//       Event ev;
//       ev.setPosition(gfx::Point(
//           GET_X_LPARAM(lparam) / display_scale,
//           GET_Y_LPARAM(lparam) / display_scale));

//       if (!display_has_mouse) {
//         display_has_mouse = true;

//         ev.setType(Event::MouseEnter);
//         queue_event(ev);

//         // Track mouse to receive WM_MOUSELEAVE message.
//         TRACKMOUSEEVENT tme;
//         tme.cbSize = sizeof(TRACKMOUSEEVENT);
//         tme.dwFlags = TME_LEAVE;
//         tme.hwndTrack = hwnd;
//         _TrackMouseEvent(&tme);
//       }

//       ev.setType(Event::MouseMove);
//       queue_event(ev);
//       break;
//     }

//     case WM_NCMOUSEMOVE:
//     case WM_MOUSELEAVE:
//       if (display_has_mouse) {
//         display_has_mouse = false;

//         Event ev;
//         ev.setType(Event::MouseLeave);
//         queue_event(ev);
//       }
//       break;

//     case WM_LBUTTONDOWN:
//     case WM_RBUTTONDOWN:
//     case WM_MBUTTONDOWN: {
//       Event ev;
//       ev.setType(Event::MouseDown);
//       ev.setPosition(gfx::Point(
//           GET_X_LPARAM(lparam) / display_scale,
//           GET_Y_LPARAM(lparam) / display_scale));
//       ev.setButton(
//         msg == WM_LBUTTONDOWN ? Event::LeftButton:
//         msg == WM_RBUTTONDOWN ? Event::RightButton:
//         msg == WM_MBUTTONDOWN ? Event::MiddleButton: Event::NoneButton);
//       queue_event(ev);
//       break;
//     }

//     case WM_LBUTTONUP:
//     case WM_RBUTTONUP:
//     case WM_MBUTTONUP: {
//       Event ev;
//       ev.setType(Event::MouseUp);
//       ev.setPosition(gfx::Point(
//           GET_X_LPARAM(lparam) / display_scale,
//           GET_Y_LPARAM(lparam) / display_scale));
//       ev.setButton(
//         msg == WM_LBUTTONUP ? Event::LeftButton:
//         msg == WM_RBUTTONUP ? Event::RightButton:
//         msg == WM_MBUTTONUP ? Event::MiddleButton: Event::NoneButton);
//       queue_event(ev);

//       // Avoid popup menu for scrollbars
//       if (msg == WM_RBUTTONUP)
//         return 0;

//       break;
//     }

//     case WM_LBUTTONDBLCLK:
//     case WM_MBUTTONDBLCLK:
//     case WM_RBUTTONDBLCLK: {
//       Event ev;
//       ev.setType(Event::MouseDoubleClick);
//       ev.setPosition(gfx::Point(
//           GET_X_LPARAM(lparam) / display_scale,
//           GET_Y_LPARAM(lparam) / display_scale));
//       ev.setButton(
//         msg == WM_LBUTTONDBLCLK ? Event::LeftButton:
//         msg == WM_RBUTTONDBLCLK ? Event::RightButton:
//         msg == WM_MBUTTONDBLCLK ? Event::MiddleButton: Event::NoneButton);
//       queue_event(ev);
//       break;
//     }

//     case WM_MOUSEWHEEL:
//     case WM_MOUSEHWHEEL: {
//       POINT pos = { GET_X_LPARAM(lparam),
//                     GET_Y_LPARAM(lparam) };
//       ScreenToClient(hwnd, &pos);

//       Event ev;
//       ev.setType(Event::MouseWheel);
//       ev.setPosition(gfx::Point(pos.x, pos.y) / display_scale);

//       int z = ((short)HIWORD(wparam)) / WHEEL_DELTA;
//       gfx::Point delta(
//         (msg == WM_MOUSEHWHEEL ? z: 0),
//         (msg == WM_MOUSEWHEEL ? -z: 0));
//       ev.setWheelDelta(delta);

//       //LOG("WHEEL: %d %d\n", delta.x, delta.y);

//       queue_event(ev);
//       break;
//     }

//     case WM_HSCROLL:
//     case WM_VSCROLL: {
//       POINT pos;
//       GetCursorPos(&pos);
//       ScreenToClient(hwnd, &pos);

//       Event ev;
//       ev.setType(Event::MouseWheel);
//       ev.setPosition(gfx::Point(pos.x, pos.y) / display_scale);

//       int bar = (msg == WM_HSCROLL ? SB_HORZ: SB_VERT);
//       int z = GetScrollPos(hwnd, bar);

//       switch (LOWORD(wparam)) {
//         case SB_LEFT:
//         case SB_LINELEFT:
//           --z;
//           break;
//         case SB_PAGELEFT:
//           z -= 2;
//           break;
//         case SB_RIGHT:
//         case SB_LINERIGHT:
//           ++z;
//           break;
//         case SB_PAGERIGHT:
//           z += 2;
//           break;
//         case SB_THUMBPOSITION:
//         case SB_THUMBTRACK:
//         case SB_ENDSCROLL:
//           // Do nothing
//           break;
//       }

//       gfx::Point delta(
//         (msg == WM_HSCROLL ? (z-50): 0),
//         (msg == WM_VSCROLL ? (z-50): 0));
//       ev.setWheelDelta(delta);

//       //LOG("SCROLL: %d %d\n", delta.x, delta.y);

//       SetScrollPos(hwnd, bar, 50, FALSE);

//       queue_event(ev);
//       break;
//     }

// #endif

//     case WM_NCCALCSIZE: {
//       if (wparam) {
//         // Scrollbars must be enabled and visible to get trackpad
//         // events of old drivers. So we cannot use ShowScrollBar() to
//         // hide them. This is a simple (maybe not so elegant)
//         // solution: Expand the client area to we overlap the
//         // scrollbars. In this way they are not visible, but we still
//         // get their messages.
//         NCCALCSIZE_PARAMS* cs = reinterpret_cast<NCCALCSIZE_PARAMS*>(lparam);
//         cs->rgrc[0].right += GetSystemMetrics(SM_CYVSCROLL);
//         cs->rgrc[0].bottom += GetSystemMetrics(SM_CYHSCROLL);
//       }
//       break;
//     }

//     case WM_NCHITTEST: {
//       LRESULT result = ::CallWindowProc(base_wndproc, hwnd, msg, wparam, lparam);
//       gfx::Point pt(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));

//       RECT rc;
//       ::GetClientRect(hwnd, &rc);
//       ::MapWindowPoints(hwnd, NULL, (POINT*)&rc, 2);
//       gfx::Rect area(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);

//       //LOG("NCHITTEST: %d %d - %d %d %d %d - %s\n", pt.x, pt.y, area.x, area.y, area.w, area.h, area.contains(pt) ? "true": "false");

//       // We ignore scrollbars so if the mouse is above them, we return
//       // as it's in the window client or resize area. (Remember that
//       // we have scroll bars are enabled and visible to receive
//       // trackpad messages only.)
//       if (result == HTHSCROLL) {
//         result = (area.contains(pt) ? HTCLIENT: HTBOTTOM);
//       }
//       else if (result == HTVSCROLL) {
//         result = (area.contains(pt) ? HTCLIENT: HTRIGHT);
//       }

//       return result;
//     }

//   }
//   return ::CallWindowProc(base_wndproc, hwnd, msg, wparam, lparam);
// }

// void subclass_hwnd(HWND hwnd)
// {
//   SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | WS_HSCROLL | WS_VSCROLL);
//   SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_ACCEPTFILES);

//   SCROLLINFO si;
//   si.cbSize = sizeof(SCROLLINFO);
//   si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
//   si.nMin = 0;
//   si.nPos = 50;
//   si.nMax = 100;
//   si.nPage = 10;
//   SetScrollInfo(hwnd, SB_HORZ, &si, FALSE);
//   SetScrollInfo(hwnd, SB_VERT, &si, FALSE);

//   base_wndproc = (wndproc_t)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wndproc);
// }

// void unsubclass_hwnd(HWND hwnd)
// {
//   SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)base_wndproc);
//   base_wndproc = NULL;
// }

// #endif // _WIN32

  } // anonymous namespace

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

// #if _WIN32
//   subclass_hwnd((HWND)nativeHandle());
// #endif
  }

  SDL2Display::~SDL2Display()
  {
    unique_display = NULL;
    sdl::windowIdToDisplay.erase(SDL_GetWindowID(m_window));

// #if _WIN32
//   unsubclass_hwnd((HWND)nativeHandle());
// #endif

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

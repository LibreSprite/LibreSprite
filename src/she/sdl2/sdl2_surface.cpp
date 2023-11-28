// SHE library
// Copyright (C) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/sdl2/sdl2_surface.h"

#include "base/string.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include <iostream>
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

namespace {

  she::DrawMode drawMode = she::DrawMode::Solid;
  int checkedModeOffset = 0;

}

namespace she {

  namespace sdl {
    extern SDL2Surface* screen;
  }

  inline gfx::Color from_sdl(SDL_PixelFormat *format, int color)
  {
    return gfx::rgba(
      (color & format->Rmask) >> format->Rshift << format->Rloss,
      (color & format->Gmask) >> format->Gshift << format->Gloss,
      (color & format->Bmask) >> format->Bshift << format->Bloss,
      (color & format->Amask) >> format->Ashift << format->Aloss
      );
  }

  inline int to_sdl(SDL_PixelFormat* format, gfx::Color color)
  {
    return SDL_MapRGBA(format, gfx::getr(color), gfx::getg(color), gfx::getb(color), gfx::geta(color));
  }

  SDL2Surface::SDL2Surface(SDL_Surface* bmp, DestroyFlag destroy)
    : m_bmp(bmp)
    , m_destroy(destroy)
    , m_lock(0)
  {
  }

  SDL2Surface::SDL2Surface(int width, int height, DestroyFlag destroy)
    : m_bmp(SDL_CreateRGBSurface(0, width, height, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000))
    , m_destroy(destroy)
    , m_lock(0)
  {
    if (!m_bmp) {
      throw std::runtime_error("Failed to create surface");
    }
  }

  SDL2Surface::SDL2Surface(int width, int height, int bpp, DestroyFlag destroy)
    : m_bmp(SDL_CreateRGBSurface(0, width, height, bpp, 0xFF, 0xFF00, 0xFF0000, bpp == 32 ? 0xFF000000 : 0))
    , m_destroy(destroy)
    , m_lock(0)
  {
    if (!m_bmp) {
      throw std::runtime_error("Failed to create surface");
    }
  }

  SDL2Surface::~SDL2Surface()
  {
    ASSERT(m_lock == 0);
    if (m_destroy & DestroyHandle) {
      if (m_bmp)
        SDL_FreeSurface(m_bmp);
    }
  }

// Surface implementation

  void SDL2Surface::dispose()
  {
    if (m_destroy & DeleteThis)
      delete this;
  }

  int SDL2Surface::width() const
  {
    return m_bmp->w;
  }

  int SDL2Surface::height() const
  {
    return m_bmp->h;
  }

  bool SDL2Surface::isDirectToScreen() const
  {
    return false;
    return m_bmp == she::sdl::screen->m_bmp;
  }

  gfx::Rect SDL2Surface::getClipBounds()
  {
    SDL_Rect rect;
    SDL_GetClipRect(m_bmp, &rect);
    return {rect.x, rect.y, rect.w, rect.h};
  }

  void SDL2Surface::setClipBounds(const gfx::Rect& rc)
  {
    SDL_Rect rect{rc.x, rc.y, rc.w, rc.h};
    SDL_SetClipRect(m_bmp, &rect);
  }

  bool SDL2Surface::intersectClipRect(const gfx::Rect& rc)
  {
    auto bounds = getClipBounds();
    bounds &= rc;
    setClipBounds(bounds);
    return !bounds.isEmpty();
  }

  void SDL2Surface::lock()
  {
    ASSERT(m_lock >= 0);
    if (m_lock++ == 0) {
      if (SDL_MUSTLOCK(m_bmp)) {
        SDL_LockSurface(m_bmp);
      }
    }
  }

  void SDL2Surface::unlock()
  {
    ASSERT(m_lock > 0);
    if (--m_lock == 0) {
      if (SDL_MUSTLOCK(m_bmp)) {
        SDL_UnlockSurface(m_bmp);
      }
    }
  }

  void SDL2Surface::setDrawMode(DrawMode mode, int param)
  {
    drawMode = mode;
    if (mode == she::DrawMode::Checked)
      checkedModeOffset = param;
  }

  void SDL2Surface::applyScale(int scale)
  {
    if (scale < 2)
      return;

    SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
      0,
      m_bmp->w * scale,
      m_bmp->h * scale,
      m_bmp->format->BitsPerPixel,
      m_bmp->format->format);

    SDL_Rect drect{0, 0, scaled->w, scaled->h};
    SDL_BlitScaled(m_bmp, nullptr, scaled, &drect);

    if (m_destroy & DestroyHandle)
      SDL_FreeSurface(m_bmp);

    m_bmp = scaled;
    m_destroy = DestroyHandle;
  }

  void* SDL2Surface::nativeHandle()
  {
    return reinterpret_cast<void*>(m_bmp);
  }

  void SDL2Surface::clear()
  {
    SDL_FillRect(m_bmp, nullptr, 0);
  }

  uint8_t* SDL2Surface::getData(int x, int y) const
  {
    return reinterpret_cast<uint8_t*>(m_bmp->pixels) + y * m_bmp->pitch + x * m_bmp->format->BytesPerPixel;
  }

  void SDL2Surface::getFormat(SurfaceFormatData* formatData) const
  {
    formatData->format = kRgbaSurfaceFormat;
    formatData->bitsPerPixel = m_bmp->format->BitsPerPixel;
    formatData->redShift   = m_bmp->format->Rshift;
    formatData->greenShift = m_bmp->format->Gshift;
    formatData->blueShift  = m_bmp->format->Bshift;
    formatData->alphaShift = m_bmp->format->Ashift;
    formatData->redMask    = m_bmp->format->Rmask;
    formatData->greenMask  = m_bmp->format->Gmask;
    formatData->blueMask   = m_bmp->format->Bmask;
    formatData->alphaMask  = m_bmp->format->Amask;
  }

  gfx::Color SDL2Surface::getPixel(int x, int y) const
  {
    int data = 0;
    if (m_bmp->format->BytesPerPixel == 4)
      data = *reinterpret_cast<uint32_t*>(getData(x, y));
    else if (m_bmp->format->BytesPerPixel == 2)
      data = *reinterpret_cast<uint16_t*>(getData(x, y));
    else if (m_bmp->format->BytesPerPixel == 1)
      data = *reinterpret_cast<uint8_t*>(getData(x, y));
    return from_sdl(m_bmp->format, data);
  }

  void SDL2Surface::putPixel(gfx::Color color, int x, int y)
  {
    auto clip = m_bmp->clip_rect;
    if (x < clip.x || x >= clip.x + clip.w || y < clip.y || y >= clip.y + clip.h)
      return;
    int sdlColor = to_sdl(m_bmp->format, color);
    if (m_bmp->format->BytesPerPixel == 4)
      *reinterpret_cast<uint32_t*>(getData(x, y)) = sdlColor;
    else if (m_bmp->format->BytesPerPixel == 2)
      *reinterpret_cast<uint16_t*>(getData(x, y)) = sdlColor;
    else if (m_bmp->format->BytesPerPixel == 1)
      *reinterpret_cast<uint8_t*>(getData(x, y)) = sdlColor;
  }

  void SDL2Surface::drawHLine(gfx::Color color, int x, int y, int w)
  {
    auto dlocked = m_bmp->locked;
    if (dlocked) SDL_UnlockSurface(m_bmp);

    SDL_Rect clip;
    SDL_GetClipRect(m_bmp, &clip);

    if (x < clip.x) {
      w += x - clip.x;
      x = clip.x;
    }

    if (x + w > clip.x + clip.w) {
      w = (clip.x + clip.w) - x;
    }

    if (w <= 0 || y < clip.y || y >= (clip.y + clip.h)) {
      return;
    }

    int sdlColor = to_sdl(m_bmp->format, gfx::rgba(gfx::getr(color), gfx::getg(color), gfx::getb(color)));
    auto data = getData(x, y);
    switch (drawMode) {
    case she::DrawMode::Solid:
      if (m_bmp->format->BytesPerPixel == 4) {
        int a = gfx::geta(color);
        int ia = 255 - a;
        int sr = gfx::getr(color) * a >> 8;
        int sg = gfx::getg(color) * a >> 8;
        int sb = gfx::getb(color) * a >> 8;
        for(; w--; data += 4) {
          int r = (data[0] * ia >> 8) + sr;
          int g = (data[1] * ia >> 8) + sg;
          int b = (data[2] * ia >> 8) + sb;
          *reinterpret_cast<uint32_t*>(data) = (r) | (g << 8) | (b << 16) | (data[3] << 24);
        }
      } else {
        clip = {
          x, y,
          w, 1
        };
        SDL_FillRect(m_bmp, &clip, sdlColor);
      }
      break;

    case she::DrawMode::Checked: {
      int offset = checkedModeOffset + x + y * 4;
      if (m_bmp->format->BytesPerPixel == 4) {
        for(; w--; data += 4)
          *reinterpret_cast<uint32_t*>(data) = ((++offset) & 7) < 4 ? 0xFFFFFFFF : 0xFF000000;
      } else if (m_bmp->format->BytesPerPixel == 2) {
        for(; w--; data += 2)
          *reinterpret_cast<uint16_t*>(data) = sdlColor;
      } else if (m_bmp->format->BytesPerPixel == 1) {
        for(; w--; data += 1)
          *reinterpret_cast<uint8_t*>(data) = sdlColor;
      }
      break;
    }

    case she::DrawMode::Xor:
      if (m_bmp->format->BytesPerPixel == 4) {
        for(; w--; data += 4)
          *reinterpret_cast<uint32_t*>(data) ^= sdlColor;
      } else if (m_bmp->format->BytesPerPixel == 2) {
        for(; w--; data += 2)
          *reinterpret_cast<uint16_t*>(data) ^= sdlColor;
      } else if (m_bmp->format->BytesPerPixel == 1) {
        for(; w--; data += 1)
          *reinterpret_cast<uint8_t*>(data) ^= sdlColor;
      }
      break;
    }

    if (dlocked) SDL_LockSurface(m_bmp);
  }

  void SDL2Surface::drawVLine(gfx::Color color, int x, int y, int h)
  {
    SDL_Rect clip;
    SDL_GetClipRect(m_bmp, &clip);

    if (y < clip.y) {
      h += y - clip.y;
      y = clip.y;
    }
    if (y + h > clip.y + clip.h) {
      h = (clip.y + clip.h) - y;
    }
    if (h <= 0 || x < clip.x || x >= clip.x + clip.w) {
      return;
    }
    int sdlColor = to_sdl(m_bmp->format, color);
    auto data = getData(x, y);
    auto stride = m_bmp->pitch;
    switch (drawMode) {
    case she::DrawMode::Solid:
      if (m_bmp->format->BytesPerPixel == 4) {
        int a = gfx::geta(color);
        int ia = 255 - a;
        int sr = gfx::getr(color) * a >> 8;
        int sg = gfx::getg(color) * a >> 8;
        int sb = gfx::getb(color) * a >> 8;
        for(; h--; data += stride) {
          int r = (data[0] * ia >> 8) + sr;
          int g = (data[1] * ia >> 8) + sg;
          int b = (data[2] * ia >> 8) + sb;
          *reinterpret_cast<uint32_t*>(data) = (r) | (g << 8) | (b << 16) | (data[3] << 24);
        }
      } else {
        clip = {
          x, y,
          1, h
        };
        SDL_FillRect(m_bmp, &clip, sdlColor);
      }
      break;

    case she::DrawMode::Checked: {
      int offset = checkedModeOffset + x + y;
      if (m_bmp->format->BytesPerPixel == 4) {
        for(; h--; data += stride)
          *reinterpret_cast<uint32_t*>(data) = ((++offset) & 7) < 4 ? 0xFFFFFFFF : 0xFF000000;
      } else if (m_bmp->format->BytesPerPixel == 2) {
        for(; h--; data += stride)
          *reinterpret_cast<uint16_t*>(data) = sdlColor;
      } else if (m_bmp->format->BytesPerPixel == 1) {
        for(; h--; data += stride)
          *reinterpret_cast<uint8_t*>(data) = sdlColor;
      }
      break;
    }

    case she::DrawMode::Xor:
      if (m_bmp->format->BytesPerPixel == 4) {
        for(; h--; data += stride)
          *reinterpret_cast<uint32_t*>(data) ^= sdlColor;
      } else if (m_bmp->format->BytesPerPixel == 2) {
        for(; h--; data += stride)
          *reinterpret_cast<uint16_t*>(data) ^= sdlColor;
      } else if (m_bmp->format->BytesPerPixel == 1) {
        for(; h--; data += stride)
          *reinterpret_cast<uint8_t*>(data) ^= sdlColor;
      }
      break;
    }
  }

  void SDL2Surface::drawLine(gfx::Color color, const gfx::Point& a, const gfx::Point& b)
  {
    if (a.x == b.x) {
      int y = a.y;
      int h = b.y - a.y;
      if (h < 0) {
        y = b.y;
        h = -h;
      }
      drawVLine(color, a.x, y, h);
      return;
    }

    if (a.y == b.y) {
      int x = a.x;
      int w = b.x - a.x;
      if (w < 0) {
        x = b.x;
        w = -w;
      }
      drawHLine(color, x, a.y, w);
      return;
    }

    printf("Unsupported line: %d,%d -> %d,%d\n", a.x, a.y, b.x, b.y);
  }

  void SDL2Surface::drawRect(gfx::Color color, const gfx::Rect& rc)
  {
    drawHLine(color, rc.x, rc.y, rc.w - 1);
    drawHLine(color, rc.x, rc.y + rc.h - 1, rc.w - 1);
    drawVLine(color, rc.x, rc.y, rc.h - 1);
    drawVLine(color, rc.x + rc.w - 1, rc.y, rc.h);
  }

  void SDL2Surface::fillRect(gfx::Color color, const gfx::Rect& rc)
  {
    SDL_Rect rect{rc.x, rc.y, rc.w, rc.h};
    SDL_FillRect(m_bmp, &rect, to_sdl(m_bmp->format, color));
  }

  void SDL2Surface::blitTo(Surface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height) const
  {
    ASSERT(m_bmp);
    ASSERT(dest);
    ASSERT(static_cast<SDL2Surface*>(dest)->m_bmp);

    SDL_Rect srect{srcx, srcy, width, height};
    SDL_Rect drect{dstx, dsty};

    auto srcbmp = static_cast<const SDL2Surface*>(dest)->m_bmp;
    auto slocked = srcbmp->locked;
    auto dlocked = m_bmp->locked;
    if (slocked) SDL_UnlockSurface(srcbmp);
    if (dlocked) SDL_UnlockSurface(m_bmp);
    SDL_BlitSurface(m_bmp, &srect, srcbmp, &drect);
    if (slocked) SDL_LockSurface(srcbmp);
    if (dlocked) SDL_LockSurface(m_bmp);
  }

  void SDL2Surface::scrollTo(const gfx::Rect& rc, int dx, int dy)
  {
    int w = width();
    int h = height();
    gfx::Clip clip(rc.x+dx, rc.y+dy, rc);
    if (!clip.clip(w, h, w, h))
      return;

    int bytesPerPixel = m_bmp->format->BytesPerPixel;
    int rowBytes = (int)m_bmp->pitch;
    int rowDelta;

    if (dy > 0) {
      clip.src.y += clip.size.h-1;
      clip.dst.y += clip.size.h-1;
      rowDelta = -rowBytes;
    }
    else
      rowDelta = rowBytes;

    char* dst = (char*)m_bmp->pixels;
    const char* src = dst;
    dst += rowBytes*clip.dst.y + bytesPerPixel*clip.dst.x;
    src += rowBytes*clip.src.y + bytesPerPixel*clip.src.x;
    w = bytesPerPixel*clip.size.w;
    h = clip.size.h;

    while (--h >= 0) {
      memmove(dst, src, w);
      dst += rowDelta;
      src += rowDelta;
    }
  }

  void SDL2Surface::drawSurface(const Surface* src, int dstx, int dsty)
  {
    src->blitTo(this, 0, 0, dstx, dsty, src->width(), src->height());
  }

  void SDL2Surface::drawRgbaSurface(const Surface* src, int dstx, int dsty)
  {
    src->blitTo(this, 0, 0, dstx, dsty, src->width(), src->height());
  }

} // namespace she
